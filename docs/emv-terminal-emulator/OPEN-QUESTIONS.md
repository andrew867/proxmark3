# Open Questions

| ID | Question | Area | Impact | Needed By | Status |
|----|----------|------|--------|-----------|--------|
| OQ-001 | Allocate `CMD_HF_EMV_TERMINAL_ASSIST` (0x0387?) in `pm3_cmd.h`? | Firmware | Low | Phase 8 / if WTX needed | Open |
| OQ-002 | Merge terminal session JSON with `emv scan` format or keep separate? | Data | Medium | Milestone 2 | Open — lean separate + optional merge flag |
| OQ-003 | Implement EMV Entry Point kernel selection (C-2/C-3) or single orchestrator with AID profiles? | Architecture | High | Milestone 4 | Open — prefer AID profiles v1 |
| OQ-004 | Redact ARQC/IAD in shared session traces by default? | Security | Medium | Milestone 2 | Open — redact AC, keep type/ATC |
| OQ-005 | Default transaction type on PM3Easy: MSD, qVSDC, or profile-driven? | Product | Medium | Milestone 2 | Open — default qVSDC |
| OQ-006 | Standalone mode `HF_EMVTERM` for antenna-only field terminal? | Firmware | Low | Post v1 | Open — defer, client-only |
| OQ-007 | Use lumag/emv-tools C snippets for PIN vs clean-room from spec? | Implementation | Medium | Milestone 3 | Open — evaluate license |
| OQ-008 | Support American Express/JCB in v1 or Visa+MC only? | Scope | Medium | Milestone 6 | Open — Visa+MC v1 |
| OQ-009 | Interactive PIN on Windows client — secure no-echo available? | UX | Medium | Milestone 3 | Open — test per platform |
| OQ-010 | Add `--mock-apdu-file` for CI or separate test binary only? | Testing | Low | Milestone 2 | Open |

## Decision Log (fill as resolved)

| ID | Decision | Date | Notes |
|----|----------|------|-------|
| — | Client-heavy architecture for PM3Easy | 2026-06-16 | Documented in ARCHITECTURE.md |
| — | ntufar/EMV: architecture port only, no C++ import | 2026-06-16 | License + style |
