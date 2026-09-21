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


## Sparse higher-order contexts

`SparseContextAssociator` is an experimental higher-order structural learner introduced
after a balanced XOR-style gate demonstrated a concrete limitation of pairwise links.

A context association keeps an opaque pair of antecedent pattern IDs followed by one
opaque consequence pattern ID without creating a synthetic combined pattern ID.

The implementation is deliberately sparse:

- only pattern pairs actually observed close together in one RealitySlice are indexed;
- the consequence must occur later than both antecedents;
- the antecedent pair must fit inside a small temporal `context_span`;
- recurrence is counted at most once per RealitySlice;
- context coverage is measured against slices where that same antecedent pair appeared;
- admission requires sufficient repetition, independent provenance, rho and contextual reliability;
- both lower-order antecedent-to-consequence relations must remain below the configured reliability ceiling.

Therefore a higher-order candidate is not admitted when a simpler pairwise relation already resolves the continuation.

This is a presemantic structural primitive. It does not label conjunctions, causality,
truth, rules or world meaning, and it does not materialize the Cartesian product of
all observed patterns.

Life Gate 019 validation target: the balanced XOR corpus must admit only sparse observed two-pattern contexts whose lower-order consequence relations remain insufficient.


### Recurrence prefilter

For distractor-rich streams, `SparseContextAssociator` can require a minimum
independent pattern recurrence before a pattern is eligible for higher-order
indexing. Set `min_pattern_support > 1` and provide the current pattern-support
map to `ingest()`.

This is an indexing prefilter, not an evidence shortcut. A one-shot pattern is
ignored before context-link allocation; recurrent patterns still pass through
the normal context coverage, temporal stability, lower-order insufficiency and
independent-slice admission gates.

The goal is to prevent transient noise from creating combinatorial higher-order
state while preserving deterministic behavior for recurrent structure.


### Passive higher-order decay

`SparseContextAssociator.advance_time(now)` decays the `rho` of every known
higher-order link, including links that are not present in the current
RealitySlice.

The implementation keeps two clocks separate:

- `last_time`: last real observation of the context/consequence link;
- `last_decay_time`: last time passive decay was applied.

Passive decay does not add repetitions, does not add RealitySlice provenance,
does not change context coverage, and does not rewrite the last observation
time. Calling `advance_time()` twice with the same timestamp is idempotent.

This allows a formerly admitted context to leave the current evidence set after
a long period without observation while its historical support remains auditable
outside the active-admission layer.


### Passive higher-order forgetting

`SparseContextAssociator.advance_time(now)` decays every known higher-order link
without creating a new observation, changing repetition counts, or modifying
independent-slice provenance.

`ingest()` calls `advance_time(rs.t_end)` before processing the current slice, so
a link can lose rho while unrelated RealitySlices continue advancing logical time.

Decay uses `last_decay_time` in addition to `last_time`, preventing the same interval
from being applied twice. Consolidation continues to reduce the decay rate for
repeatedly reinforced links.

Passive forgetting affects current evidence strength only. Historical repetition,
timing statistics, and provenance remain intact. Downstream memory can therefore
separate historical observation from current admission instead of deleting history.


### Physical-time evidence window

`SparseContextAssociator.recent_slice_ids_by_time(time_span, now=None)` selects
RealitySlices by their recorded `t_end` inside a physical-time interval.

This is intentionally different from `recent_slice_ids(N)`. A count window can
change merely because an unrelated source emits more slices; a time window keeps
evidence eligibility tied to elapsed time instead of event rate.

The time-window query is read-only. It does not delete historical links, reset rho,
change provenance, or create observations. The returned slice IDs can be passed to
`admitted_contexts(..., active_slice_ids=...)` exactly like a count-based window.

When `now` is omitted, the latest known slice end time is the reference. With an
explicit `now`, only slices satisfying `now - time_span <= t_end <= now` are returned.


### Bounded-lateness event-time reordering

`RealitySliceReorderBuffer(allowed_lateness=...)` separates event time from arrival order.

The buffer tracks a monotonic `max_event_time` and derives a monotonic watermark as
`max_event_time - allowed_lateness`. Accepted slices remain pending until the watermark
makes their event-time position safe, then they are emitted deterministically by
`(t_end, t_start, slice_id)`.

A slice with `t_end < current_watermark` is not silently ingested. `offer()` returns a
`LateRealitySliceRejection` containing the slice ID, event time, watermark and reason.

This keeps delayed-but-valid evidence inside the bounded lateness window while preventing
too-late arrivals from moving temporal learner state backward. Duplicate slice IDs raise
an error rather than creating repeated evidence. `flush()` is an explicit stream-boundary
operation that emits all accepted pending slices in event-time order.

The reorder layer is intentionally upstream of `TemporalAssociator` and
`SparseContextAssociator`; the associators themselves remain focused on structural
learning from an already coherent event-time trajectory.
