# Changelog — EMV Terminal Emulator Documentation

## Unreleased

### Added (v2 planning bundle)

- **FEATURE-CATALOG-v2.md** — master index of 39 post-MVP features (F-001–F-040)
- **IMPLEMENTATION-PLAN-v2.md** — milestones M7–M14, repo structure, waves A–D
- **MILESTONES-v2.md** — release tags emv-term-v2.0–v2.3
- **TEST-PLAN-v2-manual.md** — 130+ manual tests (MAN-V2-*)
- **TEST-PLAN-v2-automated.md** — 180+ automated tests (AUTO-V2-*)
- **QA-CHECKLIST-v2.md** — pre-release sign-off for v2 tags
- **SPEC-v2-host-online.md** — host-sim, ARQC/ARPC, TCP acquirer, online PIN
- **SPEC-v2-scheme-kernels.md** — profiles, kernels, MSD, test card matrix
- **SPEC-v2-cvm-pin.md** — interactive PIN, zeroization audit
- **SPEC-v2-scripts-data.md** — scripts 71/72, session merge, redaction, viewer
- **SPEC-v2-cli-ux.md** — CLI overrides, legal banner, capabilities
- **SPEC-v2-oda-crypto.md** — CAPK extra, fDDA, terminal CDA
- **SPEC-v2-restrictions-risk.md** — exception file
- **SPEC-v2-testing-ci.md** — mock APDU, golden fixtures, exec parity, PM3Easy CI
- **SPEC-v2-integration.md** — emv sim bridge, Lua, reader, contact
- **SPEC-v2-trace-replay.md** — PCAP, replay, phase timing
- **examples/TEST-CARD-MATRIX.md** — lab test card checklist
- **examples/host_sim_interac.json** — host-sim key template
- **examples/scheme_profile_interac.json** — scheme profile template
- **examples/exception_file_sample.txt**
- **client/src/emv/test/fixtures/** — golden fixture layout + templates

### Changed

- README.md — v2 program section, updated priorities and document map

### Implementation (prior unreleased commit)

- Terminal phases 2, 4–8 shipped in client (`emv terminal` full pipeline)

## 1.0.0 (planned)

- EMV terminal emulator MVP shipped per Milestones 1–6
- `doc/emv_notes.md` updated with terminal commands

## 0.9.0 (docs)

- Initial planning bundle: SPEC-*, IMPLEMENTATION-PLAN, TEST-PLAN, scheme reference
