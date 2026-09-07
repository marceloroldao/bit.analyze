# Publication Readiness Gate

This document defines the minimum bar for calling `bit.analyze` ready for a public experimental release.

The repository may be visible before all items pass. "Publication-ready" here means ready to tag and describe as a coherent, reproducible experimental version rather than a work-in-progress snapshot.

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

- [x] Fixed hierarchy baseline exists.
- [x] Adaptive hierarchy baseline exists.
- [x] Generalization benchmark exists.
- [x] Online vs global comparison exists.
- [x] Incremental learning benchmark exists.
- [x] Multi-cycle consolidation benchmark exists.
- [x] Real-file benchmark exists.
- [x] Real-file scale benchmark exists.
- [x] Deterministic generated multi-format corpus generator exists with explicit seed.
- [x] Same-corpus byte-bigram structural baseline exists.
- [x] Same-corpus fixed 8-byte dedup storage estimate exists.
- [x] Same-corpus RLE and simple deterministic LZ77 storage baselines exist.
- [x] External same-corpus gzip, bz2, lzma and zstd results are collected.
- [x] Generated-corpus compiled C++ result is archived in GitHub Actions artifact `10000393296` for run `34070708174`.
- [ ] Run and archive a reproducible independent real-file corpus result on compiled C++.

## Performance

- [x] Compiled trie encoder exists as an alternative to sequential rule application.
- [x] A benchmark exists for sequential vs compiled encode.
- [x] 1 MiB and 10 MiB exact round-trip test is implemented and passed in CI.
- [x] Cross-platform resource benchmark is implemented for 1 MiB, 10 MiB and 32 MiB.
- [x] Compiled resource benchmark was executed and archived on the release-benchmark runner.
- [x] Representative Linux CI result: ~258-267 MiB/s encode and ~296-328 MiB/s decode over 1-32 MiB structured inputs, with ~118 MB peak RSS at 32 MiB.
- [ ] Run the resource benchmark as evidence on Windows as well as Linux; Windows build/test is already green.

## Protection and recovery

- [x] Rule and trail integrity manifests exist.
- [x] Rule parity recovery exists for one and two damaged rules per protected group.
- [x] Trail recovery exists for one and two damaged symbols per protected group.
- [x] Burst/interleaving benchmark exists.
- [x] Random corruption probability benchmark exists.
- [x] Light / Medium / Strong protection profiles exist.
- [x] Adaptive rule-protection policy exists.
- [x] Adaptive trail-protection policy exists.
- [x] Integrated protected-memory benchmark exists.
- [x] Serialized protected-snapshot overhead benchmark exists.
- [x] Persisted simultaneous rule + trail corruption recovery test is implemented and passed through `ctest` in Linux and Windows CI.
- [x] Integrated protection and serialized-overhead benchmarks were executed and archived.
- [x] Current serialized protected-snapshot overhead result: 7.934% relative to the basic snapshot in the benchmark scenario.

## Persistence

- [x] Basic v1 snapshot persists rules and trails.
- [x] Protected v2 snapshot persists rules, trails, integrity hashes, protection profiles, interleaving configuration and P/Q parity data.
- [x] Save/load round-trip tests are implemented.
- [x] Restart reconstruction test is implemented with stable rule IDs.
- [x] Protected restart test is implemented: reload, corrupt, locate and recover from persisted protection state.
- [x] Both snapshot formats carry explicit version/magic fields; v1 is left intact while protected v2 uses a distinct format.
- [x] Protected v2 has whole-snapshot checksum / container corruption detection.

## Reproducibility and release hygiene

- [x] Linux/Windows GitHub Actions workflow is defined.
- [x] Ubuntu build + `ctest` fully green on run `34070708174`.
- [x] Windows build + `ctest` fully green on run `34070708174`.
- [x] Release benchmark collector records environment, commit SHA, corpus SHA-256 manifest and raw outputs.
- [x] CI uploads release benchmark evidence as an artifact after tests pass.
- [x] Benchmark seed/reproducibility policy is documented.
- [x] Stochastic benchmark audit is documented in `benchmarks/randomness_audit.md`; audited random sources use explicit fixed seeds.
- [x] Canonical generated-corpus release seed is `0xB17A2026` and is recorded by the release runner.
- [x] Representative release benchmark artifact archived: ID `10000393296`, ZIP SHA-256 `3cc73ce423e26827e99ebe963c27c03729a3bc94e472ebd8e77c7a2421f36494`.
- [x] README updated with architecture and explicit non-claims.
- [x] First measured release-candidate summary is archived in `benchmarks/release_candidate_results.md`.
- [ ] License reviewed for intended academic/commercial policy.
- [ ] Add changelog / release notes.
- [ ] Tag experimental `v0.1.0` only after the remaining release-quality gates pass.

## Measured negative result that must remain visible

On the deterministic 3,069,445-byte heterogeneous generated corpus, the current adaptive representation had an estimated total storage ratio of **3.5575x** after including its dictionary and 64-bit symbol trails. In the same run, fixed 8-byte dedup was ~1.3672x, the simple LZ77 baseline ~0.7287x, and actual gzip/bz2/lzma/zstd totals were all well below 1x.

Therefore this version must **not** be presented as a general-purpose compression improvement. The experiment remains interesting as a hierarchical relational memory system with stable IDs, online learning, consolidation and recovery properties.

## Current assessment

The previous major blocker—reproducible compiled execution—is closed. Linux and Windows both build and pass `ctest`, and the release-benchmark job generated a reproducible evidence artifact successfully.

The project is now **close to a defensible experimental `v0.1.0`**, but I would still complete one independent real-file corpus run and finalize legal/release hygiene before tagging it. The next technical gate is the independent corpus; the remaining non-technical gates are license choice and release notes/changelog.
