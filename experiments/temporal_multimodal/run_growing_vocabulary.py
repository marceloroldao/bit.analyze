"""Measure retained state with an expanding opaque vocabulary; do not claim boundedness."""
import pathlib
import sys
import tracemalloc

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from reality_slice import Occurrence, RealitySlice, TemporalAssociator


def run(count=100_000, period=100):
    engine = TemporalAssociator(lambda0=.00001)
    tracemalloc.start()
    samples = []
    for sid in range(1, count + 1):
        t = sid * 2.
        occurrences = [
            Occurrence(1, ("anchor", 1), t, t + .05),
            Occurrence(2, ("anchor", 2), t + .2, t + .25),
        ]
        if sid % period == 0:
            generation = sid // period
            occurrences.extend((
                Occurrence(10000 + 2 * generation, ("new", 1), t + .4, t + .45),
                Occurrence(10001 + 2 * generation, ("new", 2), t + .6, t + .65),
            ))
        engine.ingest(RealitySlice(sid, t, t + 1., tuple(occurrences)))
        if sid in (count // 4, count // 2, count):
            current, peak = tracemalloc.get_traced_memory()
            samples.append((sid, len(engine.pattern_slices), len(engine.links), current, peak))
    tracemalloc.stop()
    generations = count // period
    # Each novel pair connects to two anchors and itself: five new links.
    assert len(engine.pattern_slices) == 2 + 2 * generations
    assert len(engine.links) == 1 + 5 * generations
    assert engine.links[(1, 2)].repetitions == count
    assert all(engine.links[(10000 + 2 * g, 10001 + 2 * g)].repetitions == 1
               for g in range(1, generations + 1))
    assert samples[0][3] < samples[1][3] < samples[2][3], samples
    for sid, patterns, links, retained, peak in samples:
        print(f"slices={sid} patterns={patterns} links={links} "
              f"retained_bytes={retained} peak_bytes={peak}")
    print("growing_vocabulary_pass=True bounded_state=False "
          "next=explicit_consolidation_and_eviction_policy")


if __name__ == "__main__":
    run()
