# OpenArena Unit Tests

Minimal, dependency-free unit test harness for the OpenArena engine. No
external test framework (CUnit/Check/gtest) required — just gcc + libm.

## What is covered

These tests exercise the engine's `qcommon` string, token-parser, vector/math,
info-string, network-message, Huffman and collision-map (winding) helpers
(`code/qcommon/q_shared.c`, `q_math.c`, `msg.c`, `huffman.c`, `cm_polylib.c`),
which sit on the hot path for shader/config loading and the network protocol.
They compile and run **without SDL2 or OpenGL**, so they work in any CI
environment.

| Suite                     | Covers                                                                  |
|---------------------------|-------------------------------------------------------------------------|
| `tests_qshared.c`         | `Q_strncpyz`, `Q_stricmpn`, `Com_sprintf`, `COM_StripExtension`, `COM_SkipPath`, `COM_Compress`, `Q_CountChar`, `Q_strlwr/upr`, `Q_strcat` |
| `tests_qparse.c`          | `COM_Parse` / `COM_ParseExt` token parsing, quoting, comments           |
| `tests_qmath.c`           | vec3 macros (`VectorAdd`/`Subtract`/`Clear`/`Set`/`Copy`/`DotProduct`), `VectorNormalize`, `CrossProduct`, `AngleVectors`, `ProjectPointOnPlane`, `RotatePointAroundVector`, `Q_rsqrt`, `MakeNormalVectors` |
| `tests_qmath2.c`          | `ClearBounds`/`AddPointToBounds`, `BoundsIntersect*`, `RadiusFromBounds`, `AngleNormalize*`, `LerpAngle`, `AngleDelta`, `DirToByte`, `Q_log2`, `Q_isnan` |
| `tests_qmath3.c`          | `NormalizeColor`, `PlaneFromPoints`, `Matrix4*`, `MakeNormalVectors`, `vectoangles`, `Q_acos` |
| `tests_qmath4.c`          | `PerpendicularVector`, `RotateAroundDirection`, `Q_fabs`               |
| `tests_qmath5.c`          | `BoxOnPlaneSide` (axial fast path + general signbits case: front/behind/crossing) |
| `tests_qmath6.c`          | `ClampChar`/`ClampShort`, `VectorNormalize2`, `Vector4Scale`, `AngleMod`, `AngleSubtract`/`AnglesSubtract`, `LerpAngle`, `ColorBytes3/4`, `SetPlaneSignbits`, `Q_rand`/`Q_random`/`Q_crandom` |
| `tests_qmath7.c`          | `_DotProduct`, `_VectorAdd`/`_VectorSubtract`/`_VectorCopy`/`_VectorScale`, `_VectorMA` |
| `tests_qmatrix.c`         | `AnglesToAxis`, `AxisClear`/`AxisCopy`, `MatrixMultiply`, `VectorRotate` |
| `tests_qstr.c`            | `Q_PrintStrlen`, `Q_CleanStr`, `Com_HexStrToInt`, `COM_GetExtension`, `COM_DefaultExtension`, `COM_TruncateLongString` |
| `tests_qstring2.c`        | `Q_strlwr`/`Q_strupr`, `Q_strcat`, `Q_CountChar`                        |
| `tests_qformat.c`         | `Com_sprintf`, `va`, `Q_strncpy`                                         |
| `tests_qchar.c`           | `Q_isprint`, `Q_islower`/`Q_isupper`/`Q_isalpha`, `Q_isanumber`, `Q_isintegral` |
| `tests_qtoken.c`          | `COM_SkipCharset`, `COM_SkipTokens`, `COM_Parse` quoting/comma handling |
| `tests_qparse2.c`         | `COM_ParseExt` line-break handling (`allowLineBreaks`), `SkipBracedSection` (nested/unbalanced), `SkipRestOfLine`, `Parse1D/2D/3DMatrix` round-trip |
| `tests_qcompress.c`       | `COM_Compress` block/line comments, `/* */`, GLSL escape, newline collapse, quoted strings |
| `tests_qshared2.c`        | `Q_strncmp`, `COM_CompareExtension`, `*NoSwap` (host-order identity)    |
| `tests_qshared3.c`        | `Q_vsnprintf` (basic/float/truncation), `COM_BeginParseSession`/`COM_GetCurrentParseLine`, `COM_MatchToken` (success), `COM_ParseError`/`COM_ParseWarning` |
| `tests_qinfo.c`           | `Info_ValueForKey`, `Info_SetValueForKey` (+`_Big`), `Info_RemoveKey` (+`_Big`), `Info_Validate`, `Info_NextPair` |
| `tests_qinfo_overflow.c`  | buffer-overflow safety for `Info_SetValueForKey` (separate `test_overflow` binary) |
| `tests_qinfo_guard.c`     | fatal guard paths: `Com_Error(ERR_DROP)` on oversize input to `Info_ValueForKey`/`Info_RemoveKey`/`Info_SetValueForKey` (+`_Big`), and blacklist rejection (separate `test_overflow` binary) |
| `tests_qhuffman.c`        | Huffman encode/decode round-trip (`Huff_Compress`/`Huff_Decompress` on a `msg_t`) |
| `tests_qhuffman2.c`       | Huffman low-level primitives: `Huff_Init`, `Huff_getBloc`/`Huff_setBloc`, `Huff_putBit`/`Huff_getBit`, `Huff_addRef` |
| `tests_qmsg.c`            | `MSG_Init`, `MSG_Write*/Read*` (char/byte/short/long/float/string/bits), `MSG_HashKey`, `MSG_Clear` |
| `tests_qmsg2.c`           | `MSG_WriteBits` big strings, `MSG_ReadByte` lookahead, `MSG_WriteAngle16`, `MSG_WriteDelta*` |
| `tests_qmsg3.c`           | out-of-band `MSG_Write*/Read*`, `MSG_Copy`, `MSG_WriteDeltaKey*` + `MSG_WriteAngle`, `MSG_WriteDeltaUsercmdKey`/`MSG_ReadDeltaUsercmdKey` |
| `tests_qmsg4.c`           | `MSG_WriteBits`/ReadBits edge cases                                    |
| `tests_qmsg5.c`           | `MSG_WriteBits`/`MSG_ReadBits` bit-exact round-trip (mixed widths, signed 8/16, byte-boundary packing), OOB raw bytes, overflow flag |
| `tests_qmsg6.c`           | `MSG_WriteDeltaEntity`/`MSG_ReadDeltaEntity` entity-state round-trip   |
| `tests_qmsg7.c`           | `MSG_WriteDeltaPlayerstate`/`MSG_ReadDeltaPlayerstate` player-state round-trip |
| `tests_qva.c`             | `va()` reentrancy (ping-pong buffers, nested calls), `Com_Clamp` saturation |
| `tests_qbyteswap.c`       | byte-swap / endianness primitives: `ShortSwap`, `LongSwap`, `Long64Swap`, `FloatSwap`, `CopyShortSwap`, `CopyLongSwap` |
| `tests_qcm_poly.c`        | `cm_polylib.c` winding geometry: `BaseWindingForPlane`, `WindingArea`/`Bounds`/`Center`/`Plane`, `CopyWinding`, `WindingOnPlaneSide`, `ChopWindingInPlace`, `RemoveColinearPoints` (separate `test_cm` binary) |
| `tests_qcm_poly_nofail.c` | `CheckWinding` fatal path (`Com_Error(ERR_DROP)` on degenerate winding) — separate `test_cm_nofail` binary |
| `tests_qparse_nofail.c`   | `COM_MatchToken` mismatch fatal path (`Com_Error(ERR_DROP)`) — separate `test_parse_nofail` binary |

