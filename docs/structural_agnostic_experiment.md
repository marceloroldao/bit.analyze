# Format-blind structural experiment

This document freezes the first modality-agnostic experiment protocol before interpreting its results.

## Constraint

The core receives byte sequences only. File extension, MIME type, magic-number classification, format-specific regexes and semantic parsers are excluded from the experiment.

## Pipeline

`raw bytes -> StructuralStream -> StructuralExtractor -> canonical HierarchicalMemory encoder -> StructuralEvent`

The canonical encoder remains the authority for learned relation IDs.

## Corpus classes

The initial synthetic corpus contains eight byte sequences representing: prose-like text, markup-like structured text, object-like serialized text, spatially repetitive bytes, temporally repetitive bytes, binary-like bytes, deterministic pseudo-random bytes, and a strongly repetitive byte pattern. These labels are benchmark metadata only and are never supplied to the structural core.

## Questions

1. Does re-presenting an identical object produce an equivalent structural representation?
2. Does a small byte mutation preserve structural proximity?
3. Do different objects separate structurally?
4. Are strongly recurrent structures reflected in the representation?
5. Does pseudo-random input behave differently from structured/repetitive input?
6. Is the representation invariant to input chunk boundaries?
7. Does streaming memory remain bounded by configured windowing rather than total input size?

## Frozen first-pass measurements

- relation-ID set cardinality
- StructuralEvent count
- chunk-boundary invariance for chunk sizes 17 and 257 bytes
- Jaccard similarity over relation-ID sets for identical input, one-byte mutation, structured-vs-random and repetitive-vs-random comparisons

Jaccard is an observation instrument for this experiment, not a permanent definition of universal structural similarity.

## Interpretation rule

No format-specific feature may be introduced to improve a disappointing result. A failure to separate structured and random data is evidence about the current canonical representation and must be recorded as such.

## Gate

Quantitative similarity/novelty/recurrence/stability APIs are not promoted into the core until this experiment has executed successfully on CI and its measurements have been reviewed. The experiment must not alter existing learned-ID semantics or compatibility.
