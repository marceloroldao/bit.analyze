#!/usr/bin/env python3
import csv
import hashlib
import json
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
CORPUS_DIR = ROOT / "benchmarks" / "corpus" / "generated"
RESULTS = ROOT / "benchmarks" / "results"
RELEASE_SEED = "0xB17A2026"

BENCHMARKS = [
    "bit_analyze_same_corpus_baselines",
    "bit_analyze_compiled_encoder_benchmark",
    "bit_analyze_resource_usage_benchmark",
    "bit_analyze_serialized_overhead_benchmark",
    "bit_analyze_integrated_protected_memory_benchmark",
    "bit_analyze_corruption_probability_benchmark",
    "bit_analyze_criticality_group_failure_benchmark",
    "bit_analyze_protection_quantile_sweep",
]


def run(cmd, cwd=ROOT):
    return subprocess.run(cmd, cwd=cwd, check=True, text=True,
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


def main():
    RESULTS.mkdir(parents=True, exist_ok=True)
    CORPUS_DIR.mkdir(parents=True, exist_ok=True)

    generator = ROOT / "benchmarks" / "corpus" / "generate_corpus.py"
    run([sys.executable, str(generator), "--output", str(CORPUS_DIR), "--seed", RELEASE_SEED])

    corpus_files = sorted(p for p in CORPUS_DIR.rglob("*") if p.is_file() and p.name != "manifest.json")
    manifest = [{"path": str(p.relative_to(ROOT)), "bytes": p.stat().st_size, "sha256": sha256(p)} for p in corpus_files]
    (RESULTS / "corpus_manifest.json").write_text(json.dumps({"seed": RELEASE_SEED, "files": manifest}, indent=2), encoding="utf-8")

    try:
        commit = run(["git", "rev-parse", "HEAD"]).strip()
    except Exception:
        commit = "unknown"
    env = {
        "commit": commit,
        "platform": platform.platform(),
        "python": platform.python_version(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "cmake": shutil.which("cmake"),
        "zstd": shutil.which("zstd"),
        "release_seed": RELEASE_SEED,
    }
    (RESULTS / "environment.json").write_text(json.dumps(env, indent=2), encoding="utf-8")

    rows = []
    for name in BENCHMARKS:
        exe = executable(name)
        cmd = [str(exe)]
        if name == "bit_analyze_same_corpus_baselines":
            cmd += [str(p) for p in corpus_files]
        output = run(cmd)
        (RESULTS / f"{name}.txt").write_text(output, encoding="utf-8")
        rows.append({"benchmark": name, "exit": 0, "output_file": f"{name}.txt"})

    compression_csv = RESULTS / "external_compression.csv"
    compression = ROOT / "benchmarks" / "external_compression_baseline.py"
    run([sys.executable, str(compression), "--csv", str(compression_csv), *[str(p) for p in corpus_files]])
    rows.append({"benchmark": "external_compression", "exit": 0, "output_file": compression_csv.name})

    with (RESULTS / "summary.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["benchmark", "exit", "output_file"])
        w.writeheader(); w.writerows(rows)

    print(f"results_dir={RESULTS}")
    print(f"benchmarks={len(rows)}")
    print(f"corpus_files={len(corpus_files)}")
    print(f"release_seed={RELEASE_SEED}")


if __name__ == "__main__":
    main()