Total: **343 tests** (main suite `test_all`) + **8 overflow/guard-safety tests**
(`test_overflow`) + **1 parse-fatal test** (`test_parse_nofail`) + **11 winding
tests** (`test_cm`) + **1 winding-fatal test** (`test_cm_nofail`), all passing.

## Run locally

From the repo root:

    make -f tests/Makefile run

This builds and runs five binaries:

* `tests/test_all` — the main suite (strings, parser, vector math, info
  strings, messages, Huffman, byteswap).
* `tests/test_overflow` — buffer-overflow safety tests for the info-string
  setters. Uses a non-aborting `Com_Error` stub (`test_stubs_nofail.c`) and a
  canary placed after the buffer, so an overrun is detected instead of
  crashing the test process.
* `tests/test_parse_nofail` — fatal-path test for `COM_MatchToken` mismatch.
* `tests/test_cm` — collision-map winding geometry (`cm_polylib.c`), backed by
  a malloc-based allocator stub (`test_stubs_cm.c`).
* `tests/test_cm_nofail` — fatal-path test for `CheckWinding` on a degenerate
  winding.

Or build manually (example for the main suite):

    cd tests
    gcc -I../code/qcommon -I../code -o test_all \
        oa_test_run.c test_main.c test_stubs.c \
        tests_qshared.c tests_qparse.c tests_qmath.c tests_qmath2.c \
        tests_qmath3.c tests_qmath4.c tests_qmatrix.c tests_qstr.c \
        tests_qstring2.c tests_qformat.c tests_qchar.c tests_qtoken.c \
        tests_qinfo.c tests_qhuffman.c tests_qmsg.c tests_qmsg2.c \
        tests_qmsg3.c tests_qmsg4.c tests_qmsg5.c tests_qmsg6.c \
        tests_qmsg7.c tests_qva.c tests_qmath5.c tests_qmath6.c \
        tests_qshared2.c tests_qmath7.c tests_qshared3.c \
        tests_qhuffman2.c \
        ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
        ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
    ./test_all

