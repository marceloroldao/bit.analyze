# Incremental benchmark — preliminary results

This experiment presents files cumulatively and retrains the adaptive vocabulary over everything seen so far. It measures dictionary growth and the cost of representing a shifted probe from the current family.

Important: this is **cumulative retraining**, not yet a true online-update algorithm.

## Preliminary curve

| step | sample | adaptive rules | rule growth | fixed relations | fixed growth | adaptive probe cost/byte | fixed probe cost/byte |
|---:|---|---:|---:|---:|---:|---:|---:|
| 1 | A0 | 10 | +10 | 14 | +14 | 0.062592 | 0.004944 |
| 2 | A3 | 10 | +0 | 31 | +17 | 0.062622 | 0.004578 |
| 3 | A7_noise | 33 | +23 | 1,054 | +1,023 | 0.062531 | 0.003906 |
| 4 | B0 | 38 | +5 | 1,067 | +13 | 0.250031 | 0.004578 |
| 5 | B5 | 34 | +0 | 1,084 | +17 | 0.250061 | 0.004578 |
| 6 | B9_noise | 118 | +84 | 1,755 | +671 | 0.000336 | 0.004333 |
| 7 | C0 | 128 | +10 | 1,770 | +15 | 0.000641 | 0.005005 |
| 8 | C4 | 128 | +0 | 1,777 | +7 | 0.000061 | 0.003906 |
| 9 | C11_noise | 128 | +0 | 2,307 | +530 | 0.000336 | 0.004578 |
| 10 | A1_again | 128 | +0 | 2,318 | +11 | 0.000305 | 0.004578 |
| 11 | B2_again | 128 | +0 | 2,329 | +11 | 0.000366 | 0.004211 |
| 12 | C6_again | 128 | +0 | 2,340 | +11 | 0.000275 | 0.003906 |

## Interpretation

Two different effects are visible.

1. When the same family returns with a shifted phase, adaptive rule growth can be zero while the fixed hierarchy still creates new position-specific relations.
2. Sparse noise creates large growth in both models, but the fixed hierarchy is especially sensitive because changed positions generate many new relation paths.

However, the adaptive vocabulary reaches the configured ceiling of 128 rules at step 7. Therefore the apparent plateau after step 7 cannot yet be interpreted as true memory saturation.

## Required next test

Sweep `max_rules` across 64, 128, 256 and 512 on the same cumulative stream. We need to distinguish:

- true structural saturation (additional capacity remains unused), from
- artificial saturation (the learner would keep growing if permitted).

Only after that should a true online-update algorithm be evaluated.
