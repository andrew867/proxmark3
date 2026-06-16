# Changelog — EMV Terminal Emulator Documentation

## Unreleased

### Added

- SPEC-schemes-reference.md — Visa, MC, Plus, Cirrus, Interlink, Interac AIDs and kernels
- SPEC-cryptography-keys.md — CAPKs, ODA recovery, PIN, ARPC/ARPC-RC
- SPEC-advanced-terminal-features.md — CVM, TAA, AC2, external auth, issuer scripts
- examples/emv_terminal_profile_interac.json, interac_test_keys.json, terminal_aid_candidates.json
- Interac test CAPKs (RID A000000277 index 03/07) in client/resources/capk.txt
- CV_INTERAC vendor enum; US common debit AIDs in emvcore AIDlist

### Changed

- aidlist.json Interac entry with Flash/C-1 documentation and source URL
- TEST-PLAN-manual/automated scheme and advanced feature coverage

### Fixed

- N/A — documentation only

### Removed

- N/A

## 1.0.0 (planned)

- EMV terminal emulator implementation shipped per Milestone 6+
- `doc/emv_notes.md` updated with terminal commands
