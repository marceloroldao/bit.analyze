# Real-file scale benchmark methodology

This benchmark measures whether the append-only adaptive memory remains useful as the amount of real binary data grows.

## Input policy

- Input files are supplied explicitly on the command line.
- The first 512 bytes of each file are skipped to reduce trivial format-header effects.
- The benchmark evaluates internal windows of 2 KiB, 8 KiB, 32 KiB, 128 KiB and 512 KiB.
- Files too short to provide at least half of the requested window are skipped for that size.

## Learning policy

For each window size a fresh `AdaptiveMemory` is created.

Each file is learned online with:

- at most 8 new local rules per file;
- minimum frequency = 4;
- minimum lift = 1.5;
- minimum support = 0.001.

After all files are ingested, the current encoded trails are consolidated with at most 64 append-only meta-rules.

No existing rule ID is renamed, deleted or rewritten.

## Metrics

The benchmark emits CSV columns:

- `window_bytes`
- `files`
- `total_input_bytes`
- `rules_before`
- `meta_added`
- `rules_after`
- `total_trail_symbols`
- `mean_cost_per_byte`
- `rules_per_kib`
- `learn_us`
- `consolidate_us`
- `encode_us`
- `decode_us`
- `old_trails_ok`

`mean_cost_per_byte` is trail symbols divided by input bytes. It is a structural representation metric, not a compressed-byte ratio.

## Required invariants

A run is invalid if any of the following occurs:

1. decoded bytes differ from the input window;
2. a trail stored before consolidation no longer decodes after consolidation;
3. a rule ID is reused for a different relation.

## Interpretation criteria

The experiment is considered promising if, as window size grows:

1. `mean_cost_per_byte` stays flat or decreases on structured data;
2. `rules_per_kib` decreases or approaches a plateau;
3. random/pre-compressed data remains near one trail symbol per byte rather than generating large artificial vocabularies;
4. encode/decode time grows approximately linearly enough to be practical after implementation optimizations;
5. all old trails remain valid.

A negative result must be reported if dictionary growth remains approximately linear with data volume, or if consolidation cost grows superlinearly enough to dominate ingestion.

## Example

```bash
cmake -S . -B build
cmake --build build --config Release
./build/bit_analyze_real_scale_benchmark file1.pdf file2.png file3.zip file4.txt
```

On Windows with a multi-config generator, the executable may be under `build/Release/`.
