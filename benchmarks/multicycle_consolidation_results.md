# Multi-cycle online learning + consolidation

This experiment alternates online learning and append-only consolidation over four cycles. Each cycle introduces 8 new synthetic structural families (4 KiB each), then consolidates the encoded trails seen so far.

All rule IDs are append-only. Trails captured before later learning/consolidation cycles must remain exactly decodable.

## Results

| cycle | seen families | rules before consolidation | meta-rules added | rules after consolidation | mean probe cost (symbols/byte) | old trails valid |
|---:|---:|---:|---:|---:|---:|:---:|
| 1 | 8 | 104 | 48 | 152 | 0.003265 | yes |
| 2 | 16 | 255 | 48 | 303 | 0.003174 | yes |
| 3 | 24 | 407 | 48 | 455 | 0.003255 | yes |
| 4 | 32 | 559 | 48 | 607 | 0.003258 | yes |

## Interpretation

The representation cost remained nearly flat as the number of structural families increased from 8 to 32. This is a positive result for the two-speed design (fast append-only online learning followed by append-only consolidation).

The strongest invariant also held: every trail stored in an earlier cycle remained losslessly decodable after all later learning and consolidation cycles.

However, the dictionary itself continued to grow substantially (152 -> 607 rules). The current design therefore avoids catastrophic forgetting by preserving old rule IDs, but does not yet solve long-run vocabulary growth.

## Next experiment

Move from synthetic motifs to heterogeneous real files. Measure:

1. lossless reconstruction;
2. rules added per incoming file;
3. representation cost before/after consolidation;
4. cross-file rule reuse;
5. dictionary growth by file family;
6. sensitivity to compressed/high-entropy formats.

After that, add integrity/corruption detection and recovery tests.
