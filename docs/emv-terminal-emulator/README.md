# EMV Terminal Emulator — Planning Documentation

## Plain-English Summary

This documentation package defines how to extend the Proxmark3 Iceman firmware and client with an **EMV payment terminal emulator** for **PM3GENERIC** builds, with **PM3Easy** as the primary target hardware. The PM3 acts as the terminal (reader side): it selects applications, reads card data, performs offline authentication, runs cardholder verification including PIN, executes Generate AC, and optionally completes online steps. Card-side emulation (`emv scan` / `emv sim`) already exists in partial form; this project completes the **terminal-side** flow.

The design is client-heavy to respect PM3Easy flash limits (256 KB option), reuses the existing C EMV stack in `client/src/emv/`, and borrows phase structure from [ntufar/EMV](https://github.com/ntufar/EMV) without importing C++ into firmware.

## Current Status

| Area | Status |
|------|--------|
| Planning docs | **Draft — this bundle** |
| Client EMV reader commands (`emv exec`, `emv scan`, `emv reader`) | **Partial — exists today** |
| PIN verification (VERIFY / enciphered PIN) | **Not implemented** |
| Full terminal phase engine (TAA, CAA, AC2, issuer scripts) | **Not implemented** |
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

## Repository / Document Map

```text
docs/emv-terminal-emulator/
├── README.md                      ← you are here
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

1. **M1 — Terminal MVP on client only** — `emv terminal run` wrapping existing `exec` steps plus explicit session object and outcome reporting.
2. **M2 — PIN / CVM** — VERIFY command, enciphered PIN block formatting, TVR/CVMR updates.
3. **M3 — TAA + AC2** — full offline decision path through second Generate AC.
4. **M4 — PM3Easy firmware validation** — confirm default PM3GENERIC image still fits; add `SKIP_*` profile if needed.
5. **M5 — Online lab stub** — mock host ARPC for test cards.

## Known Gaps

- No EMVCo kernel certification path (lab/research tool only).
- Issuer keys for ARQC/ARPC validation require test-card material or host simulator.
- Contact chip (`-w wired`) needs physical smartcard mod on generic PM3.
- Full scheme-specific kernels (Visa qVSDC vs MC M/Chip nuances) need per-AID profiles.
- `emvsim.c` today implements relay/card-side, not terminal-side — must not be conflated.

## Quick Start (after implementation)

```bash
# PM3Easy typical Makefile.platform
PLATFORM=PM3GENERIC
LED_ORDER=PM3EASY

make -j && make client/client

# Lab terminal transaction (contactless)
./pm3 -- emv terminal run -j -a -t --amount 100

# PIN verify step (once implemented)
./pm3 -- emv terminal pin --offline 1234

# Scan then emulate card (existing)
./pm3 -- emv scan -at card.json
```

See [IMPLEMENTATION-PLAN.md](./IMPLEMENTATION-PLAN.md) for the full build sequence.
