#!/usr/bin/env python3
import argparse
import bz2
import csv
import gzip
import lzma
import shutil
import subprocess
import tempfile
from pathlib import Path


def zstd_size(path: Path):
    exe = shutil.which("zstd")
    if not exe:
        return None
    with tempfile.TemporaryDirectory() as td:
        out = Path(td) / "out.zst"
        subprocess.run([exe, "-q", "-19", "-f", str(path), "-o", str(out)], check=True)
        return out.stat().st_size


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--csv", dest="csv_path", type=Path)
    args = ap.parse_args()

    rows = []
    for raw in args.files:
        p = Path(raw)
        if not p.is_file():
            continue
        data = p.read_bytes()
        n = len(data)
        gz = len(gzip.compress(data, compresslevel=9, mtime=0))
        bz = len(bz2.compress(data, compresslevel=9))
        xz = len(lzma.compress(data, preset=9 | lzma.PRESET_EXTREME))
        zs = zstd_size(p)
        row = {
            "file": str(p),
            "input_bytes": n,
            "gzip_bytes": gz,
            "bz2_bytes": bz,
            "lzma_bytes": xz,
            "zstd_bytes": "" if zs is None else zs,
            "gzip_ratio": 0 if n == 0 else gz / n,
            "bz2_ratio": 0 if n == 0 else bz / n,
            "lzma_ratio": 0 if n == 0 else xz / n,
            "zstd_ratio": "" if zs is None or n == 0 else zs / n,
        }
        rows.append(row)

    fields = ["file","input_bytes","gzip_bytes","bz2_bytes","lzma_bytes","zstd_bytes",
              "gzip_ratio","bz2_ratio","lzma_ratio","zstd_ratio"]
    out = None
    if args.csv_path:
        args.csv_path.parent.mkdir(parents=True, exist_ok=True)
        out = args.csv_path.open("w", newline="", encoding="utf-8")
    else:
        import sys
        out = sys.stdout
    w = csv.DictWriter(out, fieldnames=fields)
    w.writeheader(); w.writerows(rows)
    if args.csv_path:
        out.close()


if __name__ == "__main__":
    main()
