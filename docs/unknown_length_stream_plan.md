# Unknown-length structural streams — experimental gate

## Problem

The bounded fingerprints currently use `total_bytes` to map each event into a normalized positional region. That is correct for finite files whose size is known before ingestion, but it is not a valid assumption for live audio, video, sensors, sockets, pipes, or indefinite streams.

## Constraint

Unknown-length support must not change the canonical structural path:

`bytes -> StructuralStream -> StructuralExtractor -> HierarchicalMemory::encode() -> StructuralEvent`

The canonical encoder remains the sole owner of learned relation IDs. No MIME-, format-, or sensor-specific logic is allowed.

## Experimental representation

For an unknown-length stream, keep the global frequency and transition sketches unchanged. Replace normalized whole-object regions with deterministic rolling epochs measured in byte offsets.

Configuration adds one independent quantity:

- `region_span_bytes`: fixed byte span represented by one temporal/positional region.

For event offset `p` and `R` regions:

- `epoch = p / region_span_bytes`
- `region = epoch % R`

When a region is reused for a new epoch, clear that region before adding new observations. Store one epoch tag per region so reuse is deterministic.

This creates a bounded rolling structural horizon of approximately `R * region_span_bytes` while global frequency and transitions continue to summarize the complete stream.

## Why not estimate total length

Continuously changing an estimated `total_bytes` would move the meaning of previously accumulated regions and make the fingerprint dependent on when observations arrived. The rolling-epoch design never remaps historical counters.

## Gate invariants

1. Memory is constant for fixed configuration and independent of stream duration.
2. Results are invariant to input chunk boundaries when the same `StructuralEvent` sequence is produced.
3. Identical event streams produce identical fingerprints.
4. Region reuse is deterministic and cannot mix counters from different epochs.
5. Global frequency and transition behavior remains compatible with the bounded accumulator.
6. Known-length mode is unchanged.
7. No format-specific rules enter the structural core.

## First benchmark

Use deterministic high-diversity and repetitive streams with chunks 17, 257, and 4096 bytes. Test 64 KiB, 1 MiB, and 16 MiB. Start with `regions=8` and `region_span_bytes=65536`; do not tune both capacity and span in the same experiment.

Report:

`input_kind,input_bytes,chunk,epochs_seen,region_reuses,chunk_invariant,storage_bytes`

A second experiment may compare rolling fingerprints at checkpoints to determine how much recent structural change is retained. Do not claim semantic novelty or temporal understanding from this gate alone.
