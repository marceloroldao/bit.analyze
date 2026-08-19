# Benchmarks

The benchmark suite should answer whether hierarchical relational memory provides value beyond simple representations.

## Required baselines

1. Raw bytes.
2. Fixed 2/4/8-byte blocks.
3. Byte bigrams / n-grams.
4. Adaptive hierarchical relations.
5. Conventional compression or deduplication baseline where applicable.

## Required datasets

Use held-out real files from multiple families. File names and extensions must not be used as features.

Suggested families:

- TXT
- PDF
- PNG
- JPG
- ZIP
- WAV
- MP3

Where classification-like experiments are run, split files into separate train and test sets.

## Required metrics

- exact reconstruction rate;
- original bytes;
- trail symbols;
- unique relation nodes;
- cross-file reuse rate;
- database/storage overhead;
- ingest MB/s;
- reconstruction MB/s;
- similarity within a family;
- similarity across families;
- held-out nearest-neighbor accuracy when used experimentally.

## Rule

A negative result is valid. The adaptive hierarchy should only be considered an improvement when it beats appropriate simple baselines on a clearly defined metric.
