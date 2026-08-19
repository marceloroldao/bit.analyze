# Adaptive capacity sweep — preliminary results

The goal is to distinguish true structural saturation from a hard `max_rules` ceiling.

A reduced 8 KiB/file replica of the cumulative stream was used for the quick sweep because the current reference learner rescans the full working corpus for every promoted rule. The 32 KiB C++ benchmark remains available in `capacity_sweep.cpp` for native execution.

## Results

| step | sample | rules @64 | rules @128 | rules @256 |
|---:|---|---:|---:|---:|
| 1 | A0 | 10 | 10 | 10 |
| 2 | A3 | 10 | 10 | 10 |
| 3 | A7_noise | 18 | 18 | 18 |
| 4 | B0 | 37 | 37 | 37 |
| 5 | B5 | 34 | 34 | 34 |
| 6 | B9_noise | 57 | 57 | 57 |
| 7 | C0 | **64 (cap)** | 75 | 75 |
| 8 | C4 | **64 (cap)** | 76 | 76 |
| 9 | C11_noise | **64 (cap)** | 82 | 82 |
| 10 | A1_again | **64 (cap)** | 83 | 83 |
| 11 | B2_again | **64 (cap)** | 83 | 83 |
| 12 | C6_again | **64 (cap)** | 84 | 84 |

## Interpretation

- Capacity 64 is insufficient and clips the vocabulary from step 7 onward.
- Capacities 128 and 256 converge to the same 84-rule vocabulary on this corpus.
- Therefore the plateau at 84 rules is structural under the current lift/support gate, not an artificial capacity limit.
- Returning to already-seen families adds very few or zero rules, which is consistent with vocabulary reuse.

## Performance finding

The current adaptive trainer rescans the full corpus for each candidate rule. Its rough cost is proportional to `number_of_rules × corpus_size`, which becomes expensive during large capacity sweeps. This is an implementation bottleneck, not yet evidence against the representation itself.

## Next test in sequence

Implement and evaluate **true online update**: ingest one sample, update the vocabulary without retraining on all previous raw samples, then measure:

1. new rules per sample;
2. retained reconstruction of old samples;
3. probe representation of old and new families;
4. dictionary growth;
5. sensitivity to sample order.
