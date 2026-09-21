import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from reality_slice import Modality, Occurrence, RealitySlice, SparseContextAssociator, TemporalAssociator
from synthetic_stream import HIDDEN, generate


class TemporalMultimodalTests(unittest.TestCase):
    def test_overlap(self):
        a = Occurrence(1, Modality.VISUAL, 1, 2)
        b = Occurrence(2, Modality.AUDIO, 1.5, 2.5)
        self.assertEqual(TemporalAssociator.interval_distance(a, b), 0)

    def test_hidden_pairs(self):
        engine = TemporalAssociator(lambda0=.00001)
        for rs in generate(5000):
            engine.ingest(rs)
        hidden = {p for p, _, _ in HIDDEN}
        expected = {(a, b) for a in hidden for b in hidden if a < b}
        got = {(x.a, x.b) for x in engine.strongest(6)}
        self.assertEqual(got, expected)

    def test_direction(self):
        engine = TemporalAssociator(lambda0=0, simultaneous_delta=.01)
        for sid in range(1, 21):
            t = sid * 10.
            engine.ingest(RealitySlice(
                sid, t, t + 2,
                (
                    Occurrence(10, Modality.VISUAL, t, t + .05),
                    Occurrence(20, Modality.AUDIO, t + .5, t + .55),
                ),
            ))
        fwd, sim, back = engine.links[(10, 20)].direction_probabilities()
        self.assertGreater(fwd, .99)
        self.assertLess(sim, .01)
        self.assertLess(back, .01)
        self.assertTrue(.49 < engine.links[(10, 20)].mean_dt < .51)

    def test_forgetting_consolidation(self):
        weak = TemporalAssociator(lambda0=.1)
        strong = TemporalAssociator(lambda0=.1)

        def make_slice(sid, t):
            return RealitySlice(
                sid, t, t + 1,
                (
                    Occurrence(1, Modality.VISUAL, t, t + .1),
                    Occurrence(2, Modality.AUDIO, t + .2, t + .3),
                ),
            )

        weak.ingest(make_slice(1, 1.))
        for i in range(1, 30):
            strong.ingest(make_slice(i, float(i)))
        weak._forget(weak.links[(1, 2)], 100.)
        strong._forget(strong.links[(1, 2)], 100.)
        self.assertGreater(strong.links[(1, 2)].rho, weak.links[(1, 2)].rho)

    def test_directional_reliability_penalizes_incomplete_antecedent_coverage(self):
        engine = TemporalAssociator(lambda0=0, simultaneous_delta=.01)
        sid = 1
        for i in range(8):
            t = float(i) * 10.
            occurrences = [
                Occurrence(10, Modality.SENSOR, t, t + .02),
            ]
            if i < 4:
                occurrences.append(
                    Occurrence(20, Modality.SENSOR, t + .2, t + .22)
                )
            else:
                occurrences.append(
                    Occurrence(30, Modality.SENSOR, t + .2, t + .22)
                )
            engine.ingest(
                RealitySlice(
                    sid,
                    t,
                    t + 1.,
                    tuple(occurrences),
                )
            )
            sid += 1

        link_20 = engine.links[(10, 20)]
        link_30 = engine.links[(10, 30)]

        self.assertEqual(link_20.repetitions, 4)
        self.assertEqual(link_30.repetitions, 4)
        self.assertAlmostEqual(engine.directional_coverage(link_20), .5, places=6)
        self.assertAlmostEqual(engine.directional_coverage(link_30), .5, places=6)
        self.assertGreater(link_20.direction_probabilities()[0], .99)
        self.assertGreater(link_30.direction_probabilities()[0], .99)
        self.assertTrue(.49 < engine.directional_reliability(link_20) < .51)
        self.assertTrue(.49 < engine.directional_reliability(link_30) < .51)

    def test_directional_reliability_is_high_when_consequence_follows_every_antecedent(self):
        engine = TemporalAssociator(lambda0=0, simultaneous_delta=.01)
        for sid in range(1, 5):
            t = float(sid) * 10.
            engine.ingest(
                RealitySlice(
                    sid,
                    t,
                    t + 1.,
                    (
                        Occurrence(40, Modality.SENSOR, t, t + .02),
                        Occurrence(50, Modality.SENSOR, t + .2, t + .22),
                    ),
                )
            )
        link = engine.links[(40, 50)]
        self.assertAlmostEqual(engine.directional_coverage(link), 1., places=6)
        self.assertGreater(engine.directional_reliability(link), .99)

    def test_sparse_context_recovers_balanced_xor_when_pairwise_links_are_insufficient(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )

        sid = 1
        table = (
            (10, 20, 30),
            (10, 21, 31),
            (11, 20, 31),
            (11, 21, 30),
        )
        for a, c, consequence in table:
            for _ in range(4):
                base = float(sid) * 10.
                rs = RealitySlice(
                    sid,
                    base,
                    base + 1.,
                    (
                        Occurrence(1, Modality.SENSOR, base, base + .02),
                        Occurrence(a, Modality.SENSOR, base + .20, base + .22),
                        Occurrence(c, Modality.SENSOR, base + .30, base + .32),
                        Occurrence(
                            consequence,
                            Modality.SENSOR,
                            base + .60,
                            base + .62,
                        ),
                    ),
                )
                pairwise.ingest(rs)
                higher.ingest(rs)
                sid += 1

        for antecedent in (10, 11, 20, 21):
            for consequence in (30, 31):
                link = pairwise.links[pairwise._key(antecedent, consequence)]
                self.assertTrue(
                    .49 < pairwise.directional_reliability(link) < .51
                )

        admitted = higher.admitted_contexts(
            pairwise,
            min_repetitions=3,
            min_independent_slices=3,
            min_rho=.39,
            min_context_reliability=.75,
            max_lower_order_reliability=.75,
        )
        got = {
            (link.antecedents, link.consequence)
            for link in admitted
        }
        self.assertEqual(
            got,
            {
                ((10, 20), 30),
                ((10, 21), 31),
                ((11, 20), 31),
                ((11, 21), 30),
            },
        )
        for link in admitted:
            self.assertEqual(link.repetitions, 4)
            self.assertEqual(len(link.seen_slices), 4)
            self.assertGreater(higher.context_reliability(link), .99)
            self.assertTrue(
                all(
                    value < .51
                    for value in higher.lower_order_reliabilities(
                        link,
                        pairwise,
                    )
                )
            )

    def test_sparse_context_does_not_create_unobserved_conjunctions(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )
        for sid in range(1, 5):
            base = float(sid) * 10.
            rs = RealitySlice(
                sid,
                base,
                base + 1.,
                (
                    Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                    Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                    Occurrence(30, Modality.SENSOR, base + .60, base + .62),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        self.assertIn((10, 20, 30), higher.links)
        self.assertNotIn((10, 21, 30), higher.links)
        self.assertNotIn((11, 20, 30), higher.links)

    def test_sparse_context_is_not_admitted_when_lower_order_already_resolves(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )
        for sid in range(1, 5):
            base = float(sid) * 10.
            rs = RealitySlice(
                sid,
                base,
                base + 1.,
                (
                    Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                    Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                    Occurrence(30, Modality.SENSOR, base + .60, base + .62),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        link = higher.links[(10, 20, 30)]
        self.assertGreater(higher.context_reliability(link), .99)
        self.assertGreater(
            pairwise.directional_reliability(
                pairwise.links[pairwise._key(10, 30)]
            ),
            .99,
        )
        self.assertEqual(higher.admitted_contexts(pairwise), ())

    def test_sparse_context_requires_tight_antecedent_window(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )
        for sid in range(1, 5):
            base = float(sid) * 10.
            rs = RealitySlice(
                sid,
                base,
                base + 1.,
                (
                    Occurrence(10, Modality.SENSOR, base + .10, base + .12),
                    Occurrence(20, Modality.SENSOR, base + .40, base + .42),
                    Occurrence(30, Modality.SENSOR, base + .70, base + .72),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        self.assertNotIn((10, 20, 30), higher.links)

    def test_sparse_context_prefilter_ignores_one_shot_patterns(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
            min_pattern_support=2,
        )

        slices = []
        for sid in range(1, 5):
            base = float(sid) * 10.
            unique = 1000 + sid
            slices.append(
                RealitySlice(
                    sid,
                    base,
                    base + 1.,
                    (
                        Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                        Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                        Occurrence(30, Modality.SENSOR, base + .60, base + .62),
                        Occurrence(unique, Modality.SENSOR, base + .25, base + .27),
                    ),
                )
            )

        for rs in slices:
            pairwise.ingest(rs)
            higher.ingest(rs, pattern_support=pairwise.pattern_slices)

        self.assertIn((10, 20, 30), higher.links)
        self.assertFalse(
            any(
                any(pattern >= 1001 for pattern in key)
                for key in higher.links
            )
        )

    def test_sparse_context_prefilter_requires_support_map_when_enabled(self):
        higher = SparseContextAssociator(min_pattern_support=2)
        rs = RealitySlice(
            1,
            0.,
            1.,
            (
                Occurrence(10, Modality.SENSOR, .20, .22),
                Occurrence(20, Modality.SENSOR, .30, .32),
                Occurrence(30, Modality.SENSOR, .60, .62),
            ),
        )
        with self.assertRaises(ValueError):
            higher.ingest(rs)

    def test_sparse_context_is_invariant_to_occurrence_input_order(self):
        pairwise_a = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        pairwise_b = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher_a = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )
        higher_b = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )

        for sid in range(1, 5):
            base = float(sid) * 10.
            items = (
                Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                Occurrence(30, Modality.SENSOR, base + .60, base + .62),
                Occurrence(40, Modality.SENSOR, base + .80, base + .82),
            )
            rs_a = RealitySlice(sid, base, base + 1., items)
            rs_b = RealitySlice(sid, base, base + 1., tuple(reversed(items)))
            pairwise_a.ingest(rs_a)
            pairwise_b.ingest(rs_b)
            higher_a.ingest(rs_a)
            higher_b.ingest(rs_b)

        sig_a = tuple(
            sorted(
                (
                    key,
                    link.rho,
                    link.repetitions,
                    link.mean_delay,
                    link.variance_delay,
                    tuple(sorted(link.seen_slices)),
                )
                for key, link in higher_a.links.items()
            )
        )
        sig_b = tuple(
            sorted(
                (
                    key,
                    link.rho,
                    link.repetitions,
                    link.mean_delay,
                    link.variance_delay,
                    tuple(sorted(link.seen_slices)),
                )
                for key, link in higher_b.links.items()
            )
        )
        self.assertEqual(sig_a, sig_b)

    def test_sparse_context_advance_time_passively_decays_untouched_link(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=.20,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )
        for sid in range(1, 5):
            base = float(sid) * 10.
            rs = RealitySlice(
                sid,
                base,
                base + 1.,
                (
                    Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                    Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                    Occurrence(30, Modality.SENSOR, base + .60, base + .62),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        link = higher.links[(10, 20, 30)]
        rho_before = link.rho
        last_observed = link.last_time
        repetitions = link.repetitions
        slices = set(link.seen_slices)

        higher.advance_time(last_observed + 20.0)

        self.assertLess(link.rho, rho_before)
        self.assertEqual(link.last_time, last_observed)
        self.assertEqual(link.repetitions, repetitions)
        self.assertEqual(link.seen_slices, slices)
        self.assertEqual(link.last_decay_time, last_observed + 20.0)

    def test_sparse_context_advance_time_is_not_double_applied_at_same_time(self):
        higher = SparseContextAssociator(
            lambda0=.20,
            context_span=.15,
            max_consequence_delay=1.0,
        )
        for sid in range(1, 5):
            base = float(sid) * 10.
            higher.ingest(
                RealitySlice(
                    sid,
                    base,
                    base + 1.,
                    (
                        Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                        Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                        Occurrence(30, Modality.SENSOR, base + .60, base + .62),
                    ),
                )
            )
        link = higher.links[(10, 20, 30)]
        target = link.last_time + 12.0
        higher.advance_time(target)
        once = link.rho
        higher.advance_time(target)
        self.assertEqual(link.rho, once)

    def test_sparse_context_passive_decay_can_remove_admission_without_deleting_history(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=.015,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )
        # Build the association under a short, dense observation regime first.
        # Passive forgetting is tested only after the context has actually crossed
        # the admission threshold.
        for sid in range(1, 5):
            base = float(sid) * .5
            rs = RealitySlice(
                sid,
                base,
                base + 1.,
                (
                    Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                    Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                    Occurrence(30, Modality.SENSOR, base + .60, base + .62),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        link = higher.links[(10, 20, 30)]
        lower_before = higher.lower_order_reliabilities(link, pairwise)
        self.assertEqual(link.repetitions, 4)
        self.assertEqual(len(link.seen_slices), 4)
        self.assertGreaterEqual(link.rho, .39)
        self.assertGreaterEqual(higher.context_coverage(link), .75)
        self.assertGreaterEqual(higher.temporal_stability(link), .75)
        self.assertGreaterEqual(higher.context_reliability(link), .75)
        self.assertTrue(all(value < 1.1 for value in lower_before))

        admitted_before = higher.admitted_contexts(
            pairwise,
            min_rho=.39,
            min_context_reliability=.75,
            max_lower_order_reliability=1.1,
        )
        self.assertEqual(len(admitted_before), 1)

        history_before = (
            link.repetitions,
            tuple(sorted(link.seen_slices)),
            link.last_time,
        )
        higher.advance_time(link.last_time + 40.0)

        admitted_after = higher.admitted_contexts(
            pairwise,
            min_rho=.39,
            min_context_reliability=.75,
            max_lower_order_reliability=1.1,
        )
        self.assertEqual(admitted_after, ())
        self.assertEqual(
            (
                link.repetitions,
                tuple(sorted(link.seen_slices)),
                link.last_time,
            ),
            history_before,
        )

    def test_sparse_context_recent_window_admits_remapped_consequence_without_deleting_history(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )

        for sid in range(1, 9):
            base = float(sid)
            consequence = 30 if sid <= 4 else 31
            rs = RealitySlice(
                sid,
                base,
                base + 1.,
                (
                    Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                    Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                    Occurrence(
                        consequence,
                        Modality.SENSOR,
                        base + .60,
                        base + .62,
                    ),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        # Global accumulated coverage keeps both competing consequences unresolved.
        self.assertEqual(
            higher.admitted_contexts(
                pairwise,
                min_repetitions=3,
                min_independent_slices=3,
                min_rho=.39,
                min_context_reliability=.75,
                max_lower_order_reliability=.75,
            ),
            (),
        )

        recent = higher.recent_slice_ids(4)
        admitted = higher.admitted_contexts(
            pairwise,
            min_repetitions=3,
            min_independent_slices=3,
            min_rho=.39,
            min_context_reliability=.75,
            max_lower_order_reliability=.75,
            active_slice_ids=recent,
        )

        self.assertEqual(recent, (5, 6, 7, 8))
        self.assertEqual(
            tuple(
                (link.antecedents, link.consequence)
                for link in admitted
            ),
            (((10, 20), 31),),
        )
        self.assertIn((10, 20, 30), higher.links)
        self.assertIn((10, 20, 31), higher.links)
        self.assertAlmostEqual(
            higher.context_coverage(
                higher.links[(10, 20, 31)],
                recent,
            ),
            1.0,
        )

    def test_sparse_context_recent_window_uses_temporal_order_not_ingest_tuple_order(self):
        higher = SparseContextAssociator(lambda0=0)
        pairwise = TemporalAssociator(lambda0=0)

        for sid, base in ((3, 30.), (1, 10.), (4, 40.), (2, 20.)):
            rs = RealitySlice(
                sid,
                base,
                base + 1.,
                (
                    Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                    Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                    Occurrence(30, Modality.SENSOR, base + .60, base + .62),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        self.assertEqual(higher.recent_slice_ids(2), (3, 4))

    def test_sparse_context_recent_window_does_not_mutate_historical_support(self):
        pairwise = TemporalAssociator(lambda0=0)
        higher = SparseContextAssociator(lambda0=0)

        for sid in range(1, 7):
            base = float(sid)
            consequence = 30 if sid <= 3 else 31
            rs = RealitySlice(
                sid,
                base,
                base + 1.,
                (
                    Occurrence(10, Modality.SENSOR, base + .20, base + .22),
                    Occurrence(20, Modality.SENSOR, base + .30, base + .32),
                    Occurrence(
                        consequence,
                        Modality.SENSOR,
                        base + .60,
                        base + .62,
                    ),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        old = higher.links[(10, 20, 30)]
        before = (
            old.rho,
            old.repetitions,
            tuple(sorted(old.seen_slices)),
            higher.context_slices[(10, 20)],
            tuple(sorted(higher.context_seen_slices[(10, 20)])),
        )

        _ = higher.admitted_contexts(
            pairwise,
            active_slice_ids=higher.recent_slice_ids(3),
        )

        after = (
            old.rho,
            old.repetitions,
            tuple(sorted(old.seen_slices)),
            higher.context_slices[(10, 20)],
            tuple(sorted(higher.context_seen_slices[(10, 20)])),
        )
        self.assertEqual(after, before)

    def test_sparse_context_recent_time_window_uses_physical_time_boundaries(self):
        higher = SparseContextAssociator(lambda0=0)

        for sid, end_time in ((1, 1.0), (2, 2.0), (3, 3.0), (4, 4.0)):
            start = end_time - .5
            higher.ingest(
                RealitySlice(
                    sid,
                    start,
                    end_time,
                    (
                        Occurrence(100 + sid, Modality.SENSOR, start + .1, start + .2),
                    ),
                )
            )

        self.assertEqual(
            higher.recent_slice_ids_by_time(2.0, now=4.0),
            (2, 3, 4),
        )
        self.assertEqual(
            higher.recent_slice_ids_by_time(1.0, now=3.5),
            (3,),
        )
        with self.assertRaises(ValueError):
            higher.recent_slice_ids_by_time(0)

    def test_sparse_context_count_window_can_be_displaced_by_high_rate_irrelevant_slices(self):
        pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
        higher = SparseContextAssociator(
            lambda0=0,
            simultaneous_delta=.12,
            context_span=.15,
            max_consequence_delay=1.0,
        )

        for sid in range(1, 5):
            base = sid * .1
            rs = RealitySlice(
                sid,
                base,
                base + .08,
                (
                    Occurrence(10, Modality.SENSOR, base + .01, base + .02),
                    Occurrence(20, Modality.SENSOR, base + .03, base + .04),
                    Occurrence(30, Modality.SENSOR, base + .06, base + .07),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        for offset in range(20):
            sid = 100 + offset
            base = .50 + offset * .02
            rs = RealitySlice(
                sid,
                base,
                base + .01,
                (
                    Occurrence(
                        1000 + offset,
                        Modality.SENSOR,
                        base + .002,
                        base + .008,
                    ),
                ),
            )
            pairwise.ingest(rs)
            higher.ingest(rs)

        count_window = higher.recent_slice_ids(4)
        time_window = higher.recent_slice_ids_by_time(1.0, now=.90)

        count_admitted = higher.admitted_contexts(
            pairwise,
            min_repetitions=3,
            min_independent_slices=3,
            min_rho=.39,
            min_context_reliability=.75,
            max_lower_order_reliability=1.1,
            active_slice_ids=count_window,
        )
        time_admitted = higher.admitted_contexts(
            pairwise,
            min_repetitions=3,
            min_independent_slices=3,
            min_rho=.39,
            min_context_reliability=.75,
            max_lower_order_reliability=1.1,
            active_slice_ids=time_window,
        )

        self.assertEqual(count_admitted, ())
        self.assertEqual(
            tuple(
                (link.antecedents, link.consequence)
                for link in time_admitted
            ),
            (((10, 20), 30),),
        )

    def test_sparse_context_time_window_is_invariant_to_irrelevant_event_rate(self):
        def build(noise_count):
            pairwise = TemporalAssociator(lambda0=0, simultaneous_delta=.12)
            higher = SparseContextAssociator(
                lambda0=0,
                simultaneous_delta=.12,
                context_span=.15,
                max_consequence_delay=1.0,
            )

            for sid in range(1, 5):
                base = sid * .1
                rs = RealitySlice(
                    sid,
                    base,
                    base + .08,
                    (
                        Occurrence(10, Modality.SENSOR, base + .01, base + .02),
                        Occurrence(20, Modality.SENSOR, base + .03, base + .04),
                        Occurrence(30, Modality.SENSOR, base + .06, base + .07),
                    ),
                )
                pairwise.ingest(rs)
                higher.ingest(rs)

            for offset in range(noise_count):
                sid = 1000 + noise_count * 100 + offset
                base = .50 + (offset + 1) * (.40 / (noise_count + 1))
                rs = RealitySlice(
                    sid,
                    base,
                    base + .001,
                    (
                        Occurrence(
                            5000 + offset,
                            Modality.SENSOR,
                            base + .0002,
                            base + .0008,
                        ),
                    ),
                )
                pairwise.ingest(rs)
                higher.ingest(rs)

            active = higher.recent_slice_ids_by_time(1.0, now=.90)
            admitted = higher.admitted_contexts(
                pairwise,
                min_repetitions=3,
                min_independent_slices=3,
                min_rho=.39,
                min_context_reliability=.75,
                max_lower_order_reliability=1.1,
                active_slice_ids=active,
            )
            link = higher.links[(10, 20, 30)]
            return (
                higher.context_coverage(link, active),
                tuple(
                    (item.antecedents, item.consequence)
                    for item in admitted
                ),
                tuple(
                    sorted(link.seen_slices & set(active))
                ),
            )

        slow = build(2)
        fast = build(200)

        self.assertEqual(slow[0], 1.0)
        self.assertEqual(fast[0], 1.0)
        self.assertEqual(slow[1], (((10, 20), 30),))
        self.assertEqual(fast[1], (((10, 20), 30),))
        self.assertEqual(slow[2], (1, 2, 3, 4))
        self.assertEqual(fast[2], (1, 2, 3, 4))


if __name__ == "__main__":
    unittest.main()
