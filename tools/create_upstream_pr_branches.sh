#!/usr/bin/env bash
# Create stacked upstream PR branches from integrated fork master.
# Usage: ./tools/create_upstream_pr_branches.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

INTEGRATED="${INTEGRATED_BRANCH:-master}"
UP="${UPSTREAM_REF:-upstream/master}"

git fetch upstream master 2>/dev/null || true
git fetch origin "$INTEGRATED" 2>/dev/null || true

if ! git rev-parse "$UP" >/dev/null 2>&1; then
    echo "Missing $UP — add remote: git remote add upstream https://github.com/RfidResearchGroup/proxmark3.git"
    exit 1
fi

TERMINAL_3A=(
    emv/terminal/emv_term_ctx.c
    emv/terminal/emv_term_ctx.h
    emv/terminal/emv_term_profile.c
    emv/terminal/emv_term_profile.h
    emv/terminal/emv_term_scheme.c
    emv/terminal/emv_term_scheme.h
    emv/terminal/emv_term_session.c
    emv/terminal/emv_term_session.h
    emv/terminal/emv_term_session_view.c
    emv/terminal/emv_term_session_view.h
    emv/terminal/emv_term_tvr.c
    emv/terminal/emv_term_tvr.h
    emv/terminal/emv_term_load.c
    emv/terminal/emv_term_load.h
    emv/terminal/emv_transaction.c
    emv/terminal/emv_transaction.h
    emv/terminal/phase_init.c
    emv/terminal/phase_init.h
    emv/terminal/phase_oda.c
    emv/terminal/phase_oda.h
    emv/terminal/phase_restrict.c
    emv/terminal/phase_restrict.h
    emv/terminal/phase_cvm.c
    emv/terminal/phase_cvm.h
    emv/terminal/phase_trm.c
    emv/terminal/phase_trm.h
    emv/terminal/phase_taa.c
    emv/terminal/phase_taa.h
    emv/terminal/phase_caa.c
    emv/terminal/phase_caa.h
    emv/terminal/phase_complete.c
    emv/terminal/phase_complete.h
    emv/terminal/phase_scripts.c
    emv/terminal/phase_scripts.h
    emv/terminal/emv_term_mock.c
    emv/terminal/emv_term_mock.h
    emv/terminal/emv_term_secure.c
    emv/terminal/emv_term_secure.h
    emv/terminal/emv_term_exception.c
    emv/terminal/emv_term_exception.h
    emv/terminal/emv_term_redact.c
    emv/terminal/emv_term_redact.h
    emv/terminal/emv_term_tlv.c
    emv/terminal/emv_term_tlv.h
    emv/terminal/emv_term_reader_session.c
    emv/terminal/emv_term_reader_session.h
    emv/test/terminal_taa_test.c
    emv/test/terminal_taa_test.h
    emv/test/terminal_cvm_test.c
    emv/test/terminal_cvm_test.h
    emv/test/terminal_exception_test.c
    emv/test/terminal_exception_test.h
    emv/terminal/emv_term_pcap.c
    emv/terminal/emv_term_pcap.h
    emv/terminal/emv_term_pin_prompt.c
    emv/terminal/emv_term_pin_prompt.h
)

TERMINAL_3B=(
    emv/terminal/emv_terminal.c
    emv/terminal/emv_terminal.h
    emv/terminal/phase_online.c
    emv/terminal/phase_online.h
    emv/terminal/emv_term_arqc.c
    emv/terminal/emv_term_arqc.h
    emv/terminal/emv_term_host.c
    emv/terminal/emv_term_host.h
    emv/terminal/emv_term_host_tcp.c
    emv/terminal/emv_term_host_tcp.h
    emv/terminal/emv_term_golden.c
    emv/terminal/emv_term_golden.h
    emv/terminal/emv_term_sim_export.c
    emv/terminal/emv_term_sim_export.h
    emv/terminal/emv_term_lua.c
    emv/terminal/emv_term_lua.h
    emv/terminal/emv_term_banner.c
    emv/terminal/emv_term_banner.h
    emv/terminal/emv_term_replay.c
    emv/terminal/emv_term_replay.h
    emv/terminal/emv_term_timing.c
    emv/terminal/emv_term_timing.h
    emv/terminal/emv_term_probe.c
    emv/terminal/emv_term_probe.h
    emv/terminal/emv_term_crypto.c
    emv/terminal/emv_term_crypto.h
    emv/terminal/emv_term_crypto_digest.c
    emv/terminal/emv_term_crypto_digest.h
    emv/terminal/emv_term_capabilities.c
    emv/terminal/emv_term_capabilities.h
    emv/test/terminal_host_test.c
    emv/test/terminal_host_test.h
    emv/test/terminal_crypto_test.c
    emv/test/terminal_crypto_test.h
    emv/test/terminal_sim_export_test.c
    emv/test/terminal_sim_export_test.h
    emv/test/terminal_pcap_test.c
    emv/test/terminal_pcap_test.h
    emv/test/terminal_replay_test.c
    emv/test/terminal_replay_test.h
)

