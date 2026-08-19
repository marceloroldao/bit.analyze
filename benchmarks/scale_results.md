# Scale benchmark results

This benchmark compares the adaptive memory against the deterministic fixed-pair hierarchy at 1 KiB, 10 KiB, 100 KiB and 1 MiB.

The probe is always losslessly reconstructed. The adaptive dictionary is frozen at inference, while the fixed hierarchy is allowed to create relations on the probe. Therefore two costs are reported:

- **inference cost / byte** = trail + relations created while encoding the probe;
- **total model cost / byte** = trail + complete relation dictionary cost.

The current simple accounting charges two symbols per relation. This is intentionally conservative and is not a storage-byte model.

## Results

| case | bytes | adaptive rules | adaptive trail | fixed new relations | fixed trail | adaptive inference/byte | fixed inference/byte | adaptive model/byte | fixed model/byte |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| patterned | 1,024 | 17 | 23 | 23 | 4 | 0.022461 | 0.048828 | 0.055664 | 0.208984 |
| patterned | 10,240 | 41 | 191 | 213 | 40 | 0.018652 | 0.045508 | 0.026660 | 0.145703 |
| patterned | 102,400 | 97 | 1,560 | 1,999 | 400 | 0.015234 | 0.042949 | 0.017129 | 0.114746 |
| patterned | 1,048,576 | 128 | 17,086 | 1,995 | 4,096 | 0.016294 | **0.007711** | 0.016539 | **0.015541** |
| random | 1,024 | 0 | 1,024 | 997 | 4 | **1.000000** | 1.951172 | **1.000000** | 7.900391 |
| random | 10,240 | 97 | 10,227 | 8,955 | 40 | **0.998730** | 1.752930 | **1.017676** | 7.398828 |
| random | 102,400 | 128 | 102,218 | 54,183 | 400 | **0.998223** | 1.062168 | **1.000723** | 5.197070 |
| random | 1,048,576 | 128 | 1,046,548 | 520,133 | 4,096 | 0.998066 | **0.995981** | **0.998310** | 4.097406 |

## Interpretation

### Structured data

At 1 KiB through 100 KiB the adaptive representation has a lower inference and total-model cost than the fixed hierarchy. It reuses learned relations across shifted/noisy probes instead of creating a large number of position-specific relations.

At 1 MiB the fixed hierarchy becomes slightly cheaper under the current inference-cost accounting because its learned dictionary has saturated on the highly repetitive synthetic motif. This is an important negative result: adaptive pairing is not universally superior even on structured input.

### Random data

The 1 KiB random corpus correctly produces zero adaptive rules. At larger scales, however, the current frequency-only learner begins creating rules from coincidental pair repetitions. The adaptive trail barely shrinks (~0.2%), so these rules are not useful structure.

This exposes the next required change: **frequency alone is insufficient as a rule-selection criterion**. A pair should be promoted only when its occurrence is meaningfully above the frequency expected from its marginals (or when it produces a positive description-length gain).

## Next experiment

Add a statistical/MDL gate to adaptive rule creation and rerun this exact benchmark. The desired behavior is:

1. preserve the large gains on structured/shifted data;
2. drive learned-rule count on random data back toward zero;
3. keep exact reconstruction;
4. compare inference and total-model costs under the same accounting.
