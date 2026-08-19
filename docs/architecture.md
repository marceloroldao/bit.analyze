# Architecture

## Scope

`bit.analyze` is an experimental binary-memory engine. It is not currently an AI system and does not claim semantic understanding.

The core question is whether reusable hierarchical relations over raw bytes provide measurable advantages in representation, structural similarity, prediction, or retrieval.

## Model

### Layer 0: base symbols

Each byte value is a base symbol with a stable ID in the range 0-255.

### Layer 1+: relations

A relation node is defined by two child symbol IDs:

```text
R = (left, right)
```

If the same relation already exists, its ID is reused. A relation ID can itself be used as a child of a higher-order relation.

```text
bytes:          A B A B A B A B
layer 1:        R1  R1  R1  R1       where R1=(A,B)
layer 2:        R2      R2            where R2=(R1,R1)
layer 3:        R3                     where R3=(R2,R2)
```

The structure remains lossless because every relation is recursively expandable to base symbols.

## Current prototype

The initial C++ core uses deterministic adjacent pairing. This is intentionally a baseline implementation, not the final adaptive algorithm.

The next research step is to compare:

1. deterministic fixed pairing;
2. fixed byte blocks;
3. byte n-grams;
4. adaptive pair selection based on recurrence;
5. conventional compression/deduplication baselines.

## Metrics

- exact reconstruction;
- number of unique relation nodes;
- trail length;
- reference counts;
- cross-file relation reuse;
- encoded representation overhead;
- ingest throughput;
- reconstruction throughput;
- same-family vs cross-family structural similarity;
- predictive utility on unseen sequences.

## Scientific caution

Structural recurrence is not equivalent to semantic meaning. Any claim of higher-level understanding must be supported by experiments on held-out real data and comparisons with simple baselines.