checkout_terminal_files() {
    local prefix="client/src"
    for f in "$@"; do
        git checkout "$INTEGRATED" -- "$prefix/$f"
    done
}

strip_makefile_terminal_3b() {
    local patterns=(
        'emv/terminal/emv_terminal.c'
        'emv/terminal/phase_online.c'
        'emv/terminal/emv_term_cmd.c'
        'emv/terminal/emv_term_arqc.c'
        'emv/terminal/emv_term_host.c'
        'emv/terminal/emv_term_golden.c'
        'emv/terminal/emv_term_sim_export.c'
        'emv/terminal/emv_term_host_tcp.c'
        'emv/terminal/emv_term_lua.c'
        'emv/terminal/emv_term_banner.c'
        'emv/terminal/emv_term_replay.c'
        'emv/terminal/emv_term_timing.c'
        'emv/terminal/emv_term_probe.c'
        'emv/terminal/emv_term_crypto.c'
        'emv/terminal/emv_term_crypto_digest.c'
        'emv/terminal/emv_term_crypto_cmd.c'
        'emv/terminal/emv_term_capabilities.c'
        'emv/test/terminal_host_test.c'
        'emv/test/terminal_crypto_test.c'
        'emv/test/terminal_sim_export_test.c'
        'emv/test/terminal_pcap_test.c'
        'emv/test/terminal_replay_test.c'
    )
    for p in "${patterns[@]}"; do
        sed -i "/${p//\//\\/}/d" client/Makefile
    done
}

strip_makefile_terminal_cmd() {
    local patterns=(
        'emv/terminal/emv_term_cmd.c'
        'emv/terminal/emv_term_crypto_cmd.c'
    )
    for p in "${patterns[@]}"; do
        sed -i "/${p//\//\\/}/d" client/Makefile
        sed -i "/${p//\//\\/}/d" client/CMakeLists.txt
        sed -i "/${p//\//\\/}/d" client/experimental_lib/CMakeLists.txt
    done
}

write_cryptotest_terminal() {
    local mode="$1"
    git checkout "$UP" -- client/src/emv/test/cryptotest.c client/src/emv/test/cryptotest.h
    python3 - "$mode" <<'PY'
import sys
from pathlib import Path

mode = sys.argv[1]
p = Path("client/src/emv/test/cryptotest.c")
text = p.read_text()

includes = {
    "3a": (
        "#include \"terminal_taa_test.h\"\n"
        "#include \"terminal_cvm_test.h\"\n"
        "#include \"terminal_exception_test.h\"\n"
    ),
    "3b": (
        "#include \"terminal_taa_test.h\"\n"
        "#include \"terminal_host_test.h\"\n"
        "#include \"terminal_cvm_test.h\"\n"
        "#include \"terminal_crypto_test.h\"\n"
        "#include \"terminal_exception_test.h\"\n"
        "#include \"terminal_sim_export_test.h\"\n"
        "#include \"terminal_pcap_test.h\"\n"
        "#include \"terminal_replay_test.h\"\n"
    ),
}
tests = {
    "3a": (
        "    res = exec_terminal_taa_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_cvm_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_exception_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
    ),
    "3b": (
        "    res = exec_terminal_taa_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_host_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_cvm_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_crypto_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_exception_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_sim_export_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_pcap_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
        "    res = exec_terminal_replay_test(verbose);\n"
        "    if (res) TestFail = true;\n\n"
    ),
}

anchor = "#include \"cda_test.h\"\n"
if anchor not in text:
    raise SystemExit("cryptotest.c anchor not found")
text = text.replace(anchor, anchor + includes[mode], 1)

before = "    res = exec_crypto_test(verbose, include_slow_tests);\n"
if before not in text:
    raise SystemExit("cryptotest.c exec_crypto_test anchor not found")
text = text.replace(before, tests[mode] + before, 1)
p.write_text(text)
PY
}

