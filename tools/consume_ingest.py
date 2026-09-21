#!/usr/bin/env python3
"""Consume memoria.ia.server bit-analyze-ingest/v1 records safely.

Each successful batch produces immutable StructuralEvent JSONL plus immutable
hierarchical state. A single current.json pointer atomically commits both the
state and the consumed spool offset, so a crash cannot advance one without the
other.
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
import uuid
from typing import Iterable

SCHEMA = "bit-analyze-ingest/v1"
CHECKPOINT_SCHEMA = "bit-analyze-ingest-checkpoint/v2"
LEGACY_CHECKPOINT_SCHEMA = "bit-analyze-ingest-checkpoint/v1"


def _safe_child(root: Path, relative: str, *, field: str) -> Path:
    rel = Path(relative)
    if rel.is_absolute():
        raise ValueError(f"{field} must be relative")
    root_resolved = root.resolve()
    candidate = (root_resolved / rel).resolve()
    try:
        common = Path(os.path.commonpath([str(root_resolved), str(candidate)]))
    except ValueError as exc:
        raise ValueError(f"{field} escapes its root") from exc
    if common != root_resolved:
        raise ValueError(f"{field} escapes its root")
    return candidate


def _safe_object(root: Path, relative: str) -> Path:
    return _safe_child(root, relative, field="object_path")


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
    content_source_id = str(record.get("source_id") or "")
    capture_id = str(record.get("capture_id") or "")
    source_id = capture_id or content_source_id
    if not source_id or any(ch in source_id for ch in "\t\r\n"):
        raise ValueError("capture_id/source_id must be non-empty and TSV-safe")

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
    if _sha256(path) != expected_sha:
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
            object_text = str(object_path)
            if any(ch in object_text for ch in "\t\r\n"):
                raise ValueError("captured object path is not TSV-safe")
            fh.write(f"{source_id}\t{object_text}\n")


def load_checkpoint(checkpoint_dir: Path) -> tuple[int, Path | None, dict[str, object] | None]:
    pointer = checkpoint_dir / "current.json"
    if not pointer.exists():
        return 0, None, None
    data = json.loads(pointer.read_text(encoding="utf-8"))
    if data.get("schema") not in {CHECKPOINT_SCHEMA, LEGACY_CHECKPOINT_SCHEMA}:
        raise ValueError("unsupported checkpoint schema")
    offset = int(data.get("cursor_offset", -1))
    if offset < 0:
        raise ValueError("checkpoint cursor_offset must be >= 0")
    state_rel = str(data.get("state_file") or "")
    if not state_rel:
        raise ValueError("checkpoint state_file is required")
    state_path = _safe_child(checkpoint_dir, state_rel, field="state_file")
    if not state_path.is_file():
        raise ValueError("checkpoint state file is missing")
    events_rel = str(data.get("events_file") or "")
    if events_rel:
        events_path = _safe_child(checkpoint_dir, events_rel, field="events_file")
        if not events_path.is_file():
            raise ValueError("checkpoint events file is missing")
    return offset, state_path, data


def resolve_lineage(
    previous: dict[str, object] | None,
    structural_config: dict[str, int],
) -> tuple[str, str]:
    if previous is None:
        return "hierarchy:" + uuid.uuid4().hex, "created"
    previous_config = previous.get("structural_config")
    if previous_config is not None and previous_config != structural_config:
        raise ValueError(
            "structural configuration changed for an existing hierarchy; "
            "use a new checkpoint directory to start a new lineage"
        )
    hierarchy_id = str(previous.get("hierarchy_id") or "")
    if hierarchy_id:
        return hierarchy_id, str(previous.get("lineage_origin") or "created")
    return "hierarchy:" + uuid.uuid4().hex, "legacy_checkpoint_adopted"


def commit_checkpoint(checkpoint_dir: Path, payload: dict[str, object]) -> None:
    checkpoint_dir.mkdir(parents=True, exist_ok=True)
    manifest_rel = str(payload.get("checkpoint_file") or "")
    if not manifest_rel:
        raise ValueError("checkpoint_file is required")
    manifest = _safe_child(checkpoint_dir, manifest_rel, field="checkpoint_file")
    if manifest.exists():
        raise ValueError("immutable checkpoint manifest already exists")
    manifest.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    pointer = checkpoint_dir / "current.json"
    tmp = checkpoint_dir / "current.json.tmp"
    tmp.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    os.replace(tmp, pointer)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Consume bit-analyze-ingest/v1 JSONL records")
    parser.add_argument("--spool", required=True, type=Path)
    parser.add_argument("--root", type=Path, help="raw capture root; defaults to spool parent")
    parser.add_argument("--binary", required=True, type=Path, help="bit_analyze_structural_ingest executable")
    parser.add_argument("--checkpoint-dir", required=True, type=Path)
    parser.add_argument("--max-records", type=int, default=100)
    parser.add_argument("--window", type=int, default=4096)
    parser.add_argument("--hop", type=int, default=4096)
    parser.add_argument("--chunk-size", type=int, default=65536)
    parser.add_argument("--layers", type=int, default=2)
    parser.add_argument("--emit-stdout", action="store_true", help="emit committed event batch after checkpoint")
    args = parser.parse_args(argv)

    spool = args.spool.resolve()
    root = (args.root or spool.parent).resolve()
    checkpoint_dir = args.checkpoint_dir.resolve()
    checkpoint_dir.mkdir(parents=True, exist_ok=True)

    start_offset, state_in, previous = load_checkpoint(checkpoint_dir)
    if previous is not None:
        previous_spool = str(previous.get("spool") or "")
        if previous_spool and Path(previous_spool).resolve() != spool:
            raise ValueError("checkpoint belongs to a different ingest spool")

    structural_config = {
        "window": int(args.window),
        "hop": int(args.hop),
        "layers": int(args.layers),
    }
    hierarchy_id, lineage_origin = resolve_lineage(previous, structural_config)

    items, next_offset = load_pending(
        spool,
        root,
        cursor_offset=start_offset,
        max_records=args.max_records,
    )
    if not items:
        print("bit.analyze ingest: no pending records", file=sys.stderr)
        return 0

    token = f"{next_offset}-{uuid.uuid4().hex}"
    state_out = checkpoint_dir / f"state-{token}.bin"
    events_out = checkpoint_dir / f"events-{token}.jsonl"
    checkpoint_file = checkpoint_dir / f"checkpoint-{token}.json"

    with tempfile.TemporaryDirectory(prefix="bit-analyze-ingest-") as tmp:
        batch = Path(tmp) / "batch.tsv"
        _write_batch_tsv(items, batch)
        cmd = [
            str(args.binary),
            "--batch-list", str(batch),
            "--state-out", str(state_out),
            "--window", str(args.window),
            "--hop", str(args.hop),
            "--chunk-size", str(args.chunk_size),
            "--layers", str(args.layers),
        ]
        if state_in is not None:
            cmd.extend(["--state-in", str(state_in)])

        with events_out.open("w", encoding="utf-8", newline="\n") as event_stream:
            completed = subprocess.run(cmd, stdout=event_stream, check=False)

    if completed.returncode != 0:
        state_out.unlink(missing_ok=True)
        events_out.unlink(missing_ok=True)
        print(
            f"bit.analyze ingest failed with exit code {completed.returncode}; checkpoint unchanged",
            file=sys.stderr,
        )
        return completed.returncode

    if not state_out.is_file():
        events_out.unlink(missing_ok=True)
        raise RuntimeError("structural ingest succeeded without producing state")

    checkpoint = {
        "schema": CHECKPOINT_SCHEMA,
        "hierarchy_id": hierarchy_id,
        "lineage_origin": lineage_origin,
        "structural_config": structural_config,
        "io_chunk_size": int(args.chunk_size),
        "cursor_offset": next_offset,
        "previous_cursor_offset": start_offset,
        "state_file": state_out.name,
        "events_file": events_out.name,
        "checkpoint_file": checkpoint_file.name,
        "record_count": len(items),
        "spool": str(spool),
        "previous_state_file": None if previous is None else previous.get("state_file"),
        "previous_checkpoint_file": None if previous is None else previous.get("checkpoint_file"),
    }
    commit_checkpoint(checkpoint_dir, checkpoint)

    if args.emit_stdout:
        with events_out.open("r", encoding="utf-8") as fh:
            for line in fh:
                sys.stdout.write(line)

    print(
        f"bit.analyze ingest committed: records={len(items)} cursor={next_offset} "
        f"state={state_out.name} events={events_out.name}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
