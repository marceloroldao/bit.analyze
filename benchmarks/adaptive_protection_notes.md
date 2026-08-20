# Adaptive protection notes

The adaptive policy separates rule importance from rule creation frequency.

Criticality is currently based primarily on observed rule usage in stored trails, with rule birth frequency used as a weak prior. Rules are assigned by quantiles:

- Light: lower ~70% of criticality, 2 parity symbols
- Medium: next ~25%, 4 parity symbols
- Strong: upper ~5%, 6 parity symbols

Rules must be physically grouped by profile before parity generation. A Strong rule left inside a Light parity group would only receive Light protection, so the implementation exposes `ProtectionBuckets` for this purpose.

Ignoring ties and partial groups, the target average parity cost is:

`0.70*2 + 0.25*4 + 0.05*6 = 2.7 parity symbols/rule`

Compared with uniform Strong protection at 6 parity symbols/rule, the expected reduction is:

`1 - 2.7/6 = 55%`

This 55% is a structural expectation, not a measured C++ benchmark result. Actual profile counts can differ because equal criticality values may cross quantile thresholds together.

Next validation:

1. measure actual Light/Medium/Strong counts on learned corpora;
2. measure parity overhead after bucket formation;
3. inject failures preferentially into high-usage rules;
4. compare weighted information loss against uniform Light and uniform Strong protection;
5. extend the same policy concept to trail segments.
