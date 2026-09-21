from __future__ import annotations

import hashlib
import json
from math import isfinite
from typing import Any

from reality_slice import RealitySlice


SCHEMA = "bit-analyze-reality-slice-structural/v1"


def _canonical_json(value: object) -> bytes:
    return json.dumps(
        value,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")


def structural_reality_slice(
    reality_slice: RealitySlice,
    *,
    source_id: str,
    clock_id: str,
) -> dict[str, Any]:
    """Project a RealitySlice into an opaque, deterministic structural envelope.

    The adapter does not assign semantic labels and deliberately excludes
    Occurrence.modality from structural identity. Upstream pattern IDs and
    producer/source identities carry structural identity; modality names remain
    an input-side concern.

    Exact relative timing, multiplicity, source/provenance IDs and slice
    provenance are preserved. Reordering the input occurrence tuple therefore
    does not change the resulting envelope.
    """
    source_id = str(source_id).strip()
    clock_id = str(clock_id).strip()
    if not source_id:
        raise ValueError("source_id is required")
    if not clock_id:
        raise ValueError("clock_id is required")
    if not isfinite(float(reality_slice.t_start)) or not isfinite(
        float(reality_slice.t_end)
    ):
        raise ValueError("RealitySlice bounds must be finite")

    ordered = sorted(
        reality_slice.occurrences,
        key=lambda item: (
            float(item.center),
            float(item.t_start),
            float(item.t_end),
            int(item.pattern),
            int(item.source),
            int(item.provenance),
        ),
    )
    occurrences = []
    trail = []
    for item in ordered:
        t_start = float(item.t_start)
        t_end = float(item.t_end)
        if not isfinite(t_start) or not isfinite(t_end):
            raise ValueError("RealitySlice occurrence bounds must be finite")
        pattern = int(item.pattern)
        if pattern < 0:
            raise ValueError("RealitySlice pattern ids must be >= 0")
        trail.append(pattern)
        occurrences.append(
            {
                "pattern": pattern,
                "dt_start": t_start - float(reality_slice.t_start),
                "dt_end": t_end - float(reality_slice.t_start),
                "source": int(item.source),
                "provenance": int(item.provenance),
            }
        )

    core = {
        "schema": SCHEMA,
        "source_id": source_id,
        "slice_id": int(reality_slice.slice_id),
        "trail": trail,
        "occurrences": occurrences,
        "slice_provenance": [int(item) for item in reality_slice.provenance],
        "temporal": {
            "clock_id": clock_id,
            "t_start": float(reality_slice.t_start),
            "t_end": float(reality_slice.t_end),
            "unit": "s",
        },
        "semantic_projection": False,
    }
    if core["slice_id"] < 0:
        raise ValueError("RealitySlice slice_id must be >= 0")
    core["signature"] = hashlib.blake2b(
        _canonical_json(core),
        digest_size=20,
    ).hexdigest()
    return core
