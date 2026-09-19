# StructuralFingerprint v1 — bounded streaming gate

Status: experimental design gate. This document does not change the canonical encoder or the v0 similarity weights.

## Evidence carried forward from v0

StructuralFingerprint v0 is derived only from `StructuralEvent` output and preserves chunk-boundary invariance in the mutation-gradient benchmark. Its combined similarity separates progressively larger deterministic mutations while avoiding the excessive sensitivity of exact ordered comparison.

The remaining architectural problem is memory growth: v0 stores exact maps for global relation frequency, per-region relation frequency, and relation-to-relation transitions. Their cardinality can grow with the observed stream. Therefore v0 must not be described as bounded-memory.

## v1 objective

Create an incremental accumulator whose storage is fixed by configuration rather than by input length or relation cardinality, while preserving enough of the v0 structural gradient to remain useful before Memoria.ia.

The canonical path remains:

`bytes -> StructuralStream -> StructuralExtractor -> HierarchicalMemory::encode() -> StructuralEvent`

v1 starts only after `StructuralEvent`; it must not create or reinterpret relation IDs.

## Proposed bounded representation

Use deterministic fixed-width count sketches for three independent channels:

1. global relation frequency;
2. regional relation frequency using a fixed number of regions;
3. adjacent relation transitions.

Each channel hashes its structural key into a fixed number of counters. The transition key is the ordered pair `(previous_relation_id, relation_id)`. Regional counters are partitioned by a fixed region index derived from byte position.

Configuration controls all storage:

- `frequency_buckets`
- `transition_buckets`
- `regions`
- counter width/type
- deterministic hash seed/version

No dynamic map keyed by observed relation IDs is permitted in the bounded accumulator.

## Required invariants

Before v1 can replace or complement v0, automated evidence must show:

- identical input produces similarity 1.0;
- output is independent of input chunk boundaries;
- accumulator memory is constant for a fixed configuration;
- one-byte mutation produces similarity below 1.0 without collapsing toward unrelated similarity;
- increasing mutation produces a useful separation curve on the frozen deterministic corpus;
- repetitive and high-diversity byte streams remain distinguishable;
- no MIME, extension, magic-byte, parser, codec, or format-specific rule is introduced;
- learned relation IDs remain owned exclusively by the canonical encoder.

## Benchmark gate

Add a bounded-vs-exact benchmark that feeds the same `StructuralEvent` sequence into v0 and candidate v1 accumulators. Record:

`mutation_fraction, mutated_bytes, v0_combined, v1_combined, absolute_error, chunk_invariant, storage_bytes`

Run at least these mutation levels:

`0, 1 byte, 0.1%, 1%, 5%, 10%, 25%, 50%`

Run multiple fixed capacities rather than tuning one capacity against the corpus. Suggested first sweep:

- 64 buckets
- 128 buckets
- 256 buckets
- 512 buckets
- 1024 buckets

The experiment should report the curve for every capacity. Do not select a capacity or alter similarity weights until the evidence is collected.

## Promotion criteria

StructuralFingerprint v1 remains experimental until the release benchmark demonstrates bounded storage and acceptable degradation relative to v0 across the frozen mutation and format-blind corpora. Exact thresholds should be established from measured evidence, not chosen in advance to make the candidate pass.