echo "=== PR1: docs usage (operator guide) ==="
git checkout -B cursor/upstream-pr-1-docs-e836 "$UP"
git checkout "$INTEGRATED" -- \
    doc/planning/emv-terminal-emulator/OPERATOR-GUIDE.md \
    doc/emv_pcap_format.md \
    doc/emv_notes.md
mkdir -p doc/planning/emv-terminal-emulator
python3 <<'PY'
from pathlib import Path
# Slim usage README — not the implementation planning bundle
Path("doc/planning/emv-terminal-emulator/README.md").write_text(
    """# EMV terminal emulator (lab use)

> **FOR RESEARCH AND LAB USE ONLY — NO WARRANTY — PROVIDED AS-IS**
>
> This is **not** a certified payment terminal. Use only with authorized EMV test cards.

## How to use

See **[OPERATOR-GUIDE.md](./OPERATOR-GUIDE.md)** for day-to-day commands, workflows, RDV4 3.3 V notes, and safety acknowledgments.

Also:

- Command overview: [doc/emv_notes.md](../../emv_notes.md) (`emv terminal` section)
- PCAP export notes: [doc/emv_pcap_format.md](../../emv_pcap_format.md)

```bash
./pm3 --offline -c 'emv terminal capabilities'
./pm3 --offline -c 'emv terminal help'
```
"""
)
# Strip internal planning-spec cross-links (not shipped in user docs PR)
op = Path("doc/planning/emv-terminal-emulator/OPERATOR-GUIDE.md")
text = op.read_text()
replacements = [
    (
        "See [SPEC-security-privacy.md](./SPEC-security-privacy.md) for PIN handling, redaction, and threat model.",
        "PIN handling: use `--pin` / `EMV_TEST_PIN` for automation; interactive prompt on TTY only. "
        "Session export redacts PAN/crypto by default (`--no-redact` is lab-only).",
    ),
    (
        "Fixtures live in `client/src/emv/test/fixtures/`. See [fixtures README](../../client/src/emv/test/fixtures/README.md).",
        "Fixtures live in `client/src/emv/test/fixtures/` (JSON only). Run `emv terminal test --golden` or `--fixture <name>`.",
    ),
    (
        "Full CLI flags: `emv terminal help` and [SPEC-v2-cli-ux.md](./SPEC-v2-cli-ux.md).",
        "Full CLI flags: run `emv terminal help` or see [doc/emv_notes.md](../../emv_notes.md) (`emv terminal` section).",
    ),
    (
        "## Related documentation\n\n"
        "- [README.md](./README.md) — overview and document map\n"
        "- [SPEC-security-privacy.md](./SPEC-security-privacy.md) — security requirements\n"
        "- [SPEC-v2-trace-replay.md](./SPEC-v2-trace-replay.md) — PCAP and replay\n"
        "- [doc/emv_notes.md](../../doc/emv_notes.md) — all EMV commands\n"
        "- [CHANGELOG.md](./CHANGELOG.md) — feature history\n",
        "## Related documentation\n\n"
        "- [README.md](./README.md) — overview\n"
        "- [doc/emv_notes.md](../../emv_notes.md) — all EMV commands\n"
        "- [doc/emv_pcap_format.md](../../emv_pcap_format.md) — PCAP export format\n"
        "- Terminal profile JSON under `client/resources/` (added in PR 2)\n",
    ),
]
for old, new in replacements:
    text = text.replace(old, new)
if "SPEC-" in text:
    raise SystemExit("OPERATOR-GUIDE still references planning SPEC-* files")
