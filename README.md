# bit.analyze

Experimental hierarchical binary-memory engine.

`bit.analyze` investigates whether raw binary data can be represented as a hierarchy of reusable relations, where recurring structures receive stable IDs and can participate in higher-order relations.

The project is intentionally separate from `memoria.ia`.

## Research question

Instead of treating a file only as an opaque blob or fixed-size chunks, the engine asks whether repeated low-level structures can become reusable symbols:

```text
raw bytes
   ↓
base symbols (0..255)
   ↓
recurring relations
   ↓
stable relation IDs
   ↓
relations between relations
   ↓
structural trails
```

The current project does **not** claim semantic understanding, cognition, or superiority over conventional compression. Those are experimental questions, not assumptions.

## Current architecture

### Representation

- exact lossless reconstruction from symbol trails;
- fixed hierarchical baseline;
- adaptive recurring-pair hierarchy;
- append-only online learning with stable IDs;
- consolidation that adds higher-order relations without renumbering old IDs;
- compiled trie encoder as an alternative to sequential rule application.

### Integrity and recovery

- rule fingerprints;
- whole-trail and block-level fingerprints;
- single-rule recovery with parity;
- two-rule recovery using independent GF(256) P/Q equations;
- single- and double-symbol trail recovery;
- reversible interleaving to improve burst-error tolerance;
- Light / Medium / Strong protection policies;
- adaptive rule and trail protection based on structural criticality.

### Persistence

Two experimental snapshot formats currently exist:

- basic v1: rules + trails;
- protected v2: rules, trails, integrity hashes, protection profiles, interleaving configuration and persisted P/Q parity.

Protected v2 also carries a whole-file checksum so a damaged snapshot container is rejected before its state is trusted.

Snapshot formats are experimental and are **not yet declared a stable public compatibility contract**.

## What has been observed so far

Release-candidate experiments support narrower conclusions:

- adaptive rules can reuse known structures when their position changes;
- append-only learning preserves old trails;
- periodic consolidation can substantially shorten online representations on structured corpora;
- random data can be rejected by stricter support/lift thresholds instead of generating arbitrary hierarchy indefinitely;
- interleaving strongly improves resistance to localized burst damage, but does not solve high-rate uniformly random corruption;
- adaptive protection can concentrate redundancy on structurally important relations instead of applying maximum protection everywhere;
- the current representation is **not competitive as a general-purpose compressor**. This negative result was reproduced on both the deterministic release corpus and the independent Canterbury Corpus.

Measured release-candidate evidence is recorded in [`benchmarks/release_candidate_results.md`](benchmarks/release_candidate_results.md) and [`benchmarks/canterbury_results.md`](benchmarks/canterbury_results.md).

## Repository layout

```text
include/      public C++ headers
src/          C++17 core
examples/     usage examples
tests/        reconstruction, learning, integrity, recovery and persistence invariants
benchmarks/   comparative and stress experiments
docs/         architecture, methodology and publication-readiness notes
.github/      CI configuration
```

## Build

Requirements:

- CMake 3.16+
- C++17 compiler

Linux/macOS-style build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Windows with a multi-config generator:

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Benchmark policy

A useful result must be compared against simple conventional baselines where applicable, including:

- fixed-size blocks;
- byte n-grams;
- conventional compression/deduplication;
- online versus globally-trained vocabulary.

Negative results are retained. A shorter trail alone is not sufficient evidence of a better memory system because dictionary growth and protection overhead also have a cost.

## Release status

Current status: **v0.1.0 experimental release candidate**.

Cross-platform CI, deterministic release benchmarks, independent Canterbury Corpus evaluation, persistence/recovery tests and same-corpus baselines have been completed. The release gate is tracked in [`docs/publication_readiness.md`](docs/publication_readiness.md).

## License

Copyright (c) 2026 Marcelo Roldão Matos.

This project is distributed under the **Resolutive Research and Non-Commercial License (RRNCL) v1.0**. Academic/scientific research, education, personal experimentation and other permitted non-commercial uses are allowed under the terms of [`LICENSE`](LICENSE). **No commercial rights are granted.** Commercial use requires separate written authorization or licensing from the copyright holder.

Because the license restricts commercial use, this repository should not be represented as OSI-approved open-source software.

## Principles

- lossless reconstruction is mandatory;
- stable IDs must preserve old memories across later learning;
- negative results are valid results;
- benchmarks must include strong simple baselines;
- structural similarity must not be confused with semantics;
- randomized experiments should be deterministic/reproducible;
- protection overhead must be measured together with representation cost.
