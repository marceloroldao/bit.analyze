# Changelog

All notable changes to `bit.analyze` are documented here.

## [0.1.0] - Unreleased

First public experimental candidate.

### Added

- Fixed and adaptive hierarchical binary-relation memory.
- Stable relation IDs and exact lossless trail reconstruction.
- Append-only online learning and multi-cycle consolidation.
- Compiled trie encoder.
- Rule and trail integrity manifests.
- Single- and double-damage recovery using parity and independent GF(256) equations.
- Reversible interleaving for improved burst-damage tolerance.
- Adaptive Light / Medium / Strong protection policies for rules and trails.
- Basic v1 snapshots and protected v2 snapshots.
- Whole-snapshot corruption detection.
- Cross-platform CMake build and test workflow for Linux and Windows.
- Deterministic generated benchmark corpus and reproducible release evidence collector.
- Independent Canterbury Corpus benchmark.
- Same-corpus baselines for fixed 8-byte dedup, byte bigrams, RLE and simple LZ77.
- External gzip, bz2, lzma and zstd comparison.
- Resource, protection-overhead, corruption, interleaving and integrated-recovery benchmarks.

### Validated

- Linux and Windows build + `ctest` pass in GitHub Actions.
- Exact 1 MiB and 10 MiB round trips pass in CI.
- Persisted simultaneous rule/trail corruption recovery passes in CI.
- Protected snapshot overhead measured at 7.934% in the representative benchmark scenario.
- Independent Canterbury Corpus run completed successfully on 11 files / 2,810,784 bytes.

### Known limitations

- This release is experimental research software.
- The current representation is not a competitive general-purpose compressor.
- On Canterbury, the adaptive storage estimate is 3.283670x the original input; gzip/bz2/zstd/lzma all produce ratios well below 1x.
- No semantic-understanding or cognition claim is made.
- Protection benchmarks use explicit experimental corruption models and are not universal reliability guarantees.
- Current trail storage uses 64-bit symbol IDs and carries substantial representation overhead.

### Release blockers

- Finalize and add the intended license before tagging `v0.1.0`.
- Convert this Unreleased entry into the final release date and prepare the GitHub release notes.