op.write_text(text)
readme = Path("README.md")
rt = readme.read_text()
if "OPERATOR-GUIDE" not in rt and "Notes on EMV" in rt:
    rt = rt.replace(
        "[Notes on EMV](/doc/emv_notes.md)",
        "[Notes on EMV](/doc/emv_notes.md) · [EMV terminal operator guide](/doc/planning/emv-terminal-emulator/OPERATOR-GUIDE.md)",
        1,
    )
    readme.write_text(rt)
cl = Path("CHANGELOG.md")
ct = cl.read_text()
needle = "## [unreleased][unreleased]\n"
insert = "- Added EMV terminal emulator operator guide (lab/research use only; not a certified payment terminal).\n"
if "EMV terminal emulator operator guide" not in ct:
    cl.write_text(ct.replace(needle, needle + insert, 1))
PY
git add doc/planning/emv-terminal-emulator/OPERATOR-GUIDE.md \
    doc/planning/emv-terminal-emulator/README.md \
    doc/emv_pcap_format.md doc/emv_notes.md README.md CHANGELOG.md
git commit -m "docs(emv): add terminal operator guide and usage notes

How-to-use documentation for the lab EMV terminal emulator:
operator workflows, command overview, PCAP notes, and RDV4 3.3 V
hardware caveat. No implementation planning specs in this PR."

echo "=== PR2: user-facing terminal resources ==="
git checkout -B cursor/upstream-pr-2-resources-e836 cursor/upstream-pr-1-docs-e836
git checkout "$INTEGRATED" -- \
    client/resources/emv_terminal_profile.json \
    client/resources/host_sim_interac.json \
    client/resources/interac_test_keys.json \
    client/resources/emv_terminal_profile_interac.json \
    client/resources/terminal_aid_candidates.json \
    client/resources/exception_file_sample.txt \
    client/resources/scheme_profiles
# User-facing resource README (not internal planning notes)
python3 <<'INNER'
from pathlib import Path
Path("client/resources/README-emv-terminal.md").write_text("""# EMV terminal resources (lab use)

Runtime JSON used by `emv terminal` / `emv terminal profile` / host-sim.

> Lab / research only — not for live payment networks.

| File | Used by | Notes |
|------|---------|--------|
| `emv_terminal_profile.json` | `emv terminal … -j` / `profile validate` | Default terminal TLV defaults (amount, country, currency, TTQ, …) |
| `emv_terminal_profile_interac.json` | `--profile interac` | Interac-oriented terminal defaults |
| `scheme_profiles/` | `--profile auto\\|visa\\|mc\\|interac` | Per-scheme TTQ / TAC / policy hints |
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
""")
INNER
# Minimal .gitignore allowlist for these resources only (no fixtures/codeql/docs fork noise)
python3 <<'INNER'
from pathlib import Path
p = Path(".gitignore")
text = p.read_text()
block = """
# EMV terminal lab resources (tracked JSON)
!client/resources/emv_terminal_profile.json
!client/resources/emv_terminal_profile_interac.json
!client/resources/host_sim_interac.json
!client/resources/interac_test_keys.json
!client/resources/terminal_aid_candidates.json
!client/resources/exception_file_sample.txt
!client/resources/scheme_profiles/
!client/resources/scheme_profiles/*.json
!client/resources/README-emv-terminal.md
"""
if "emv_terminal_profile.json" not in text:
    # insert after existing resources json exceptions if present, else before docs section
    anchor = "!client/resources/calypso/*.json\n"
    if anchor in text:
        text = text.replace(anchor, anchor + block.lstrip("\n"), 1)
    else:
        text += "\n" + block
    p.write_text(text)
INNER
git add client/resources .gitignore
git commit -m "chore(emv): add user-facing terminal profiles and scheme resources

Runtime profiles, scheme JSON, and public Interac lab test keys for
operators. No C sources and no internal planning / fixture notes."

echo "=== PR3a: feat/emv-terminal-core-phases ==="
git checkout -B cursor/upstream-pr-3a-phases-e836 cursor/upstream-pr-2-resources-e836
checkout_terminal_files "${TERMINAL_3A[@]}"
git checkout "$INTEGRATED" -- \
    client/src/emv/emvcore.c \
    client/src/emv/emvcore.h \
    client/src/emv/emvjson.c \
    client/src/emv/emvjson.h \
    client/src/emv/emv_pk.c \
    client/src/emv/emv_pk.h \
    client/src/iso7816/iso7816core.c \
    client/Makefile
