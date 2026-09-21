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


if __name__ == "__main__":
    unittest.main()
