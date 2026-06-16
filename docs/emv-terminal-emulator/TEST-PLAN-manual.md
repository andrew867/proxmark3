# Manual Test Plan — EMV Terminal Emulator

## Test Environment

| Item | Value |
|------|-------|
| Device | PM3Easy, PM3GENERIC firmware |
| Host | Ubuntu 22.04+ or macOS, Iceman client built from branch |
| Connection | USB |
| Cards | EMV qVSDC test + offline PIN test card |
| Profile | `client/resources/emv_terminal_profile.json` |

## Setup Steps

1. Flash PM3Easy: `PLATFORM=PM3GENERIC`, `LED_ORDER=PM3EASY`
2. `./client/proxmark3`; run `hw status`
3. Place test card on antenna; verify `hf 14a info`

## Reset Steps

- `hf 14a fieldoff`
- Remove card between tests
- Delete `/tmp/emv_session*.json`

## Core Workflow Tests

| ID | Area | Steps | Expected Result | Status |
|----|------|-------|-----------------|--------|
| MAN-CORE-001 | Init | Run `emv terminal run -j -a -t -o /tmp/s1.json --qvsdc --amount 100` with qVSDC card | Session completes; outcome printed; JSON has Phases | Not Run |
| MAN-CORE-002 | PPSE | Same with Visa card, no `--forceaid` | PPSE select succeeds; AID chosen | Not Run |
| MAN-CORE-003 | AID force | `emv terminal run --forceaid --aid A0000000031010 ...` | Forced AID selected | Not Run |
| MAN-CORE-004 | ODA | Run with `-t` on DDA card | Phase `oda` success in trace; or TVR bit if fail | Not Run |
| MAN-CORE-005 | Exec parity | Compare APDU log `emv exec -sat --qvsdc` vs `emv terminal run -at --qvsdc` | Same APDU sequence through GEN AC1 | Not Run |
| MAN-CORE-010 | PIN ok | After init on PIN card: `emv terminal pin --offline <correct>` | SW 9000; CVMR success | Not Run |
| MAN-CORE-011 | PIN fail | Wrong PIN three times | 63Cx then block or decline per card | Not Run |
| MAN-CORE-012 | PIN prompt | `emv terminal pin` interactive | No echo; VERIFY sent | Not Run |
| MAN-CORE-015 | Decline | Card/profile causing ODA fail + TAC deny | Outcome `declined`, AAC | Not Run |
| MAN-CORE-020 | Online | ARQC outcome + `emv terminal online --arpc ...` | Session completes online path | Not Run |

## Step Mode Tests

| ID | Area | Steps | Expected Result | Status |
|----|------|-------|-----------------|--------|
| MAN-STEP-001 | Step init | `emv terminal step init -j -o /tmp/s.json --qvsdc` | Session file with card TLV | Not Run |
| MAN-STEP-002 | Step oda | `emv terminal step oda --session /tmp/s.json` | ODA phase result recorded | Not Run |

## Device / Platform Tests

| ID | Area | Steps | Expected Result | Status |
|----|------|-------|-----------------|--------|
| MAN-DEVICE-001 | Flash 256 | Build `PLATFORM_SIZE=256` profile | Image ≤ 262144 or documented skips | Not Run |
| MAN-DEVICE-002 | Field stability | 20 APDU transaction without card move | No timeout | Not Run |
| MAN-DEVICE-003 | Button abort | Start run; press PM3 button | Field off; partial session if `-o` | Not Run |
| MAN-DEVICE-004 | No smartcard | `emv terminal run -w` without mod | Immediate `PM3_EDEVNOTSUPP` | Not Run |

## API / CLI Tests

| ID | Area | Steps | Expected Result | Status |
|----|------|-------|-----------------|--------|
| MAN-API-001 | Help | `emv terminal run --help` | Options documented | Not Run |
| MAN-API-002 | Profile | `emv terminal profile validate` bad JSON | Non-zero exit | Not Run |

## Security Tests

| ID | Area | Steps | Expected Result | Status |
|----|------|-------|-----------------|--------|
| MAN-SEC-001 | Log redaction | `emv terminal pin --offline 1234 -a` | APDU log omits PIN digits | Not Run |
| MAN-SEC-002 | JSON mask | Export session default | PAN masked | Not Run |

## Regression Checklist (after each milestone)

- [ ] `emv exec` unchanged behavior
- [ ] `emv scan` produces valid JSON
- [ ] `emv test` crypto tests pass
- [ ] `emv reader` still works
- [ ] `hf 14a` unrelated commands unaffected

## Edge / Failure Tests

| ID | Area | Steps | Expected Result | Status |
|----|------|-------|-----------------|--------|
| MAN-ERR-001 | Card removed | Remove card during readrec | Abort; partial JSON if `-o` | Not Run |
| MAN-ERR-002 | Bad PDOL | Wrong amount format in profile | Clear 6985 message | Not Run |
