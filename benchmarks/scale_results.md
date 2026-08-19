# Scale benchmark results

This benchmark compares adaptive memory against deterministic fixed-pair hierarchy at 1 KiB, 10 KiB, 100 KiB and 1 MiB.

The probe is always reconstructed losslessly. The adaptive dictionary is frozen at inference, while the fixed hierarchy is allowed to create relations on the probe.

Two simple costs are reported:

- **inference cost / byte** = trail + relations created while encoding the probe;
- **total model cost / byte** = trail + complete relation dictionary cost.

The accounting charges two symbols per relation. It is a structural comparison, not yet a physical storage-byte model.

## Statistical gate

The adaptive learner now promotes a relation only when all of the following hold:

- minimum raw frequency: `4`;
- minimum pair lift: `1.2`;
- minimum support in the current corpus: `0.001` (0.1%);
- maximum learned rules: `128`.

The support gate prevents large random corpora from producing a dictionary merely because accidental byte pairs eventually repeat.

## Results after gating

| case | bytes | adaptive rules | adaptive trail | fixed new relations | fixed trail | adaptive inference/byte | fixed inference/byte | adaptive model/byte | fixed model/byte |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| patterned | 1,024 | 12 | 77 | 23 | 4 | 0.075195 | **0.048828** | **0.098633** | 0.208984 |
| patterned | 10,240 | 29 | 697 | 213 | 40 | 0.068066 | **0.045508** | **0.073730** | 0.145703 |
| patterned | 102,400 | 34 | 6,893 | 1,999 | 400 | 0.067314 | **0.042949** | **0.067979** | 0.114746 |
| patterned | 1,048,576 | 34 | 70,571 | 1,995 | 4,096 | 0.067302 | **0.007711** | 0.067367 | **0.015541** |
| random | 1,024 | **0** | 1,024 | 997 | 4 | **1.000000** | 1.951172 | **1.000000** | 7.900391 |
| random | 10,240 | **0** | 10,240 | 8,955 | 40 | **1.000000** | 1.752930 | **1.000000** | 7.398828 |
| random | 102,400 | **0** | 102,400 | 54,183 | 400 | **1.000000** | 1.062168 | **1.000000** | 5.197070 |
| random | 1,048,576 | **0** | 1,048,576 | 520,133 | 4,096 | 1.000000 | **0.995981** | **1.000000** | 4.097406 |

## Timing snapshot (local Release build)

Representative measured timings in microseconds:

| case | bytes | adaptive train | adaptive encode | adaptive decode | fixed encode | fixed decode |
|---|---:|---:|---:|---:|---:|---:|
| patterned | 1,024 | 234 | 7 | 4 | 10 | 2 |
| patterned | 10,240 | 3,281 | 87 | 26 | 203 | 25 |
| patterned | 102,400 | 31,442 | 1,055 | 245 | 865 | 259 |
| patterned | 1,048,576 | 289,587 | 6,918 | 2,596 | 8,373 | 2,968 |
| random | 1,024 | 198 | 39 | 2 | 42 | 2 |
| random | 10,240 | 2,917 | 9 | 24 | 827 | 76 |
| random | 102,400 | 23,612 | 113 | 237 | 23,391 | 1,611 |
| random | 1,048,576 | 172,872 | 871 | 2,411 | 616,105 | 11,115 |

Timing is machine-dependent and should not be treated as a stable benchmark until repeated across machines and multiple runs.

## Findings

### 1. Random-data rejection now works

The most important result of this round is that the adaptive learner creates **zero rules at every tested random-data scale**, from 1 KiB through 1 MiB. The earlier frequency-only learner created many accidental rules on large random inputs.

This is a meaningful improvement: the memory can now decline to manufacture structure when pair recurrence is consistent with chance.

### 2. Adaptive and fixed optimize different things

The fixed hierarchy aggressively collapses every positional pair and therefore often produces a very short trail. The adaptive learner is intentionally more selective and pays for that selectivity with a longer trail.

On patterned data the adaptive model has substantially lower complete dictionary cost than the fixed model through 100 KiB. At 1 MiB, however, the fixed hierarchy wins under the current total-cost accounting because the synthetic motif repeats so strongly that its positional dictionary saturates.

This is an important negative result: **adaptive memory is not universally cheaper than fixed hierarchical pairing**.

### 3. Generalization remains the adaptive hypothesis

The main candidate advantage is not raw compression. It is a reusable, frozen structural vocabulary that can apply to shifted or related unseen data without continually creating new relations. Future benchmarks should therefore separate:

- storage/compression cost;
- dictionary growth;
- generalization under shifts and noise;
- structural similarity/search quality.

## Next experiment

The next benchmark should use mixed structured corpora rather than one highly repetitive motif. It should vary:

1. motif diversity;
2. phase/position shifts;
3. noise percentage;
4. training-corpus size;
5. unseen-pattern fraction.

The key question is whether the adaptive dictionary remains stable and reusable where the fixed positional hierarchy grows with each structural variation.
