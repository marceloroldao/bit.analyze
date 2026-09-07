#!/usr/bin/env python3
import csv
import hashlib
import json
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
WORK = ROOT / "benchmarks" / "external_corpus" / "canterbury"
FILES_DIR = WORK / "files"
RESULTS = ROOT / "benchmarks" / "results" / "canterbury"
URL = "https://corpus.canterbury.ac.nz/resources/cantrbry.zip"

EXPECTED = {
    "alice29.txt": 152089,
    "asyoulik.txt": 125179,
    "cp.html": 24603,
    "fields.c": 11150,
    "grammar.lsp": 3721,
    "kennedy.xls": 1029744,
    "lcet10.txt": 426754,
    "plrabn12.txt": 481861,
    "ptt5": 513216,
    "sum": 38240,
    "xargs.1": 4227,
}


def run(cmd):
    return subprocess.run(cmd, cwd=ROOT, check=True, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout


def sha256(path: Path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def executable(name: str):
    candidates = [BUILD / name, BUILD / "Release" / (name + ".exe"), BUILD / (name + ".exe")]
    for p in candidates:
        if p.exists():
            return p
    raise FileNotFoundError(name)


def safe_extract(z: zipfile.ZipFile, dest: Path):
    root = dest.resolve()
    for info in z.infolist():
        target = (dest / info.filename).resolve()
        if root not in target.parents and target != root:
            raise RuntimeError(f"unsafe zip path: {info.filename}")
    z.extractall(dest)


def main():
    shutil.rmtree(WORK, ignore_errors=True)
    FILES_DIR.mkdir(parents=True, exist_ok=True)
    RESULTS.mkdir(parents=True, exist_ok=True)

    archive = WORK / "cantrbry.zip"
    with urllib.request.urlopen(URL, timeout=60) as r, archive.open("wb") as out:
        shutil.copyfileobj(r, out)

    archive_sha = sha256(archive)
    with zipfile.ZipFile(archive) as z:
        safe_extract(z, FILES_DIR)

    actual = {}
    for name, expected_size in EXPECTED.items():
        matches = list(FILES_DIR.rglob(name))
        if len(matches) != 1:
            raise RuntimeError(f"expected exactly one {name}, found {len(matches)}")
        path = matches[0]
        size = path.stat().st_size
        if size != expected_size:
            raise RuntimeError(f"size mismatch for {name}: {size} != {expected_size}")
        actual[name] = path

    ordered = [actual[name] for name in EXPECTED]
    manifest = {
        "source": URL,
        "archive_sha256": archive_sha,
        "expected_total_bytes": sum(EXPECTED.values()),
        "files": [
            {"name": name, "bytes": EXPECTED[name], "sha256": sha256(actual[name])}
            for name in EXPECTED
        ],
    }
    (RESULTS / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    baseline = executable("bit_analyze_same_corpus_baselines")
    output = run([str(baseline), *map(str, ordered)])
    (RESULTS / "bit_analyze_same_corpus_baselines.txt").write_text(output, encoding="utf-8")

    compression_csv = RESULTS / "external_compression.csv"
    compression = ROOT / "benchmarks" / "external_compression_baseline.py"
    run([sys.executable, str(compression), "--csv", str(compression_csv), *map(str, ordered)])

    with (RESULTS / "summary.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["source", "files", "input_bytes", "archive_sha256"])
        w.writerow([URL, len(ordered), sum(EXPECTED.values()), archive_sha])

    print(f"canterbury_files={len(ordered)}")
    print(f"canterbury_bytes={sum(EXPECTED.values())}")
    print(f"archive_sha256={archive_sha}")
    print(f"results_dir={RESULTS}")


if __name__ == "__main__":
    main()
