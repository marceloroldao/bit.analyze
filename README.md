# bit.analyze

Experimental hierarchical binary-memory engine.

## Objective

`bit.analyze` investigates whether raw binary data can be represented as a hierarchy of reusable relations, where recurring low-level structures receive stable IDs and can participate in higher-order relations.

The project is intentionally separate from `memoria.ia`.

## Core hypothesis

Instead of treating a file as an opaque blob or only as fixed-size chunks, the engine builds reusable structural relations:

```text
raw bytes
   ↓
base symbols
   ↓
first-order relations
   ↓
higher-order relations
   ↓
structural trails
```

A higher layer may reference IDs from a lower layer instead of storing the same raw information again.

## Current experimental goals

1. Lossless ingestion and reconstruction of arbitrary files.
2. Deterministic file trails.
3. Hierarchical relation discovery.
4. Cross-file structural reuse.
5. Measurement of storage overhead and reuse.
6. Comparison against fixed blocks, n-grams and conventional compression/deduplication baselines.
7. Test whether structural representations preserve enough information to cluster or classify unseen data without using file names or extensions.

## Initial architecture

The first implementation focuses on a minimal hierarchy:

- Layer 0: raw byte symbols.
- Layer 1: relations between lower-layer symbols.
- Higher layers: relations between already-created relation IDs.

The first milestone deliberately avoids claiming semantic understanding. The initial question is narrower: **does hierarchical relational memory discover useful structure in raw data?**

## Repository layout

```text
src/          C++ core
include/      public headers
tests/        lossless reconstruction and invariants
benchmarks/   comparative experiments
docs/         architecture and experimental notes
examples/     usage examples
```

## Status

Early experimental research prototype.

## Principles

- lossless reconstruction is mandatory;
- negative results are valid results;
- benchmarks must include strong simple baselines;
- structural similarity must not be confused with semantics;
- experiments should be reproducible.
