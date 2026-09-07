# bit.analyze v0.1.0 — Experimental Release

`bit.analyze` is an experimental hierarchical binary-memory engine that investigates whether recurring low-level binary structures can become reusable relations with stable IDs and participate in higher-order relations.

This first experimental release focuses on **structural memory**, not semantic understanding and not compression superiority.

## What is included

- adaptive hierarchical relation learning;
- stable relation IDs;
- exact reconstruction from structural trails;
- append-only online learning;
- higher-order consolidation without renumbering existing IDs;
- compiled trie encoding;
- rule and trail integrity detection;
- one- and two-damage recovery using parity / GF(256);
- reversible interleaving for burst-damage resistance;
- adaptive Light, Medium and Strong protection profiles;
- persistent basic and protected snapshots;
- snapshot corruption detection;
- deterministic benchmark and evidence pipeline;
- Linux and Windows CI validation.

## Reproducibility

The release candidate builds and passes `ctest` on both Ubuntu and Windows in GitHub Actions. Benchmarks use explicit deterministic seeds where randomness is involved, and evidence artifacts record the corpus and execution environment.

An independent Canterbury Corpus run processed all 11 expected files (2,810,784 bytes) successfully.

## Measured results

Representative compiled trie throughput on the generated structured benchmark was approximately 258–267 MiB/s encode and 296–328 MiB/s decode for 1–32 MiB inputs on the Linux CI runner. At 32 MiB, peak RSS was approximately 118 MB.

The protected snapshot benchmark measured 7.934% serialized overhead relative to the basic snapshot in that scenario.

The independent Canterbury benchmark produced the following total storage ratios:

| Method | Ratio |
|---|---:|
| bit.analyze adaptive estimate | **3.283670x** |
| fixed 8-byte dedup estimate | 1.572948x |
| RLE | 1.532073x |
| simple LZ77 | 0.675709x |
| gzip | 0.258985x |
| bz2 | 0.193081x |
| zstd | 0.183721x |
| lzma | **0.175424x** |

## Important negative result

The current `bit.analyze` representation is **not a competitive general-purpose compressor**. This is an explicit result of both the deterministic generated corpus and the independent Canterbury Corpus evaluation.

The research question that remains open is whether stable relational reuse, online structural learning, persistence, consolidation and targeted recovery provide useful memory-system properties independent of compression ratio.

## Scope and non-claims

This release does not claim:

- semantic understanding;
- cognition or consciousness;
- general-purpose compression superiority;
- universal corruption tolerance;
- production-readiness.

It is a reproducible experimental baseline from which those structural questions can be tested more rigorously.

## Next directions

- reduce trail-symbol representation overhead;
- test long-lived incremental workloads and cross-file reuse;
- study relation lifetime and pruning without breaking stable IDs;
- evaluate structural retrieval and partial reconstruction;
- broaden independent corpora and hardware measurements;
- investigate adaptive protection policies under realistic storage-failure models.
