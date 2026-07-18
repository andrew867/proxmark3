# EMV terminal resources (lab use)

Runtime JSON used by `emv terminal` / `emv terminal profile` / host-sim.

> Lab / research only — not for live payment networks.

| File | Used by | Notes |
|------|---------|--------|
| `emv_terminal_profile.json` | `emv terminal … -j` / `profile validate` | Default terminal TLV defaults (amount, country, currency, TTQ, …) |
| `emv_terminal_profile_interac.json` | `--profile interac` | Interac-oriented terminal defaults |
| `scheme_profiles/` | `--profile auto\|visa\|mc\|interac` | Per-scheme TTQ / TAC / policy hints |
| `terminal_aid_candidates.json` | AID selection helpers | Candidate AIDs for lab testing |
| `host_sim_interac.json` | `--host-sim` / `emv terminal host-sim` | Local host-sim config (no network) |
| `interac_test_keys.json` | `--host-keys` | **Public** Interac Flash interoperability test pack keys — never live credentials |
| `exception_file_sample.txt` | `--exception-file` | Sample exception-file format for denial testing |

## Quick examples

```bash
./pm3 --offline -c 'emv terminal profile validate'
./pm3 --offline -c 'emv terminal profile validate client/resources/emv_terminal_profile_interac.json'
./pm3 -- emv terminal run -j --profile auto --host-sim --host-keys client/resources/interac_test_keys.json
```

See also [OPERATOR-GUIDE.md](../doc/planning/emv-terminal-emulator/OPERATOR-GUIDE.md).
