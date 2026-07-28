## Title
Fix truncated-codestream exception handling in tile header parsing

## Summary
- Fix exception handling in `tile::parse_tile_header` to also catch `std::exception` and unknown exceptions, not just `const char*`.
- Preserve resilience behavior by logging in resilient mode and only re-raising through `OJPH_ERROR` in non-resilient mode.
- Add a small standalone demo script (`subprojects/js/standalone/truncated_decode_demo.sh`) to reproduce truncated codestream decoding behavior and confirm graceful process termination.

## Problem
When decoding truncated codestreams, OpenJPH may throw `std::runtime_error` via `OJPH_ERROR`.  
`tile::parse_tile_header` currently catches only `const char*`, so `std::runtime_error` bypasses this catch and may terminate the process unexpectedly in downstream WASM/node integrations.

## Root Cause
Mismatch between thrown exception type and caught exception type:
- Throw site: `OJPH_ERROR` -> `std::runtime_error`
- Catch site in `ojph_tile.cpp`: `catch (const char*)`

## Fix Details
In `src/core/codestream/ojph_tile.cpp`, keep the existing handler and add two more:
- `catch (const char* error)` — unchanged; still the path for the `throw "..."`
  string literals in `ojph_bitbuffer_read.h` and `ojph_precinct.cpp`, which are
  the most common truncation errors and carry useful messages.
- `catch (const std::exception& error)` — new; covers `std::runtime_error` from
  `OJPH_ERROR` and `std::bad_alloc` from the allocators.
- `catch (...)` — new; backstop so nothing escapes `parse_tile_header`.

## Why This Is Safe
- No behavior change on successful decodes.
- The `const char*` handler is untouched, so existing truncation diagnostics keep
  their original messages.
- In resilient mode, errors continue to be reported via `OJPH_INFO`.
- In non-resilient mode, failures still propagate as errors through `OJPH_ERROR`.

## Repro / Validation
1. Start from a valid `.j2c`.
2. Truncate to first 10 KiB.
3. Decode with `ojph_expand`.
4. Verify process exits normally (possibly non-zero), instead of aborting via uncaught exception.

Standalone script:
- `subprojects/js/standalone/truncated_decode_demo.sh`

## Notes
Downstream WASM wrappers may also require exception-catching support at link time to avoid runtime aborts when exceptions are thrown across wrapper boundaries.
