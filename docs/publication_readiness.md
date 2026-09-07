# Publication Readiness Gate

This document defines the minimum bar for calling `bit.analyze` ready for a public experimental release.

## Core invariants

- [x] Exact lossless reconstruction from stored trails.
- [x] Adaptive relation learning is append-only for online updates.
- [x] Old trails remain decodable after later learning.
- [x] Consolidation can add higher-order relations without renumbering old IDs.
- [x] Integrity layer detects rule corruption.
- [x] Integrity layer detects and localizes trail corruption by block.
- [x] Single-rule recovery is defined with parity.
- [x] Two-rule recovery is defined with independent GF(256) parity equations.
- [x] Single- and double-symbol trail recovery are implemented.
- [x] Interleaving is reversible and improves burst-error tolerance structurally.

## Representation and learning benchmarks

- [x] Fixed hierarchy and adaptive hierarchy baselines exist.
- [x] Generalization, online/global, incremental-learning and multi-cycle consolidation benchmarks exist.
- [x] Real-file and real-file scale benchmarks exist.
- [x] Deterministic generated multi-format corpus exists with explicit seed.
- [x] Same-corpus byte-bigram, fixed-block, RLE and deterministic LZ77 baselines exist.
- [x] Same-corpus gzip, bz2, lzma and zstd results are collected.
- [x] Generated-corpus compiled C++ evidence archived in GitHub Actions artifact `10000393296`, run `34070708174`.
- [x] Independent Canterbury Corpus executed successfully in run `34071048369`.
- [x] Canterbury artifact `10000500422`, ZIP SHA-256 `839ce7c3ea6148a6c1db083f26dd82bb1ccdbe991372aac50ea841eb4feb32ae`.
- [x] Independent result summarized in `benchmarks/canterbury_results.md`.

## Performance

- [x] Compiled trie encoder exists.
- [x] Sequential vs compiled benchmark exists.
- [x] 1 MiB and 10 MiB exact round-trip test passed in CI.
- [x] Resource benchmark exists for 1 MiB, 10 MiB and 32 MiB.
- [x] Linux resource evidence archived; representative result ~258-267 MiB/s encode and ~296-328 MiB/s decode, ~118 MB peak RSS at 32 MiB.
- [ ] Windows resource benchmark evidence remains desirable follow-up, but is not a blocker because Windows build/test correctness is green.

## Protection, recovery and persistence

- [x] Rule and trail integrity manifests exist.
- [x] One/two damaged-rule recovery exists.
- [x] One/two damaged-symbol trail recovery exists.
- [x] Burst/interleaving and random-corruption benchmarks exist.
- [x] Light / Medium / Strong adaptive protection exists for rules and trails.
- [x] Integrated protected-memory benchmark exists.
- [x] Basic v1 and protected v2 snapshots persist state.
- [x] Protected v2 persists integrity/protection/interleaving/P-Q data.
- [x] Restart, corruption localization and recovery tests passed in Linux and Windows CI.
- [x] Protected v2 includes whole-snapshot corruption detection.
- [x] Serialized protected-snapshot overhead benchmark executed; current scenario measured 7.934% over basic snapshot.

## Reproducibility and release hygiene

- [x] Ubuntu build + `ctest` green.
- [x] Windows build + `ctest` green.
- [x] Release collector records environment, commit, corpus SHA-256 manifest and raw outputs.
- [x] Deterministic seed policy and stochastic benchmark audit documented.
- [x] Generated-corpus and independent Canterbury evidence archived.
- [x] README updated with architecture, measured limitations and non-claims.
- [x] `CHANGELOG.md` contains the v0.1.0 candidate entry.
- [x] `docs/release_notes_v0.1.0.md` exists.
- [x] License policy finalized: **Resolutive Research and Non-Commercial License (RRNCL) v1.0**.
- [x] `LICENSE` added with copyright (c) 2026 Marcelo Roldão Matos.
- [x] README explicitly states that no commercial rights are granted and that RRNCL is not an OSI-approved open-source license.
- [ ] Create experimental `v0.1.0` tag/release from the final reviewed commit.

## Measured negative result that must remain visible

On the deterministic 3,069,445-byte heterogeneous generated corpus, the current adaptive representation had an estimated total storage ratio of **3.5575x** after including its dictionary and 64-bit symbol trails. Fixed 8-byte dedup was ~1.3672x, simple LZ77 ~0.7287x, and gzip/bz2/lzma/zstd were all well below 1x.

The independent 2,810,784-byte Canterbury Corpus confirmed the same conclusion: adaptive estimate **3.283670x**, versus 0.675709x for simple LZ77 and 0.258985x / 0.193081x / 0.183721x / 0.175424x for gzip / bz2 / zstd / lzma.

Therefore v0.1.0 must **not** be presented as a general-purpose compression improvement. The experiment is released as a hierarchical relational memory system exploring stable IDs, online learning, consolidation, persistence and recovery.

## Current assessment

All technical and licensing gates required for the first public experimental candidate are closed. The repository is ready for final commit review and creation of the experimental `v0.1.0` tag/release under RRNCL v1.0.
