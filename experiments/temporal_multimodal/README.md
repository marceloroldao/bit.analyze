# Temporal multimodal RealitySlice experiment

This experiment tests a pre-semantic hypothesis: recurring patterns from independent modalities can become associated from temporal proximity, repetition, temporal direction and forgetting without modality-specific labels or an LLM.

Boundary: bit.analyze detects recurring pattern IDs; this experiment groups occurrences into immutable RealitySlices and learns temporal associations; memoria.ia is intentionally not modified.

The deterministic synthetic generator hides a four-pattern multimodal structure among independent noise. The learner receives only pattern IDs, modalities, timestamps, slice IDs and provenance.

Run: python experiments/temporal_multimodal/run_experiment.py

Gate: the six pairwise links among the four hidden patterns must occupy the six strongest associations after 100,000 independent slices.

Tests: python -m pytest experiments/temporal_multimodal/tests -q

The experiment records direction, mean temporal displacement, variance, independent-slice recurrence, saturating reinforcement and repetition-dependent forgetting. Synthetic recovery validates only this temporal-association mechanism, not semantic understanding.


## Directional coverage and reliability

The temporal associator also exposes two structural metrics for contextual
discrimination:

- `directional_coverage(link)`: the fraction of slices containing the inferred
  antecedent that also contain the pair;
- `directional_reliability(link)`: directional coverage multiplied by the dominant
  temporal-direction confidence.

For a dominant forward relation, coverage uses pattern `a` as the antecedent. For a
dominant backward relation it uses pattern `b`. Simultaneous relations use the more
frequent pattern as a conservative denominator.

These metrics are not causal probabilities. They answer a narrower structural
question: when the observed antecedent is present, how consistently is this temporal
pair also present with the same dominant direction?

This supports contextual experiments where one antecedent can lead to different
consequences under different unobserved or separately observed world conditions.

## Adversarial structural closure

The adversarial gate includes a deliberately correlated pair whose members co-occur often but do not belong to the same recurrent temporal structure. Direct pair evidence alone can therefore rank that distractor alongside true hidden links.

For adversarial ranking, each direct pair evidence score is multiplied by the mean of the three strongest shared-neighbor support scores. This treats recurrent triangle closure as joint structural evidence: a pair is stronger when its direct temporal evidence is also supported by independently strong neighboring relations. The gate keeps its original requirement of recovering all expected links with at least a 2x margin; the threshold is not relaxed.

Integration gate before freeze: this experiment was first validated against `experiment/universal-structural-stream`, then revalidated against consolidated `main` after the universal structural stream was promoted. This keeps the temporal multimodal layer and the universal structural stream under the same release gate before freezing.
