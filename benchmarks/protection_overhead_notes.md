# Protection overhead notes

This note separates structural overhead estimates from compiled benchmark output.

## Current protection model

- Rules: integrity hash per rule + two-parity recovery group.
- Trail: global hash + hash per block + two-parity recovery block.
- Default examples below use 8 rules per rule group and 64 symbols per trail block.

## Expected 64-bit build layout

On a typical 64-bit ABI, the current structs are expected to have approximately:

- `AdaptiveRule`: 32 bytes
- `RuleDualParityGroup`: 80 bytes
- `SymbolId`: 8 bytes
- `TrailDualParityBlock`: 32 bytes

The executable `bit_analyze_protection_overhead` reports actual `sizeof(...)` values for the compiler/ABI in use.

For 1024 rules and a trail of 1,048,576 symbols, the expected overhead is approximately:

| Component | Overhead vs its payload |
|---|---:|
| Rule dual parity | 31.25% |
| Rule integrity hashes | 25.00% |
| Rule protection total | 56.25% |
| Trail dual parity | 6.25% |
| Trail block + global hashes | 1.56% |
| Trail protection total | 7.81% |
| Combined rule + trail protection | ~8.00% |

The high rule percentage is less alarming than it first appears because the rule dictionary is normally far smaller than the trail payload. The combined percentage is the more relevant system-level number.

## Recovery guarantees under the current model

Within one protected group/block, assuming corruption positions can be localized by integrity metadata:

- 1 rule: recoverable.
- 2 rules: recoverable with dual GF(256) parity.
- 3+ rules: not guaranteed.
- 1 trail symbol: recoverable without storing its exact index; block hash disambiguates the candidate position.
- 2 trail symbols: recoverable without storing their exact indices; dual GF(256) parity generates candidates and block hash identifies the valid repair.
- 3+ trail symbols in the same block: not guaranteed.

## Validation status

The GF(256) two-symbol algebra was independently exercised over random 64-bit values for block sizes 2, 8 and 64 and reconstructed the selected missing values exactly.

The repository contains C++ tests for rule recovery, trail recovery and combined rule+trail corruption. A local full CMake/CTest run still needs to be executed in an environment with repository checkout access; the current assistant runtime cannot resolve `github.com` from its local shell.