strip_makefile_terminal_3b
write_cryptotest_terminal 3a
git checkout "$INTEGRATED" -- client/src/emv/test/terminal_test_util.h
# Golden/unit-test fixture JSON only (no internal README / template notes)
git checkout "$INTEGRATED" -- client/src/emv/test/fixtures
find client/src/emv/test/fixtures -name 'README.md' -delete
find client/src/emv/test/fixtures -name '*.template' -delete
# gitignore allowlist for fixtures
python3 <<'INNER'
from pathlib import Path
p = Path(".gitignore")
text = p.read_text()
if "emv/test/fixtures" not in text:
    text += "\n!client/src/emv/test/fixtures/\n!client/src/emv/test/fixtures/**/*.json\n"
    p.write_text(text)
INNER
git add -A
git commit -m "feat(emv): terminal emulator phase engine and offline unit tests

Adds phase pipeline (init through CAA/complete), session/profile/scheme
loaders, mock APDU path, and terminal_taa/cvm/exception self-tests.
Full terminal orchestrator, online host path, and crypto lab land in
follow-up PRs 3b and 4."

echo "=== PR3b: feat/emv-terminal-core-host-crypto ==="
git checkout -B cursor/upstream-pr-3b-host-crypto-e836 cursor/upstream-pr-3a-phases-e836
checkout_terminal_files "${TERMINAL_3B[@]}"
git checkout "$INTEGRATED" -- \
    client/src/scripting.c \
    client/luascripts/emv_terminal_demo.lua \
    client/Makefile \
    client/CMakeLists.txt \
    client/experimental_lib/CMakeLists.txt
strip_makefile_terminal_cmd
write_cryptotest_terminal 3b
git add -A
git commit -m "feat(emv): host simulator, golden runner, and crypto playground core

Adds online phase, host/TCP acquirer, ARQC/ARPC, golden fixtures runner,
crypto lab internals, Lua hooks, and remaining terminal self-tests.
User-facing emv terminal CLI commands land in PR 4."

echo "=== PR4: feat/emv-terminal-cli ==="
git checkout -B cursor/upstream-pr-4-cli-e836 cursor/upstream-pr-3b-host-crypto-e836
git checkout "$INTEGRATED" -- \
    client/src/emv/terminal/emv_term_cmd.c \
    client/src/emv/terminal/emv_term_cmd.h \
    client/src/emv/terminal/emv_term_crypto_cmd.c \
    client/src/emv/terminal/emv_term_crypto_cmd.h \
    client/src/emv/cmdemv.c \
    client/src/proxmark3.c \
    CHANGELOG.md \
    README.md \
    tools/pm3_tests.sh \
    .github/workflows/ubuntu.yml \
    .github/workflows/macos.yml \
    .github/workflows/windows.yml \
    .github/codeql/codeql-config.yml \
    .github/workflows/codeql-analysis.yml \
    client/src/emv/test/cryptotest.c \
    client/src/emv/test/cryptotest.h \
    client/Makefile \
    client/CMakeLists.txt \
    client/experimental_lib/CMakeLists.txt
# Field activation / protocol hooks if present
for f in armsrc/iso14443b.c include/protocols.h include/iso14b.h client/src/cmdhf14b.c client/src/ui.h client/resources/aidlist.json client/resources/capk.txt client/resources/emv_defparams.json; do
    if git diff --name-only "$UP" "$INTEGRATED" -- "$f" | grep -q .; then
        git checkout "$INTEGRATED" -- "$f"
    fi
done
git add -A
git commit -m "feat(emv): emv terminal CLI and CI fixes

User-facing emv terminal command tree, crypto playground CLI, offline
test hooks, MinGW-safe strings, cmake source sync, and CodeQL tuning for
historic EMV interop algorithms. Operator guide is in PR 1."

echo "=== Done. Branches:"
git branch --list 'cursor/upstream-pr-*'
