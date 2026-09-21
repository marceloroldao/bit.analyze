import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from reality_slice import Modality, Occurrence, RealitySlice, TemporalAssociator
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


if __name__ == "__main__":
    unittest.main()
