import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from reality_slice import Modality, Occurrence, RealitySlice, TemporalAssociator
from structural_slice import SCHEMA as STRUCTURAL_SLICE_SCHEMA, structural_reality_slice
from synthetic_stream import HIDDEN, generate


class TemporalMultimodalTests(unittest.TestCase):
    def test_occurrence_accepts_opaque_stream_identity(self):
        custom = Occurrence(1, "lidar:roof:v3", 1.0, 1.1)
        tuple_stream = Occurrence(2, ("device-7", 42), 1.1, 1.2)

        self.assertEqual(custom.stream_id, "lidar:roof:v3")
        self.assertEqual(tuple_stream.stream_id, ("device-7", 42))

        engine = TemporalAssociator(lambda0=0)
        engine.ingest(
            RealitySlice(
                1,
                1.0,
                2.0,
                (custom, tuple_stream),
            )
        )
        self.assertIn((1, 2), engine.links)

    def test_association_dynamics_do_not_depend_on_stream_label_taxonomy(self):
        left = TemporalAssociator(lambda0=0)
        right = TemporalAssociator(lambda0=0)

        for sid in range(1, 6):
            t = float(sid)
            left.ingest(
                RealitySlice(
                    sid,
                    t,
                    t + 1.0,
                    (
                        Occurrence(10, "unknown-stream-A", t, t + 0.05),
                        Occurrence(20, "unknown-stream-B", t + 0.2, t + 0.25),
                    ),
                )
            )
            right.ingest(
                RealitySlice(
                    sid,
                    t,
                    t + 1.0,
                    (
                        Occurrence(10, Modality.VISUAL, t, t + 0.05),
                        Occurrence(20, Modality.AUDIO, t + 0.2, t + 0.25),
                    ),
                )
            )

        a = left.links[(10, 20)]
        b = right.links[(10, 20)]
        self.assertEqual(a.repetitions, b.repetitions)
        self.assertAlmostEqual(a.rho, b.rho, places=12)
        self.assertEqual(a.direction_probabilities(), b.direction_probabilities())
        self.assertAlmostEqual(a.mean_dt, b.mean_dt, places=12)

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


    def test_structural_slice_is_deterministic_under_occurrence_reordering(self):
        occurrences = (
            Occurrence(30, Modality.AUDIO, 10.2, 10.3, source=2, provenance=8),
            Occurrence(10, Modality.VISUAL, 10.0, 10.1, source=1, provenance=7),
            Occurrence(20, Modality.SENSOR, 10.1, 10.2, source=3, provenance=9),
        )
        left = structural_reality_slice(
            RealitySlice(5, 10.0, 11.0, occurrences, provenance=(11, 12)),
            source_id="reality:test",
            clock_id="world:clock",
        )
        right = structural_reality_slice(
            RealitySlice(5, 10.0, 11.0, tuple(reversed(occurrences)), provenance=(11, 12)),
            source_id="reality:test",
            clock_id="world:clock",
        )

        self.assertEqual(left, right)
        self.assertEqual(left["schema"], STRUCTURAL_SLICE_SCHEMA)
        self.assertEqual(left["trail"], [10, 20, 30])
        self.assertFalse(left["semantic_projection"])
        self.assertNotIn("subject", left)
        self.assertNotIn("predicate", left)
        self.assertNotIn("object", left)

    def test_structural_slice_identity_is_modality_agnostic(self):
        visual = RealitySlice(
            1,
            0.0,
            1.0,
            (Occurrence(42, Modality.VISUAL, 0.2, 0.3, source=7, provenance=3),),
        )
        audio = RealitySlice(
            1,
            0.0,
            1.0,
            (Occurrence(42, Modality.AUDIO, 0.2, 0.3, source=7, provenance=3),),
        )

        left = structural_reality_slice(
            visual,
            source_id="reality:test",
            clock_id="world:clock",
        )
        right = structural_reality_slice(
            audio,
            source_id="reality:test",
            clock_id="world:clock",
        )
        self.assertEqual(left, right)

    def test_structural_slice_preserves_multiplicity_and_relative_time(self):
        rs = RealitySlice(
            9,
            100.0,
            101.0,
            (
                Occurrence(5, Modality.SENSOR, 100.10, 100.20, source=1, provenance=2),
                Occurrence(5, Modality.SENSOR, 100.30, 100.40, source=1, provenance=3),
                Occurrence(6, Modality.SENSOR, 100.20, 100.25, source=1, provenance=4),
            ),
        )
        envelope = structural_reality_slice(
            rs,
            source_id="reality:test",
            clock_id="sensor:clock",
        )

        self.assertEqual(envelope["trail"], [5, 6, 5])
        self.assertEqual(envelope["temporal"], {
            "clock_id": "sensor:clock",
            "t_start": 100.0,
            "t_end": 101.0,
            "unit": "s",
        })
        self.assertAlmostEqual(envelope["occurrences"][0]["dt_start"], 0.10)
        self.assertAlmostEqual(envelope["occurrences"][1]["dt_start"], 0.20)
        self.assertAlmostEqual(envelope["occurrences"][2]["dt_start"], 0.30)

    def test_structural_slice_collapses_dense_simultaneous_occurrences_into_one_unit(self):
        occurrences = tuple(
            Occurrence(
                pattern=index % 64,
                modality=Modality.SENSOR,
                t_start=1.0,
                t_end=1.0,
                source=index % 8,
                provenance=index,
            )
            for index in range(10_000)
        )
        envelope = structural_reality_slice(
            RealitySlice(77, 1.0, 1.0, occurrences),
            source_id="dense:test",
            clock_id="sensor:dense",
        )

        self.assertEqual(len(envelope["occurrences"]), 10_000)
        self.assertEqual(len(envelope["trail"]), 10_000)
        self.assertEqual(envelope["temporal"]["t_start"], 1.0)
        self.assertEqual(envelope["temporal"]["t_end"], 1.0)
        self.assertEqual(len(envelope["signature"]), 40)

    def test_structural_slice_signature_changes_with_temporal_context(self):
        rs = RealitySlice(
            1,
            0.0,
            1.0,
            (Occurrence(10, Modality.SENSOR, 0.2, 0.3),),
        )
        first = structural_reality_slice(
            rs,
            source_id="reality:test",
            clock_id="clock:a",
        )
        second = structural_reality_slice(
            rs,
            source_id="reality:test",
            clock_id="clock:b",
        )
        self.assertNotEqual(first["signature"], second["signature"])


if __name__ == "__main__":
    unittest.main()
