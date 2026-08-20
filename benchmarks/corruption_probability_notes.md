# Corruption probability study (preliminary)

Configuration used for the initial internal replication of `corruption_probability_benchmark.cpp`:

- 4096 trail symbols
- 64 protection groups
- 64 symbols per group
- correction capacity: 2 corrupted symbols per group
- 5000 random trials per rate
- deterministic seed

## Preliminary results

| Damage rate | Errors | Random contiguous fail | Random interleaved fail | Burst contiguous fail | Burst interleaved fail |
|---:|---:|---:|---:|---:|---:|
| 0.01% | 1 | 0.0000 | 0.0000 | 0.0000 | 0.0000 |
| 0.1% | 4 | 0.0006 | 0.0016 | 0.9846 | 0.0000 |
| 1% | 41 | 0.8488 | 0.8520 | 1.0000 | 0.0000 |

Interpretation:

- Interleaving strongly improves localized burst tolerance because consecutive logical symbols are distributed across different protection groups.
- Interleaving does not materially improve uniformly random corruption; the occupancy distribution among groups remains similar.
- With only two recoverable symbols per group, 1% uniformly random damage is beyond the reliable operating region for this configuration.

## Capacity sweep at 1% random damage

A separate internal Monte Carlo sweep (3000 trials, same 4096/64 layout) estimated the probability that at least one group exceeds the correction capacity:

| Capacity per group | Failure probability at 1% random damage |
|---:|---:|
| 2 | ~0.856 |
| 3 | ~0.194 |
| 4 | ~0.027 |
| 5 | ~0.001 |
| 6 | ~0.0003 |
| 8 | no failures observed in 3000 trials |

These are preliminary probabilistic results, not a production reliability guarantee. They motivate a configurable protection profile instead of hard-coding two parities for every workload.

## Next experiment

Benchmark correction capacity versus redundancy cost, targeting several protection profiles:

- light: 2-symbol correction per group
- medium: 4-symbol correction per group
- strong: 6-symbol correction per group

The goal is to determine a Pareto frontier between storage overhead and recoverability for random and burst corruption.
