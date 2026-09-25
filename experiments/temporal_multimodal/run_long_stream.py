"""Long-running pre-semantic association regression, independent of fixture labels."""
import pathlib
import sys
import tracemalloc

ROOT = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from reality_slice import Occurrence, RealitySlice, TemporalAssociator
from structural_slice import structural_reality_slice


def run(count=100_000):
    engine = TemporalAssociator(lambda0=0)
    tracemalloc.start()
    snapshots = []
    first_signature = None
    for sid in range(1, count + 1):
        t = float(sid) * 2.0
        occurrences = (
            Occurrence(11, ("device", 1), t, t + .05, source=1, provenance=sid),
            Occurrence(11, ("device", 1), t + .01, t + .06, source=1, provenance=sid),
            Occurrence(22, ("device", 2), t + .2, t + .25, source=2, provenance=sid),
            Occurrence(33, ("device", 3), t + .4, t + .45, source=3, provenance=sid),
        )
        rs = RealitySlice(sid, t, t + 1., occurrences, provenance=(sid,))
        engine.ingest(rs)
        if sid == 1:
            envelope = structural_reality_slice(rs, source_id="long:stream", clock_id="clock:1")
            reordered = structural_reality_slice(
                RealitySlice(sid, t, t + 1., tuple(reversed(occurrences)), provenance=(sid,)),
                source_id="long:stream", clock_id="clock:1",
            )
            assert envelope == reordered
            first_signature = envelope["signature"]
        if sid in (1_000, 10_000, count):
            current, peak = tracemalloc.get_traced_memory()
            snapshots.append((sid, current, peak))
    tracemalloc.stop()
    assert set(engine.links) == {(11, 22), (11, 33), (22, 33)}
    assert all(link.repetitions == count for link in engine.links.values())
    assert all(not hasattr(link, "seen_slices") for link in engine.links.values())
    assert all(engine.pattern_slices[p] == count for p in (11, 22, 33))
    # With fixed vocabulary, retained association state must not grow with time.
    # The final snapshot may include transient allocations; allow 256 KiB slack.
    assert snapshots[-1][1] - snapshots[0][1] < 256 * 1024, snapshots
    print(f"long_stream_slices={count} links={len(engine.links)} "
          f"repetitions={min(x.repetitions for x in engine.links.values())} "
          f"retained_delta_bytes={snapshots[-1][1]-snapshots[0][1]} "
          f"signature={first_signature} pass=True")


if __name__ == "__main__":
    run()
