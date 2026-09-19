# Universal Structural Stream — experimental direction

## Goal

Extend `bit.analyze` from finite binary-file experiments toward a modality-agnostic structural front end that can consume arbitrary byte streams and emit reusable structural observations for downstream systems such as `memoria.ia`.

This document defines a research contract, not a claim of semantic perception.

## Principle

The core MUST remain unaware of file type, MIME type, codec, sensor model, or semantic label.

```text
arbitrary bytes / stream
        ↓
   bit.analyze core
        ↓
relations / hierarchy / trail
        ↓
 structural observation
```

HTML, text, images, audio, video, executable files, telemetry, ADC, IMU, microphones and cameras are all input families. Format-specific decoders MAY exist outside the core as optional assisted adapters, but the RAW path is mandatory.

## Two experimental modes

### RAW

The engine receives bytes only. File extension, MIME type and semantic labels are withheld.

Purpose: test whether reusable structure emerges from the data itself.

### ASSISTED

An external adapter may decode a representation (for example WAV→PCM, PNG→pixels, HTML→DOM/text) before feeding the same structural engine.

Purpose: compare structure discovered from raw encoding against structure discovered from decoded physical/logical content.

Adapters MUST NOT become dependencies of the core.

## Required capabilities

### 1. Incremental streaming

The engine should accept bounded chunks without requiring the complete source in memory. State must be bounded/configurable so continuous sources can be tested.

Target abstraction:

```text
push(bytes, source_id, sequence)
        ↓
updated structural state
```

Chunk boundaries must not silently change canonical results where equivalence is expected; boundary-sensitive behavior must be measured explicitly.

### 2. Structural signature

A window/trail needs a deterministic structural descriptor suitable for comparison. A cryptographic hash alone is insufficient because it destroys proximity.

The first implementation should expose the descriptor components rather than prematurely claiming a universal metric.

### 3. Structural similarity and novelty

Given observations A and B, the experimental layer should produce measurable similarity/distance from shared relations, hierarchy and trails.

Novelty must be defined relative to previously observed structures, not semantic labels.

### 4. Universal output contract

Provisional contract:

```text
StructuralEvent
  version
  source_id
  sequence
  byte_offset
  byte_length
  structure_id        # when a stable identity exists
  trail
  relation_ids
  signature
  similarity          # optional, relative to a reference/index
  novelty             # optional, relative to learned history
  resolution
  metadata             # opaque provenance supplied by caller; core does not interpret it
```

The contract must be serializable and versioned before integration with `memoria.ia`.

## Separation of responsibilities

`bit.analyze`:
- discovers/reuses binary structural relations;
- maintains stable structural identities;
- emits structural events;
- measures structural similarity/novelty experimentally.

`memoria.ia`:
- associates observations with episodes, context, time and provenance;
- accumulates competing evidence;
- learns downstream meaning/experience;
- must not require raw high-rate sensor/file streams to be persisted.

A structural ID MUST NOT be presented as semantic understanding.

## First falsifiable benchmark

Build a mixed corpus with labels hidden from the engine:

- TXT / structured text;
- HTML;
- JSON or CSV;
- PNG / JPEG;
- WAV;
- compressed media where practical;
- executable/binary data;
- pseudorandom controls;
- synthetic sensor streams.

Run RAW mode first.

Measure:

1. deterministic reconstruction invariants;
2. stable-ID preservation under append-only learning;
3. relation reuse across samples;
4. similarity separation between same-family and different-family samples;
5. clustering quality only after labels are revealed (ARI/NMI or equivalent);
6. novelty discrimination against held-out samples and random controls;
7. throughput, peak RAM and bytes touched per input byte;
8. dictionary/relation growth;
9. representation/protection overhead.

Then repeat selected families in ASSISTED mode and compare.

## Integration gate for memoria.ia

Do not integrate merely because `StructuralEvent` exists.

The adapter becomes justified when the experiment demonstrates at least:

- deterministic event generation;
- stable identities across incremental learning;
- bounded streaming memory;
- useful same-family/recurrence separation above simple baselines;
- provenance fields sufficient to trace an event to its source interval.

After that gate, implement a thin `bit.analyze → memoria.ia` adapter. The first integration test should verify that a repeated physical/digital pattern can retrieve a prior episode without assigning semantic labels inside `bit.analyze`.

## Non-goals

This experiment does not claim:

- universal file-type recognition;
- semantic understanding;
- neural-network replacement;
- general-purpose compression superiority;
- modality-independent perception.

Those remain hypotheses to test.
