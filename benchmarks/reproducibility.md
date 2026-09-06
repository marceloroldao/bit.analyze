# Benchmark reproducibility policy

All randomized experiments in `bit.analyze` must be deterministic by default.

## Rules

- Every randomized C++ benchmark must use an explicit fixed seed.
- Seeds must be printed in benchmark output.
- Corpus generation must use a fixed seed unless the caller explicitly overrides it.
- A benchmark result is not release-grade unless its raw output is archived together with commit SHA, platform, compiler and corpus manifest.
- Randomized benchmarks should not use `std::random_device` for release evidence.

## Canonical seeds

| Experiment family | Seed |
| --- | ---: |
| corpus generator | `0xB17A4A` |
| corruption probability | `0xB17A4A` |
| criticality / protection | `0xC171C` |
| generic randomized stress | `0x51A7E` |

Different experiments may use different fixed seeds, but the selected value must remain visible in source and output.

## Release evidence

Use `benchmarks/run_release_benchmarks.py` to produce a results directory containing:

- `environment.json`
- one raw `.txt` output per benchmark
- `summary.csv`
- corpus manifest/checksums

The collected directory should be archived for each release candidate rather than manually copying individual values into documentation.
