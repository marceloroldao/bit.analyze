# Online learning results

## Order sensitivity

The online learner was evaluated across eight arrival orders over the same eight structural families.

A faithful replica of the current `learn_online` algorithm produced:

- final dictionary size: **85–86 rules**;
- mean probe cost: **0.086189–0.086197 symbols/byte**;
- historical trails still decodable after all updates: **100%**.

Interpretation: on this corpus, arrival order changes rule IDs and occasionally one rule, but has negligible effect on probe cost. The append-only rule policy preserves old trails.

## Retention stress

A 32-family stress test was added. Every previously stored trail is decoded after each new family is learned.

The faithful algorithm replica reached approximately **415 rules** after 32 families and preserved all historical trails. This confirms retention under append-only learning, but also exposes continuing dictionary growth.

## Online versus global training

Using the same 32 families:

| method | rules | mean probe cost (symbols/byte) |
|---|---:|---:|
| online append-only | 415 | 0.063362 |
| global retraining | 624 | 0.003677 |

The global learner uses more rules but produces a much shorter representation of held-out shifted probes. The online learner therefore preserves old IDs well, but its local append-only choices are not globally efficient.

This is an important negative result. Online stability and global compactness are currently in tension.

## Consequence for the architecture

The next design problem is not catastrophic forgetting: old trails already survive. The next problem is **vocabulary consolidation**.

A future consolidation mechanism must improve the online dictionary without invalidating historical trails. Candidate directions include:

1. immutable canonical nodes plus alias/redirect nodes;
2. a second-level meta-dictionary over existing rule IDs;
3. background-equivalent consolidation performed explicitly as an offline maintenance operation (not required for inference);
4. MDL-based promotion of cross-family relations while retaining original IDs as compatibility anchors.

Corruption/integrity tests remain scheduled after vocabulary stability and consolidation experiments.
