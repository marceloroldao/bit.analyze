#!/usr/bin/env python3
"""Consume memoria.ia.server bit-analyze-ingest/v1 records safely.

The consumer validates provenance metadata and the captured object's SHA-256,
then invokes one long-running structural-ingest process for the selected batch.
The C++ process shares one HierarchicalMemory across all sources and persists
that relation state only after the complete batch succeeds.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from typing import Iterable

SCHEMA = "bit-analyze-ingest/v1"


def _read_cursor(path: Path) -> int:
    try:
        value = int(path.read_text(encoding="utf-8").strip())
        return max(0, value)
    except Exception:
        return 0


def _write_cursor(path: Path, offset: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(str(offset), encoding="utf-8")
    os.replace(tmp, path)


def _safe_object(root: Path, relative: str) -> Path:
    rel = Path(relative)
    if rel.is_absolute():
        raise ValueError("object_path must be relative to the raw capture root")
    root_resolved = root.resolve()
    candidate = (root_resolved / rel).resolve()
    try:
        common = Path(os.path.commonpath([str(root_resolved), str(candidate)]))
    except ValueError as exc:
        raise ValueError("object_path escapes raw capture root") from exc
    if common != root_resolved:
        raise ValueError("object_path escapes raw capture root")
    return candidate


def _sha256(path: Path, chunk_size: int = 1024 * 1024) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        while True:
            block = fh.read(chunk_size)
            if not block:
                break
            digest.update(block)
    return digest.hexdigest()


def validate_record(record: dict[str, object], root: Path) -> tuple[str, Path]:
    if record.get("schema") != SCHEMA:
        raise ValueError(f"unsupported ingest schema: {record.get('schema')!r}")
    source_id = str(record.get("source_id") or "")
    if not source_id or any(ch in source_id for ch in "\t\r\n"):
        raise ValueError("source_id must be non-empty and TSV-safe")
    object_path = str(record.get("object_path") or "")
    if not object_path:
        raise ValueError("object_path is required")

    path = _safe_object(root, object_path)
    if not path.is_file():
        raise ValueError(f"captured object does not exist: {object_path}")

    expected_length = int(record.get("byte_length") if record.get("byte_length") is not None else -1)
    if expected_length < 0 or path.stat().st_size != expected_length:
        raise ValueError(f"captured object length mismatch: {object_path}")

    expected_sha = str(record.get("sha256") or "").casefold()
    if len(expected_sha) != 64 or any(c not in "0123456789abcdef" for c in expected_sha):
        raise ValueError("sha256 must be a 64-character hexadecimal digest")
    actual_sha = _sha256(path)
    if actual_sha != expected_sha:
        raise ValueError(f"captured object SHA-256 mismatch: {object_path}")
    return source_id, path


def load_pending(
    spool: Path,
    root: Path,
    *,
    cursor_offset: int,
    max_records: int,
) -> tuple[list[tuple[str, Path]], int]:
    if max_records <= 0:
        raise ValueError("max_records must be > 0")
    selected: list[tuple[str, Path]] = []
    committed_offset = cursor_offset
    with spool.open("rb") as fh:
        fh.seek(cursor_offset)
        while len(selected) < max_records:
            raw = fh.readline()
            if not raw:
                break
            next_offset = fh.tell()
            if not raw.strip():
                committed_offset = next_offset
                continue
            try:
                record = json.loads(raw.decode("utf-8"))
            except Exception as exc:
                raise ValueError(f"invalid JSONL at byte offset {committed_offset}") from exc
            selected.append(validate_record(record, root))
            committed_offset = next_offset
    return selected, committed_offset


def _write_batch_tsv(items: Iterable[tuple[str, Path]], path: Path) -> None:
    with path.open("w", encoding="utf-8", newline="\n") as fh:
        for source_id, object_path in items:
            if "\t" in str(object_path) or "\n" in str(object_path) or "\r" in str(object_path):
                raise ValueError("captured object path is not TSV-safe")
            fh.write(f"{source_id}\t{object_path}\n")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Consume bit-analyze-ingest/v1 JSONL records")
    parser.add_argument("--spool", required=True, type=Path)
    parser.add_argument("--root", type=Path, help="raw capture root; defaults to spool parent")
    parser.add_argument("--binary", required=True, type=Path, help="bit_analyze_structural_ingest executable")
    parser.add_argument("--state", required=True, type=Path, help="persistent hierarchical relation state")
    parser.add_argument("--cursor", type=Path, help="byte-offset cursor; defaults beside the spool")
    parser.add_argument("--output", type=Path, help="StructuralEvent JSONL output; defaults to stdout")
    parser.add_argument("--max-records", type=int, default=100)
    parser.add_argument("--window", type=int, default=4096)
    parser.add_argument("--hop", type=int, default=4096)
    parser.add_argument("--chunk-size", type=int, default=65536)
    parser.add_argument("--layers", type=int, default=2)
    args = parser.parse_args(argv)

    spool = args.spool.resolve()
    root = (args.root or spool.parent).resolve()
    cursor = args.cursor or Path(str(spool) + ".cursor")
    start = _read_cursor(cursor)
    items, next_offset = load_pending(spool, root, cursor_offset=start, max_records=args.max_records)
    if not items:
        print("bit.analyze ingest: no pending records", file=sys.stderr)
        return 0

    with tempfile.TemporaryDirectory(prefix="bit-analyze-ingest-") as tmp:
        batch = Path(tmp) / "batch.tsv"
        _write_batch_tsv(items, batch)
        cmd = [
            str(args.binary),
            "--batch-list", str(batch),
            "--state", str(args.state),
            "--window", str(args.window),
            "--hop", str(args.hop),
            "--chunk-size", str(args.chunk_size),
            "--layers", str(args.layers),
        ]

        output_handle = None
        try:
            if args.output:
                args.output.parent.mkdir(parents=True, exist_ok=True)
                output_handle = args.output.open("a", encoding="utf-8", newline="\n")
            completed = subprocess.run(
                cmd,
                stdout=output_handle if output_handle is not None else None,
                check=False,
            )
        finally:
            if output_handle is not None:
                output_handle.close()

    if completed.returncode != 0:
        print(f"bit.analyze ingest failed with exit code {completed.returncode}; cursor unchanged", file=sys.stderr)
        return completed.returncode

    _write_cursor(cursor, next_offset)
    print(
        f"bit.analyze ingest committed: records={len(items)} cursor={next_offset} state={args.state}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
