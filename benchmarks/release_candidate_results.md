# Reproducible release-candidate benchmark summary

This document records the first fully green cross-platform CI run with archived benchmark evidence.

## Provenance

- Commit: `85e18855d03f7895c4d73412c33390a0881de478`
- GitHub Actions run: `34070708174`
- Release corpus seed: `0xB17A2026`
- Generated corpus files: 28
- Generated corpus bytes: 3,069,445
- Artifact: `bit-analyze-release-benchmarks`
- Artifact ID: `10000393296`
- Artifact ZIP SHA-256: `3cc73ce423e26827e99ebe963c27c03729a3bc94e472ebd8e77c7a2421f36494`
- Runner used for release benchmarks: Ubuntu 24.04 / GCC 13.3.0

Both Ubuntu and Windows build/test jobs completed successfully before the benchmark artifact was generated.

## Representation/storage comparison

The same generated corpus was used for all methods.

| Method | Total ratio vs input | Note |
|---|---:|---|
| bit.analyze adaptive estimate | 3.5575 | estimated dictionary + 64-bit symbol trails |
| fixed 8-byte dedup estimate | 1.3672 | unique block payload + 64-bit references |
| RLE baseline | 1.6262 | simple implementation |
| simple LZ77 baseline | 0.7287 | deterministic experimental implementation |
| gzip | 0.3059 | weighted total actual compressed bytes |
| bz2 | 0.2853 | weighted total actual compressed bytes |
| lzma | 0.2685 | weighted total actual compressed bytes |
| zstd | 0.2747 | weighted total actual compressed bytes |

Important interpretation: this result is **negative for using the current representation as a general-purpose compressor**. The adaptive engine has a much higher storage cost on this heterogeneous corpus. Any value proposition must therefore come from stable relational reuse, online learning, structural identity, persistence/recovery or downstream structural operations—not from compression ratio alone.

The adaptive representation did behave very differently across families: highly structured binary inputs produced short trails, while random data produced trails near the raw-symbol regime. That is useful structurally, but the 64-bit symbol representation dominates storage cost in the current prototype.

## Compiled encoder

For the 512 KiB compiled-encoder benchmark:

- rules: 7
- trie nodes: 19
- canonical trail symbols: 163,843
- compiled trail symbols: 163,843
- canonical encode: 4,919 us
- compiled encode: 1,551 us
- speedup: **3.172x**

The compiled path preserved trail length in this benchmark.

## Resource usage

Compiled trie encoder, deterministic structured data:

| Input | Encode | Decode | Encode throughput | Decode throughput | Peak RSS |
|---:|---:|---:|---:|---:|---:|
| 1 MiB | 0.003871 s | 0.003053 s | 258.36 MiB/s | 327.57 MiB/s | 18.1 MB |
| 10 MiB | 0.037388 s | 0.033750 s | 267.47 MiB/s | 296.29 MiB/s | 46.9 MB |
| 32 MiB | 0.121753 s | 0.099621 s | 262.83 MiB/s | 321.22 MiB/s | 118.1 MB |

These are CI-runner measurements, not hardware-independent performance guarantees.

## Protection serialization overhead

For the protected snapshot benchmark:

- input: 262,144 bytes
- rules: 8
- trail symbols: 21,846
- basic snapshot: 175,060 bytes
- protected snapshot: 188,949 bytes
- serialized protection overhead: **7.934%** relative to the basic snapshot

## Integrated adaptive protection

Integrated synthetic protected-memory benchmark:

- rules: 406
- trails: 24
- Light rules: 323
- Medium rules: 74
- Strong rules: 9
- parity cost: 134 symbols in this model

| Damage rate | Preserved usage | Mean unrecoverable groups |
|---:|---:|---:|
| 0.1% | 1.0000 | 0.0000 |
| 1% | 1.0000 | 0.0030 |
| 5% | 0.9987 | 0.2470 |
| 10% | 0.9914 | 1.6800 |

This benchmark is synthetic and weighted by the current rule-usage model; it should not be interpreted as a general storage reliability claim.

## Interleaving/corruption result

With 4096 symbols, 64 groups and correction capacity 2/group:

- 0.1% random damage: ~0.10% failure contiguous, ~0.08% interleaved.
- 0.1% burst damage: ~98.46% failure contiguous, **0%** in the tested interleaved layouts.
- 1% random damage: ~85.78% failure contiguous, ~84.44% interleaved.
- 1% burst damage: 100% failure contiguous, **0%** in the tested interleaved layouts.

This confirms the expected distinction: interleaving addresses burst locality; it does not materially solve high-rate independent random corruption.

## Protection quantile sweep

Under the benchmark's efficiency score, the strongest tested score was at:

- Medium quantile: `0.80`
- Strong quantile: `0.98`
- mean parity: `0.310`
- score: `3.219596`

This supports using 80/98 as the current experimental default, not as a universal optimum.

## Release interpretation

This run closes the prior cross-platform build/test and benchmark-execution blockers. It also establishes an important negative result: the current prototype is not storage-efficient relative to conventional compression on a heterogeneous corpus.

Before a public `v0.1.0` experimental tag, the remaining release-quality work should focus on:

1. one independent real-file corpus run, separate from the generated corpus;
2. final license policy;
3. changelog and release notes;
4. README/gate update with the measured results and their limitations.
