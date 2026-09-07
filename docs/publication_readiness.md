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
- [x] Independent Canterbury Corpus result executed successfully on compiled C++ in run `34071048369`.
- [x] Canterbury evidence archived as artifact `10000500422`, ZIP SHA-256 `839ce7c3ea6148a6c1db083f26dd82bb1ccdbe991372aac50ea841eb4feb32ae`.
- [x] Independent result summarized in `benchmarks/canterbury_results.md`.

## Performance

- [x] Compiled trie encoder exists as an alternative to sequential rule application.
- [x] A benchmark exists for sequential vs compiled encode.
- [x] 1 MiB and 10 MiB exact round-trip test is implemented and passed in CI.
- [x] Cross-platform resource benchmark is implemented for 1 MiB, 10 MiB and 32 MiB.
- [x] Compiled resource benchmark was executed and archived on the release-benchmark runner.
- [x] Representative Linux CI result: ~258-267 MiB/s encode and ~296-328 MiB/s decode over 1-32 MiB structured inputs, with ~118 MB peak RSS at 32 MiB.
- [ ] Run the resource benchmark as evidence on Windows as well as Linux; Windows build/test is already green. This is desirable follow-up evidence but is not a blocker for the first experimental tag because cross-platform correctness is already validated.

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
- [x] Ubuntu build + `ctest` fully green.
- [x] Windows build + `ctest` fully green.
- [x] Release benchmark collector records environment, commit SHA, corpus SHA-256 manifest and raw outputs.
- [x] CI uploads release benchmark evidence as an artifact after tests pass.
- [x] Benchmark seed/reproducibility policy is documented.
- [x] Stochastic benchmark audit is documented in `benchmarks/randomness_audit.md`; audited random sources use explicit fixed seeds.
- [x] Canonical generated-corpus release seed is `0xB17A2026` and is recorded by the release runner.
- [x] Representative generated-corpus release benchmark artifact archived: ID `10000393296`.
- [x] Independent Canterbury artifact archived: ID `10000500422`.
- [x] README updated with architecture and explicit non-claims.
- [x] Generated-corpus release-candidate summary is archived in `benchmarks/release_candidate_results.md`.
- [x] Independent Canterbury summary is archived in `benchmarks/canterbury_results.md`.
- [x] Changelog exists with a v0.1.0 candidate entry.
- [x] Draft v0.1.0 release notes exist in `docs/release_notes_v0.1.0.md`.
- [ ] License reviewed and added for intended academic/commercial policy.
- [ ] Tag experimental `v0.1.0` only after the license is finalized.

## Measured negative result that must remain visible

On the deterministic 3,069,445-byte heterogeneous generated corpus, the current adaptive representation had an estimated total storage ratio of **3.5575x** after including its dictionary and 64-bit symbol trails. In the same run, fixed 8-byte dedup was ~1.3672x, the simple LZ77 baseline ~0.7287x, and actual gzip/bz2/lzma/zstd totals were all well below 1x.

The independent 2,810,784-byte Canterbury Corpus confirmed the same conclusion: the adaptive estimate was **3.283670x**, versus 0.675709x for the simple LZ77 baseline and 0.258985x / 0.193081x / 0.183721x / 0.175424x for gzip / bz2 / zstd / lzma respectively.

Therefore v0.1.0 must **not** be presented as a general-purpose compression improvement. The experiment remains interesting as a hierarchical relational memory system with stable IDs, online learning, consolidation, persistence and recovery properties.

## Current assessment

All technical gates required for the first public experimental candidate are now closed: cross-platform build/test is green, generated-corpus evidence is archived, and the independent Canterbury Corpus benchmark executed successfully with archived evidence.

The project is now technically ready for an experimental `v0.1.0`. The only blocking release-quality item is the **license policy**. Once the intended license is selected and added, the changelog can be dated and the `v0.1.0` tag/release can be created.
