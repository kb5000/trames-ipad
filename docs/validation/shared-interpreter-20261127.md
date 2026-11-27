# iPad shared interpreter migration — 2026-11-27

The implementation moved to Runtime `5b2504914e349301d4238bfe9430a56bce673925`.
`trames-cpu` and `trames-compute` are compatibility re-exports, preserving public
APIs without an extra runtime wrapper, dynamic dispatch or memory layer.
Instruction-audit source hashes now include the canonical Runtime implementation.
Existing product-level register/stack tests remain at their previous entry points;
implementation unit tests moved with the source and run in shared Runtime CI.

The all-features normal dependency tree of `trames-cpu` contains
`trames-interpreter` / `trames-host-compute` and **no `trames-jit`**. The product
boundary script enforces that invariant before builds. Android explicitly opts
into its optional JIT dependency; iPad does not compile/link its code cache.

Existing unrelated dirty application/performance work was preserved. Runtime
integration was fast-forward only; all 20 previously modified tracked Runtime
files kept their exact bytes during the integration. Validation of the extracted
implementation is recorded in Runtime's `software-cpu-android-bootstrap-20261127.md`.

Passed migration checks: compatibility register/small-stack tests, scoped Clippy,
iOS and ARM64 Linux musl CPU checks, and the complete product boundary script
against a pristine published Runtime. `trames-core --lib` also compiled against
that pristine Runtime. Later Android-only alias binding fixes keep the same
Apple/Linux interpreter implementation; their passed checks are reused.
