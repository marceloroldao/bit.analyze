# Preliminary heterogeneous real-file experiment

A small real-file corpus was sampled from the local environment. To avoid trivial format-header recognition, the first 512 bytes of each file were skipped and only an internal 2 KiB window was analyzed.

The corpus contained 16 distinct files across 8 extensions: TXT, PY, PDF, PNG, JPG, ZIP, JSON and shared-library binaries (SO). Two files per extension were processed in two online-learning cycles, with append-only consolidation after each cycle.

## Aggregate results

| cycle | files seen | online rules added in cycle | rules before consolidation | meta-rules added | rules after consolidation | mean cost before | mean cost after | old trails valid |
|---:|---:|---:|---:|---:|---:|---:|---:|:---:|
| 1 | 8 | 27 | 27 | 16 | 43 | 0.916748 | 0.775818 | yes |
| 2 | 16 | 32 | 75 | 16 | 91 | 0.806702 | 0.773346 | yes |

All sampled windows were reconstructed exactly.

## Final per-extension representation cost

These numbers are only descriptive for this tiny corpus and must not be treated as a benchmark of file formats.

| extension | symbols/byte after final consolidation |
|---|---:|
| TXT | 0.5718 |
| PY | 0.9631 |
| PDF | 0.8604 |
| PNG | 0.9919 |
| JPG | 0.5901 |
| ZIP | 0.9910 |
| JSON | 0.9436 |
| SO | 0.2749 |

Compressed/high-entropy samples (PNG, ZIP) showed almost no reduction, which is desirable: the learner did not manufacture a strong hierarchy where little reusable local structure was available. Some text/binary samples showed substantially more reuse.

## Limitations

- only 16 files;
- only a 2 KiB internal window per file;
- files came from the local software/documentation environment, not an independent public benchmark corpus;
- Python reference execution was used to reproduce the current C++ learning/consolidation logic because the local checkout environment could not fetch GitHub directly;
- the C++ `real_file_benchmark` harness is included for reproducible local runs on arbitrary files.

## Next

1. benchmark time and memory as real-file windows scale from KiB to MiB;
2. optimize sequential rule application if necessary;
3. then add integrity, corruption localization and recovery experiments.
