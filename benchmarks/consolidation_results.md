# Append-only consolidation results

This experiment evaluates whether an online append-only vocabulary can be consolidated by adding higher-order relations over already encoded trails, without renumbering or deleting existing rules.

## Setup

- 32 structural families
- 4096 bytes per training sample
- shifted probes from the same families
- online learning: up to 16 new rules per sample
- consolidation: up to 512 append-only meta-rules
- statistical gate: minimum frequency 4, minimum lift 1.5, minimum support 0.001
- global reference: up to 1024 rules

## Preliminary replicated result

| metric | value |
|---|---:|
| online rules before consolidation | 415 |
| meta-rules added | 192 |
| online rules after consolidation | 607 |
| online probe cost before | 0.063362 symbol/byte |
| online probe cost after | 0.003258 symbol/byte |
| previous global probe cost | ~0.00368 symbol/byte |
| previous global rules | 624 |

The append-only consolidation reduced mean probe trail cost by about 94.9% relative to the unconsolidated online vocabulary, while preserving all previously stored trails because old rule IDs were not changed.

## Interpretation

The earlier weakness of online learning was not necessarily lack of information. The online learner had discovered many useful local relations, but they were not organized deeply enough across samples. Consolidation over existing trails creates a second learning timescale: fast append-only acquisition followed by slower higher-order organization.

This is an experimental result on synthetic structural families, not evidence of universal superiority. It must next be tested for:

1. multiple consolidation cycles;
2. order sensitivity after consolidation;
3. heterogeneous real files;
4. dictionary growth and pruning/alias strategies;
5. persistence and integrity under corruption.

## Safety property

Consolidation is append-only. Existing relations and IDs remain unchanged, so a trail created before consolidation remains decodable afterward.
