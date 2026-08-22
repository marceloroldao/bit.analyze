# Publication Readiness Gate

This document defines the minimum bar for calling `bit.analyze` ready for a public experimental release.

The repository may be public before all items pass. "Publication-ready" here means ready to tag and describe as a coherent, reproducible experimental version rather than a work-in-progress snapshot.

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
- [ ] Run and archive a reproducible real-file corpus result on compiled C++.
- [ ] Compare against at least one conventional byte n-gram baseline on the same corpus.
- [ ] Compare storage overhead against a conventional compressor/deduplicator on the same corpus.

## Performance

- [x] Compiled trie encoder exists as an alternative to sequential rule application.
- [x] A benchmark exists for sequential vs compiled encode.
- [ ] Run compiled C++ benchmark on a reproducible machine and archive raw output.
- [ ] Confirm encode/decode correctness at >= 1 MiB and >= 10 MiB inputs.
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
- [ ] Execute integrated protected-memory benchmark on compiled C++ and archive the result.
- [ ] Measure total serialized overhead including parity, hashes and metadata.
- [ ] Test simultaneous rule + trail corruption using persisted serialized state.

## Persistence

- [ ] Define stable on-disk format for rules, trails, integrity metadata and parity.
- [ ] Implement save/load round-trip.
- [ ] Verify byte-for-byte reconstruction after process restart.
- [ ] Add format version field and compatibility policy.

## Reproducibility and release hygiene

- [ ] CI build on Windows and Linux.
- [ ] `ctest` fully green in CI.
- [ ] Seed all randomized benchmarks.
- [ ] Archive representative benchmark CSV/output files.
- [ ] README updated with current architecture and explicit non-claims.
- [ ] License reviewed for intended academic/commercial policy.
- [ ] Add changelog / release notes.
- [ ] Tag experimental `v0.1.0` only after the items above required for reproducibility pass.

## Current assessment

The project has moved beyond a toy proof of concept: it now has lossless hierarchical representation, online append-only learning, consolidation, a compiled encoder path, integrity checking, recovery primitives, interleaving, and adaptive protection policies.

It is **not yet ready for a scientific/public experimental release tag** because persistence, compiled end-to-end benchmark evidence, CI reproducibility, and same-corpus baseline comparisons are still missing.
