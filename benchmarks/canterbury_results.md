# Independent Canterbury Corpus benchmark

This document records the first independent public-corpus benchmark used as release evidence for `bit.analyze`.

## Provenance

- Commit: `f7a9a2b8324997a3f8f847be2dfe6c47df2cb7e9`
- GitHub Actions run: `34071048369`
- Job: `independent-corpus`
- Corpus: Canterbury Corpus (`cantrbry.zip`)
- Files: 11
- Input bytes: 2,810,784
- Downloaded archive SHA-256: `c44b686dfc137e74aba4db0540e5d6568cb09e270ba8f8411d2f9df24f39a1a6`
- Evidence artifact: `bit-analyze-canterbury-benchmark`
- Artifact ID: `10000500422`
- Artifact ZIP SHA-256: `839ce7c3ea6148a6c1db083f26dd82bb1ccdbe991372aac50ea841eb4feb32ae`
- Runner: Ubuntu 24.04 / GCC 13.3.0

The workflow validated the expected names and byte sizes of all 11 corpus files before running the benchmark.

## Storage / representation comparison

All ratios below are total bytes divided by the original 2,810,784-byte corpus size.

| Method | Bytes | Ratio |
|---|---:|---:|
| bit.analyze adaptive estimate | 9,229,688 | **3.283670x** |
| fixed 8-byte dedup estimate | 4,421,216 | 1.572948x |
| RLE baseline | 4,306,326 | 1.532073x |
| simple deterministic LZ77 | 1,899,272 | 0.675709x |
| gzip | 727,952 | 0.258985x |
| bz2 | 542,710 | 0.193081x |
| zstd | 516,401 | 0.183721x |
| lzma | 493,080 | **0.175424x** |

Additional adaptive statistics:

- learned rules: 181
- estimated adaptive dictionary: 5,792 bytes
- adaptive 64-bit symbol trails: 9,223,896 bytes
- byte-bigram vocabulary structural baseline: 19,302 bytes (reported as vocabulary size, not a complete compressed representation)

## Interpretation

The independent corpus confirms the negative compression result already seen on the generated heterogeneous corpus. The current `bit.analyze` representation is **not competitive as a general-purpose compressor**. Its estimated representation is 3.283670x the original corpus, while conventional compressors reduce the corpus substantially.

This result is important and should remain visible in release documentation. The experimental value proposition is instead the combination of stable relational IDs, hierarchical reuse, append-only online learning, consolidation, exact reconstruction, persisted integrity metadata and targeted recovery.

The dominant storage cost in this prototype remains the use of 64-bit IDs for every trail symbol. Reducing that cost is a possible future optimization, but it must not be presented as an achieved compression improvement in v0.1.0.

## Release consequence

This run closes the independent-corpus technical gate for the experimental v0.1.0 candidate. It does not establish superiority over compression or semantic understanding; it establishes that the implementation can be evaluated reproducibly on an external corpus and that the negative storage result persists outside the project's generated data.