The binaries exit non-zero if any test fails, so they are CI-friendly.

## How it works

* `oa_test.h` — assertion macros (`OA_ASSERT`, `OA_ASSERT_INT`,
  `OA_ASSERT_STR`, `OA_ASSERT_STRN`, `OA_ASSERT_FLOAT`, `OA_ASSERT_VEC3`,
  `OA_ASSERT_VEC3_ZERO`) plus the `TEST(name)` macro. Each `TEST` self-registers
  via a GCC/Clang constructor attribute (no linker-section tricks, no
  global-state-in-header pitfalls).
* `oa_test_run.c` — the runner and registry. `test_main.c` calls `oa_test_run()`.
* `test_stubs.c` — tiny stubs for engine symbols (`Com_Error`, `Com_Printf`, …)
  referenced by `q_shared.c`, so the helpers can be tested standalone.
* `test_stubs_nofail.c` — like `test_stubs.c` but `Com_Error` records the error
  code and `longjmp`s back (used by `test_overflow`, `test_parse_nofail`).
* `test_stubs_cm.c` / `test_stubs_cm_nofail.c` — malloc-backed allocator stub
  for `cm_polylib.c` (replaces the engine zone memory manager).

Float/vector assertions take an explicit tolerance (`eps`) because vector math
uses `float` and is not bit-identical across platforms.

## Adding a test

1. Pick or create a `tests_*.c` file.
2. Write `TEST(my_check) { ... OA_ASSERT_*(...); }`.
3. Add the file to `SUITES` in `tests/Makefile` (or a dedicated binary target
   if it needs different stubs, like the `cm` / `nofail` suites).

## Known limitations (documented, not bugs)

* `COM_SkipPath` only recognises `/` as a separator; backslash paths are
  normalised elsewhere in the engine. The test asserts the actual behaviour so
  a regression in `/` handling is caught.
* `COM_Compress` keeps one newline between remaining tokens after stripping a
  `//` comment line.
* `Huff_offsetTransmit` / `Huff_offsetReceive` are intentionally NOT unit-tested:
  without lock-step tree growth they dereference `loc[ch]` with no NYT handling
  and crash. The safe round-trip path is covered via `Huff_Compress` /
  `Huff_Decompress` in `tests_qhuffman.c`.
* `BaseWindingForPlane` with a zero normal does NOT trigger `Com_Error` (the
  `x==-1` guard is unreachable with real floats); the degenerate-input fatal
  path is instead covered via `CheckWinding(<3 points)` in
  `tests_qcm_poly_nofail.c`.
