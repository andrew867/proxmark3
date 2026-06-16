# EMV Terminal Emulator — Planning Documentation

## Plain-English Summary

This documentation package defines how to extend the Proxmark3 Iceman firmware and client with an **EMV payment terminal emulator** for **PM3GENERIC** builds, with **PM3Easy** as the primary target hardware. The PM3 acts as the terminal (reader side): it selects applications, reads card data, performs offline authentication, runs cardholder verification including PIN, executes Generate AC, and optionally completes online steps. Card-side emulation (`emv scan` / `emv sim`) already exists in partial form; this project completes the **terminal-side** flow.

The design is client-heavy to respect PM3Easy flash limits (256 KB option), reuses the existing C EMV stack in `client/src/emv/`, and borrows phase structure from [ntufar/EMV](https://github.com/ntufar/EMV) without importing C++ into firmware.

## Current Status

| Area | Status |
|------|--------|
| Planning docs | **Complete — this bundle** |
| Client EMV reader commands (`emv exec`, `emv scan`, `emv reader`) | **Partial — exists today** |
| Terminal phase engine (`emv terminal run/step`) | **Implemented** |
| PIN verification (VERIFY / enciphered PIN) | **Implemented (`emv terminal pin`, CVM phase)** |
| Restrictions, TRM, TAA, CAA (GEN AC1) | **Implemented** |
| Online lab stub (EXTERNAL AUTH, AC2, issuer script 71) | **Implemented (`emv terminal online`)** |
| Scan JSON load for offline testing | **Implemented (`emv terminal load`)** |
| Firmware terminal assist (`emvsim.c` is relay/card-side) | **Not implemented** |
| PM3Easy 256 KB firmware fit | **Not validated** |

## Who This Is For

- Proxmark3 developers implementing EMV terminal features
- PM3Easy owners testing contactless EMV in a lab
- Security researchers studying EMV protocol behavior (authorized test cards only)
- Maintainers integrating ntufar/EMV concepts into the Iceman C codebase

## What This System Does

1. **Load terminal profile** — country, currency, floor limits, TACs, capabilities from JSON (`emv_defparams.json` extended).
2. **Present card / read card** — PPSE/AID selection, GPO, AFL record reads over HF 14a (contactless) or smartcard slot (contact, if `SMARTCARD` mod).
3. **Authenticate card** — SDA, DDA, fDDA, CDA using existing `emvcore.c` crypto paths.
4. **Verify cardholder** — offline plaintext PIN, enciphered PIN (ICC public key), online PIN stub; update TVR/CVM Results.
5. **Decide and generate cryptogram** — Terminal Action Analysis → `GENERATE AC` (CDOL1/CDOL2) → parse ARQC/TC/AAC.
6. **Optional online path** — host simulator or manual ARPC entry for lab completion.
7. **Export session** — structured JSON trace for replay, regression, and `emv sim` card dumps.

## Major Components

| Component | Location (planned / existing) | Role |
|-----------|-------------------------------|------|
| Terminal phase engine | `client/src/emv/terminal/` (new) | Orchestrates EMV Book 3 phases |
| Existing EMV core | `client/src/emv/emvcore.c`, `dol.c`, `crypto*.c` | APDU exchange, ODA, DOL |
| CLI commands | `client/src/emv/cmdemv.c` | `emv terminal`, `emv pin`, extensions to `exec` |
| ISO7816 transport | `client/src/iso7816/` | Contactless via PM3 HF; contact via I2C smartcard |
| Firmware field/APDU assist | `armsrc/emvterm.c` (new, optional) | Timing-sensitive relay only if needed |
| Card-side sim (existing) | `armsrc/emvsim.c`, `emv scan` JSON | Out of scope except interoperability |
| Reference architecture | [ntufar/EMV](https://github.com/ntufar/EMV) | Phase model, not direct code import |

## v2 Enhancement Program (Post-MVP)

Milestones **M1–M6 (Phases 0–8)** are implemented in client code. The **v2 program** specifies everything needed to reach a lab-grade, CI-backed, multi-scheme terminal:

| Start here | Purpose |
|------------|---------|
| [FEATURE-CATALOG-v2.md](./FEATURE-CATALOG-v2.md) | Master index of 39 features (F-001–F-040) |
| [IMPLEMENTATION-PLAN-v2.md](./IMPLEMENTATION-PLAN-v2.md) | Milestones M7–M14, waves A–D |
| [MILESTONES-v2.md](./MILESTONES-v2.md) | Release gates and demo scripts |
| [TEST-PLAN-v2-manual.md](./TEST-PLAN-v2-manual.md) | 130+ manual test cases |
| [TEST-PLAN-v2-automated.md](./TEST-PLAN-v2-automated.md) | 180+ automated test IDs |
| [QA-CHECKLIST-v2.md](./QA-CHECKLIST-v2.md) | Pre-release sign-off |

### v2 Technical Specs

| Spec | Features covered |
|------|------------------|
| [SPEC-v2-host-online.md](./SPEC-v2-host-online.md) | Host-sim, ARQC/ARPC, TCP acquirer, online PIN |
| [SPEC-v2-scheme-kernels.md](./SPEC-v2-scheme-kernels.md) | Profiles, kernels, MSD, test matrix |
| [SPEC-v2-cvm-pin.md](./SPEC-v2-cvm-pin.md) | Interactive PIN, zeroization audit |
| [SPEC-v2-scripts-data.md](./SPEC-v2-scripts-data.md) | Scripts 71/72, session merge, redaction |
| [SPEC-v2-cli-ux.md](./SPEC-v2-cli-ux.md) | CLI overrides, banner, capabilities |
| [SPEC-v2-oda-crypto.md](./SPEC-v2-oda-crypto.md) | CAPK, fDDA, terminal CDA |
| [SPEC-v2-restrictions-risk.md](./SPEC-v2-restrictions-risk.md) | Exception file |
| [SPEC-v2-testing-ci.md](./SPEC-v2-testing-ci.md) | Mock APDU, golden fixtures, parity |
| [SPEC-v2-integration.md](./SPEC-v2-integration.md) | emv sim, Lua, reader, contact |
| [SPEC-v2-trace-replay.md](./SPEC-v2-trace-replay.md) | PCAP, replay, timing |

**Deferred:** firmware WTX assist (F-027) — only if contactless timing fails on measured hardware.

---

## Repository / Document Map

```text
docs/emv-terminal-emulator/
├── README.md                      ← you are here
├── FEATURE-CATALOG-v2.md          v2 master feature index
├── IMPLEMENTATION-PLAN-v2.md      M7–M14 build order
├── MILESTONES-v2.md               v2 delivery gates
├── TEST-PLAN-v2-manual.md         v2 manual tests
├── TEST-PLAN-v2-automated.md      v2 CI / unit tests
├── QA-CHECKLIST-v2.md             v2 ship criteria
├── SPEC-v2-*.md                   v2 technical specs (10 files)
├── PRODUCT-OVERVIEW.md            Business scope, staged versions
├── ARCHITECTURE.md                System design, data flow
├── SPEC-core-loop.md              Terminal transaction phases
├── SPEC-data-model.md             Context, JSON schemas, TLV state
├── SPEC-api.md                    CLI commands, USB protocol hooks
├── SPEC-user-flows.md             Operator scenarios
├── SPEC-security-privacy.md       PIN handling, legal use
├── SPEC-error-handling.md         Failure modes and recovery
├── SPEC-device-behavior.md        PM3Easy / PM3GENERIC constraints
├── SPEC-firmware.md               armsrc changes, flash budget
├── SPEC-connectivity.md           USB, field control, smartcard
├── SPEC-field-operations.md       Lab setup, test cards
├── SPEC-schemes-reference.md      Visa/MC/Plus/Cirrus/Interlink/Interac
├── SPEC-cryptography-keys.md      CAPKs, ODA, PIN, ARPC
├── SPEC-advanced-terminal-features.md  CVM, TAA, AC2, issuer scripts
├── IMPLEMENTATION-PLAN.md         Phased build order
├── MILESTONES.md                  Delivery milestones
├── TEST-PLAN-manual.md            Human-run tests
├── TEST-PLAN-automated.md         Unit/integration/CI mapping
├── QA-CHECKLIST.md                Ship criteria
├── RISKS-AND-ASSUMPTIONS.md       Honest risk register
├── OPEN-QUESTIONS.md              Decision log
└── CHANGELOG.md                   Doc change history
```

Related existing docs:

- [doc/emv_notes.md](../../doc/emv_notes.md) — current EMV command inventory
- [doc/md/Use_of_Proxmark/4_Advanced-compilation-parameters.md](../../doc/md/Use_of_Proxmark/4_Advanced-compilation-parameters.md) — PM3GENERIC / PM3Easy build flags

## Current Build Priorities

**Done (M1–M6):** Full terminal phase pipeline, CVM, TAA, online stub, session JSON.

**Next (v2 Wave A — P0):**

1. **M7 — Host simulator** — real ARQC verify + ARPC ([SPEC-v2-host-online.md](./SPEC-v2-host-online.md))
2. **M8 — Scheme profiles** — `--profile interac|visa|mc|auto`
3. **M9 — Mock APDU + golden CI** — no hardware regression ([SPEC-v2-testing-ci.md](./SPEC-v2-testing-ci.md))

See [IMPLEMENTATION-PLAN-v2.md](./IMPLEMENTATION-PLAN-v2.md) for M10–M14.

## Known Gaps

- No EMVCo kernel certification path (lab/research tool only).
- Issuer keys for ARQC/ARPC validation require test-card material or host simulator.
- Contact chip (`-w wired`) needs physical smartcard mod on generic PM3.
- Full scheme-specific kernels (Visa qVSDC vs MC M/Chip nuances) need per-AID profiles.
- `emvsim.c` today implements relay/card-side, not terminal-side — must not be conflated.

## Quick Start

```bash
# PM3Easy typical Makefile.platform
PLATFORM=PM3GENERIC
LED_ORDER=PM3EASY

make -j && make client/client

# Full terminal transaction (contactless)
./pm3 -- emv terminal run -satj -o /tmp/session.json --qvsdc

# PIN verify (lab test cards only)
./pm3 -- emv terminal pin --offline 1234

# Online completion after ARQC
./pm3 -- emv terminal online --session /tmp/session.json --arpc <hex> --arpc-rc 8840

# Load card from prior scan (offline phase testing)
./pm3 -- emv terminal load card.json -o card_session.json

# Scan then emulate card (existing)
./pm3 -- emv scan -at card.json
```

See [IMPLEMENTATION-PLAN-v2.md](./IMPLEMENTATION-PLAN-v2.md) for the v2 build sequence.  
MVP sequence: [IMPLEMENTATION-PLAN.md](./IMPLEMENTATION-PLAN.md).
