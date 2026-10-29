# COMM4-02 frozen design vectors

Contract revision: `00434634-0002-4000-8000-202610290001`.
Authority: [wire W0–W8](../../../docs/architecture/communication-v4/wire.md),
[freeze report](../../../docs/architecture/communication-v4/freeze-comm4-02.md).
These files do not implement a production codec, server, ledger or robot adapter.

`manifest.json` lists every golden frame's length, SHA256, expected outcome,
request fingerprint or terminal digest. `golden/*.bin` and matching `.hex` contain
the complete frame including zero padding. All `.hex` are one lowercase hex line.
82 frames cover all16 requests/all9 responses, HELLO discovery/exact contract,
station1/2/3, all13 catalog categories, remote selection/Auto/Stop/Abort, holdout
SAMPLE, result/ACK identity, pose identity/nontrivial/inverse/missing and negative
boundary vectors. `bad_*` frames intentionally violate the frozen contract.
`stale_*` are valid bytes under a stale simulated active snapshot/session.
The signed/UUID word reversal cases demonstrate decoded identity/value changes;
they are not universally malformed bytes.

`plc-words.csv` is an independent whole-frame table: byteOffset aligned4,
unsigned uint32 hexadecimal/decimal and two unsigned uint16 halves high-first.
PLC memory/register addresses and controller byte swapping must be configured
explicitly by the integrator. For the trigger requestId above2^53 the words are
`00200000 00000001`; INT_MIN/MAX are `80000000 7fffffff`; peer UUID words are
`00112233 44554677 8899aabb 00000002`. UUIDs never pass through Float64 or a native
GUID struct. Word swaps are not auto-detected or corrected.

`expected-fields.json` lists independent decoded literal anchors for all message kinds.
`semantic-vectors.json` carries rational conversion expected values, quaternion
canonicalization, stream split/coalescing, deadline/limit and admission/ACK
expectations. Numeric limits use exact rational conversion before ties-to-even;
65byte text rejects before copying. A64byte text uses32 UTF8 `é` characters,
so character count cannot replace byte count. Inverse pose ground truth is a
120degree cyclic-axis rotation of translation(1,2,3), inverse(-2,-3,-1).

Run from the repository root:

```powershell
python tests/fixtures/communication-v4/verify.py
```

Verification reads committed goldens; it never invokes generation or imports its
helpers. It uses independent layout widths, hex/word arithmetic, literal anchors,
exact rational rounding and a bounded negative oracle. It checks the W1/W4/W5
document offset tables for contiguous exact widths. It reconstructs split streams;
it does not run a TCP framer or timing simulator. Future implementation cards must
test actual bounded framing, admission, persistence and socket behavior against
these expectations. No fixture is physical or controller compatibility evidence.

Deliberate regeneration (review changed manifest/data/digests together):

```powershell
python tests/fixtures/communication-v4/generate.py
python tests/fixtures/communication-v4/verify.py --write-words
python tests/fixtures/communication-v4/verify.py
```

Generation uses `struct` network packing and absolute offsets. The second oracle
generates PLC rows from hex halves, not packing. SHA256 is additionally cross-checked
with .NET in the task evidence, including terminal preimages and request fingerprints.
Changing an expected byte/enum/meaning after this freeze requires a new contract
revision; regeneration alone does not authorize a contract change.
