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
- [x] Deterministic generated multi-format corpus generator exists.
- [x] Same-corpus byte-bigram structural baseline exists.
- [x] Same-corpus fixed 8-byte dedup storage estimate exists.
- [x] Same-corpus RLE and simple deterministic LZ77 storage baselines exist.
- [ ] Run and archive the generated-corpus result on compiled C++.
- [ ] Run and archive a reproducible independent real-file corpus result on compiled C++.
- [ ] Add an external production compressor result (for example gzip/zstd) on the exact same corpus before making compression comparisons in release notes.

## Performance

- [x] Compiled trie encoder exists as an alternative to sequential rule application.
- [x] A benchmark exists for sequential vs compiled encode.
- [x] 1 MiB and 10 MiB exact round-trip test is implemented.
- [ ] Run compiled C++ performance benchmark on a reproducible machine and archive raw output.
- [ ] Execute the 1 MiB and 10 MiB round-trip test in CI.
- [ ] Record peak memory usage for large inputs.

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
- [x] Persisted simultaneous rule + trail corruption recovery test is implemented.
- [ ] Execute integrated protected-memory and serialized-overhead benchmarks on compiled C++ and archive results.
- [ ] Execute persisted corruption/recovery test in CI.

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
- [ ] CI runners actually execute the workflow. Current runs fail before step 1 with no runner assigned (`runner_id=0`), so this is presently an external Actions/infrastructure blocker rather than a demonstrated C++ failure.
- [ ] `ctest` fully green in CI.
- [ ] Audit randomized benchmarks and ensure every random source has an explicit fixed seed.
- [ ] Archive representative benchmark CSV/output files.
- [x] README updated with current architecture and explicit non-claims.
- [ ] License reviewed for intended academic/commercial policy.
- [ ] Add changelog / release notes.
- [ ] Tag experimental `v0.1.0` only after the required reproducibility gates pass.

## Current assessment

The project now has lossless hierarchical representation, append-only online learning, consolidation, a compiled encoder path, integrity checking, recovery primitives, interleaving, adaptive protection, protected persistence across restart, deterministic corpus generation and same-corpus conventional baselines.

It is **not yet ready for a `v0.1.0` experimental release tag**. The principal remaining blocker is reproducible compiled execution: the GitHub Actions jobs are not receiving runners. Once compiled runs are available, the next release decision should be based on archived same-corpus results, memory/performance measurements, and external compressor comparison rather than on unexecuted benchmark code.
