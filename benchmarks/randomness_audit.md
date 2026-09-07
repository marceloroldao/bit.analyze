# Randomness and seed audit

Release-grade benchmarks must be deterministic unless a benchmark explicitly documents a multi-seed protocol.

## Policy

- No `std::random_device`, wall-clock seed, PID-derived seed, or implicit library seed is allowed in release evidence.
- Every stochastic C++ benchmark must construct its RNG from an explicit numeric seed.
- The generated corpus accepts `--seed` and records it in `manifest.json`.
- Release collection archives SHA-256 hashes for every generated corpus file.
- If a benchmark uses several independent stochastic scenarios, each scenario should use a stable derived seed rather than sharing hidden mutable global state.

## Audited stochastic benchmarks

- `corruption_probability_benchmark.cpp`: `std::mt19937_64` seeded explicitly (`0xB17A4A`).
- `criticality_failure_benchmark.cpp`: `std::mt19937_64` seeded explicitly (`0xB17A`).
- `criticality_group_failure_benchmark.cpp`: `std::mt19937_64` seeded explicitly (`0xC171C`).
- `protection_quantile_sweep.cpp`: `evaluate()` receives explicit per-scenario seeds (`1001`, `1005`, `1010`).
- `benchmarks/corpus/generate_corpus.py`: deterministic `random.Random(seed)` with command-line `--seed`.

The remaining benchmark programs are deterministic by construction or operate on caller-supplied files. Any future use of `<random>` must be added to this audit before release evidence is accepted.

## Release seed

The canonical generated-corpus release seed is:

```text
0xB17A2026
```

The release runner invokes the corpus generator with this seed explicitly and archives the resulting manifest and file checksums.
