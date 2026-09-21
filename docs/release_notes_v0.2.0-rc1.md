# bit.analyze v0.2.0-rc1 — Release Candidate

`bit.analyze` v0.2.0-rc1 is a pre-release candidate that extends the v0.1.0 structural-memory baseline with two experimentally validated layers: a bounded universal structural byte stream and a pre-semantic temporal multimodal association model.

The release remains research software. It does not claim semantic understanding, causal inference, cognition, or general-purpose compression superiority.

## Main additions

- modality-agnostic `StructuralEvent` / `StructuralStream` processing;
- rolling and bounded structural fingerprints for incremental streams;
- deterministic gates for recycling, renormalization, memory bounds, out-of-order rejection and fragmentation equivalence;
- temporal structural baselines and recurrence/change experiments;
- immutable `RealitySlice` records with timestamps, modalities and provenance;
- temporal association based on proximity, repetition, direction and forgetting;
- directional coverage and reliability metrics;
- adversarial structural-closure ranking for correlated distractors.

## Pre-release validation

The candidate was validated in GitHub Actions on the consolidated code path.

- Linux build/test: pass
- Windows build/test: pass
- independent Canterbury corpus job: pass
- release benchmark job: pass
- 100,000-slice hidden-structure gate: pass
- hidden structural-pair recovery: 6/6
- 100,000-slice adversarial gate: pass
- adversarial expected relations: 9/9
- adversarial evidence margin: 3.133x
- required adversarial margin: 2x

The 2x acceptance threshold was retained; it was not relaxed to make the candidate pass.

## Important interpretation boundary

The temporal multimodal experiment identifies recurring structural relations across time and modalities. Directional reliability measures how consistently an observed temporal relation recurs. Structural closure rewards links supported by strong shared-neighbor relations.

None of those quantities is a causal probability or semantic interpretation.

## Compatibility

Snapshot formats and newly introduced experimental structures are not yet declared a stable public compatibility contract. This is one reason the version is published as `rc1` rather than a final `v0.2.0`.

## License

Distributed under the Resolutive Research and Non-Commercial License (RRNCL) v1.0. No commercial rights are granted.

## Previous archival release

The previous public release, v0.1.0, is archived at DOI 10.5281/zenodo.22568307. A distinct archival identifier for v0.2.0-rc1 should be recorded after the new release is deposited.
