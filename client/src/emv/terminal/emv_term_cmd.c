//-----------------------------------------------------------------------------
// Copyright (C) Proxmark3 contributors. See AUTHORS.md for details.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// See LICENSE.txt for the text of the license.
//-----------------------------------------------------------------------------
// EMV terminal emulator — CLI commands
//-----------------------------------------------------------------------------

#include "emv_term_cmd.h"
#include "emv_terminal.h"
#include "emv_term_profile.h"
#include "emv_term_session.h"
#include "phase_cvm.h"
#include "cliparser.h"
#include "cmdparser.h"
#include "proxmark3.h"
#include "ui.h"
#include "iso7816/iso7816core.h"
#include <string.h>
#include <stdlib.h>

static int CmdHelp(const char *Cmd);

static void print_channel(Iso7816CommandChannel channel) {
    switch (channel) {
        case CC_CONTACTLESS:
            PrintAndLogEx(INFO, "Selected channel... " _GREEN_("CONTACTLESS (T=CL)"));
            break;
        case CC_CONTACT:
            PrintAndLogEx(INFO, "Selected channel... " _GREEN_("CONTACT"));
            break;
    }
}

static int parse_tr_type(CLIParserContext *ctx, int qvsdc_idx, int cda_idx, int vsdc_idx, TransactionType_t *tr_type) {
    *tr_type = TT_MSD;
    if (arg_get_lit(ctx, qvsdc_idx)) {
        *tr_type = TT_QVSDCMCHIP;
    }
    if (arg_get_lit(ctx, cda_idx)) {
        *tr_type = TT_CDA;
    }
    if (arg_get_lit(ctx, vsdc_idx)) {
        *tr_type = TT_VSDC;
    }
    return PM3_SUCCESS;
}

static int parse_common_exec_args(CLIParserContext *ctx, emv_term_cli_opts_t *opts,
                                  int select_idx, int apdu_idx, int tlv_idx, int jload_idx,
                                  int force_idx, int qvsdc_idx, int cda_idx, int vsdc_idx,
                                  int acgpo_idx, int wired_idx) {
    memset(opts, 0, sizeof(*opts));
    opts->activate_field = arg_get_lit(ctx, select_idx);
    opts->show_apdu = arg_get_lit(ctx, apdu_idx);
    opts->decode_tlv = arg_get_lit(ctx, tlv_idx);
    opts->param_load_json = arg_get_lit(ctx, jload_idx);
    opts->force_search = arg_get_lit(ctx, force_idx);
    opts->gen_ac_gpo = arg_get_lit(ctx, acgpo_idx);
    opts->channel = arg_get_lit(ctx, wired_idx) ? CC_CONTACT : CC_CONTACTLESS;
    return parse_tr_type(ctx, qvsdc_idx, cda_idx, vsdc_idx, &opts->tr_type);
}

static emv_term_phase_t parse_phase_name(const char *name) {
    for (emv_term_phase_t p = EMV_PHASE_INIT; p < EMV_PHASE_COUNT; p++) {
        if (strcmp(name, emv_term_phase_name(p)) == 0) {
            return p;
        }
    }
    return EMV_PHASE_COUNT;
}

static int CmdEMVTerminalRun(const char *Cmd) {
    CLIParserContext *ctx;
    CLIParserInit(&ctx, "emv terminal run",
                  "Execute EMV terminal phase loop through init, ODA, CVM, and GEN AC1",
                  "emv terminal run -satj    -> select, show APDU/TLV, load terminal profile\n"
                  "emv terminal run -j --pin 1234 -o /tmp/session.json --qvsdc\n"
                  "emv terminal run -j --trace-phases --stop-after cvm\n");

    void *argtable[] = {
        arg_param_begin,
        arg_lit0("s",  "select",   "Activate field and select card"),
        arg_lit0("a",  "apdu",     "Show APDU requests and responses"),
        arg_lit0("t",  "tlv",      "TLV decode results"),
        arg_lit0("j",  "jload",    "Load terminal profile (emv_terminal_profile.json)"),
        arg_lit0(NULL, "force",    "Force search AID instead of PPSE"),
        arg_rem("By default:",     "Transaction type - MSD"),
        arg_lit0(NULL, "qvsdc",    "Transaction type - qVSDC or M/Chip"),
        arg_lit0("c",  "qvsdccda", "Transaction type - qVSDC/M/Chip plus CDA"),
        arg_lit0("x",  "vsdc",     "Transaction type - VSDC (contact test path)"),
        arg_lit0("g",  "acgpo",    "VISA: generate AC from GPO"),
        arg_lit0("w",  "wired",    "Contact (ISO7816) interface"),
        arg_str0("o",  "output",   "<file>", "Session JSON output path"),
        arg_str0(NULL, "pin",      "<digits>", "Offline PIN (lab only; prefer EMV_TEST_PIN env)"),
        arg_str0(NULL, "profile",  "<file>", "Terminal profile JSON path"),
        arg_str0(NULL, "stop-after", "<phase>", "Stop after named phase (init|oda|cvm|caa|...)"),
        arg_lit0(NULL, "trace-phases", "Log phase boundaries"),
        arg_lit0(NULL, "cvm-skip-online", "Skip online PIN CVM (set TVR bit only)"),
        arg_param_end
    };
    CLIExecWithReturn(ctx, Cmd, argtable, true);

    emv_term_cli_opts_t opts;
    parse_common_exec_args(ctx, &opts, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11);

    opts.output_session = arg_get_str(ctx, 12)->sval[0];
    opts.pin = arg_get_str(ctx, 13)->sval[0];
    opts.profile_path = arg_get_str(ctx, 14)->sval[0];
    opts.stop_after = arg_get_str(ctx, 15)->sval[0];
    opts.trace_phases = arg_get_lit(ctx, 16);
    opts.cvm_skip_online = arg_get_lit(ctx, 17);
    opts.use_terminal_profile = opts.param_load_json;
    CLIParserFree(ctx);

    print_channel(opts.channel);

    if (IfPm3Smartcard() == false && opts.channel == CC_CONTACT) {
        PrintAndLogEx(WARNING, "PM3 does not have SMARTCARD support. Exiting.");
        return PM3_EDEVNOTSUPP;
    }

    emv_term_ctx_t term_ctx;
    int res = emv_term_ctx_init(&term_ctx, &opts);
    if (res) {
        return res;
    }

    res = emv_terminal_run(&term_ctx);
    emv_term_ctx_free(&term_ctx);
    return res;
}

static int CmdEMVTerminalStep(const char *Cmd) {
    CLIParserContext *ctx;
    CLIParserInit(&ctx, "emv terminal step",
                  "Run a single terminal phase",
                  "emv terminal step init -satj\n"
                  "emv terminal step cvm --session /tmp/s.json --pin 1234\n");

    void *argtable[] = {
        arg_param_begin,
        arg_str1(NULL, NULL, "<phase>", "Phase: init|oda|restrict|cvm|trm|taa|caa|online|complete"),
        arg_lit0("s",  "select",   "Activate field and select card"),
        arg_lit0("a",  "apdu",     "Show APDU requests and responses"),
        arg_lit0("t",  "tlv",      "TLV decode results"),
        arg_lit0("j",  "jload",    "Load terminal profile"),
        arg_lit0(NULL, "force",    "Force search AID"),
        arg_lit0(NULL, "qvsdc",    "Transaction type - qVSDC or M/Chip"),
        arg_lit0("c",  "qvsdccda", "Transaction type - CDA"),
        arg_lit0("x",  "vsdc",     "Transaction type - VSDC"),
        arg_lit0("g",  "acgpo",    "Generate AC from GPO"),
        arg_lit0("w",  "wired",    "Contact interface"),
        arg_str0(NULL, "session", "<file>", "Session file for state carry-over"),
        arg_str0(NULL, "pin",      "<digits>", "PIN for cvm phase"),
        arg_str0("o",  "output",   "<file>", "Updated session JSON path"),
        arg_param_end
    };
    CLIExecWithReturn(ctx, Cmd, argtable, true);

    const char *phase_name = arg_get_str(ctx, 1)->sval[0];
    emv_term_phase_t phase = parse_phase_name(phase_name);
    if (phase >= EMV_PHASE_COUNT) {
        PrintAndLogEx(ERR, "Unknown phase '%s'", phase_name);
        CLIParserFree(ctx);
        return PM3_EINVARG;
    }

    emv_term_cli_opts_t opts;
    parse_common_exec_args(ctx, &opts, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12);
    opts.session_path = arg_get_str(ctx, 13)->sval[0];
    opts.pin = arg_get_str(ctx, 14)->sval[0];
    opts.output_session = arg_get_str(ctx, 15)->sval[0];
    CLIParserFree(ctx);

    print_channel(opts.channel);

    emv_term_ctx_t term_ctx;
    int res = emv_term_ctx_init(&term_ctx, &opts);
    if (res) {
        return res;
    }

    if (opts.session_path && opts.session_path[0] && phase != EMV_PHASE_INIT) {
        emv_term_session_load_json(&term_ctx, opts.session_path);
    }

    SetAPDULogging(opts.show_apdu);
    res = emv_terminal_step(&term_ctx, phase);

    const char *outpath = opts.output_session;
    if (!outpath || !outpath[0]) {
        outpath = term_ctx.session_file[0] ? term_ctx.session_file : opts.session_path;
    }
    if (outpath && outpath[0]) {
        emv_term_session_save_json(&term_ctx, outpath);
    }

    if (phase != EMV_PHASE_INIT) {
        DropFieldEx(opts.channel);
    }
    SetAPDULogging(false);
    emv_term_ctx_free(&term_ctx);
    return res;
}

static int CmdEMVTerminalPin(const char *Cmd) {
    CLIParserContext *ctx;
    CLIParserInit(&ctx, "emv terminal pin",
                  "Standalone VERIFY PIN for debugging (requires active session/card)",
                  "emv terminal pin --offline 1234\n"
                  "emv terminal pin --offline 1234 --enciphered -w\n");

    void *argtable[] = {
        arg_param_begin,
        arg_str0(NULL, "offline", "<pin>", "Offline PIN digits (4-12)"),
        arg_lit0(NULL, "enciphered", "Use enciphered offline PIN (CVM 04)"),
        arg_lit0("a",  "apdu",     "Show APDU requests and responses"),
        arg_lit0("w",  "wired",    "Contact interface"),
        arg_str0(NULL, "session", "<file>", "Load session from file (optional)"),
        arg_param_end
    };
    CLIExecWithReturn(ctx, Cmd, argtable, true);

    const char *pin = arg_get_str(ctx, 1)->sval[0];
    bool enciphered = arg_get_lit(ctx, 2);
    bool show_apdu = arg_get_lit(ctx, 3);
    bool wired = arg_get_lit(ctx, 4);
    const char *session = arg_get_str(ctx, 5)->sval[0];
    CLIParserFree(ctx);

    if (!pin || !pin[0]) {
        PrintAndLogEx(ERR, "PIN required: use --offline <pin> or EMV_TEST_PIN env var");
        return PM3_EINVARG;
    }

    emv_term_cli_opts_t opts = {0};
    opts.channel = wired ? CC_CONTACT : CC_CONTACTLESS;
    opts.show_apdu = show_apdu;

    emv_term_ctx_t term_ctx;
    int res = emv_term_ctx_init(&term_ctx, &opts);
    if (res) {
        return res;
    }

    if (session && session[0]) {
        emv_term_session_load_json(&term_ctx, session);
    }

    SetAPDULogging(show_apdu);
    res = phase_cvm_verify_pin(&term_ctx, pin, enciphered);
    DropFieldEx(opts.channel);
    SetAPDULogging(false);
    emv_term_ctx_free(&term_ctx);
    return res;
}

static int CmdEMVTerminalProfile(const char *Cmd) {
    CLIParserContext *ctx;
    CLIParserInit(&ctx, "emv terminal profile",
                  "Print or validate terminal profile JSON",
                  "emv terminal profile print\n"
                  "emv terminal profile validate docs/emv-terminal-emulator/examples/emv_terminal_profile.json\n");

    void *argtable[] = {
        arg_param_begin,
        arg_str1(NULL, NULL, "<action>", "Action: print|validate"),
        arg_str0(NULL, NULL, "<file>", "Profile JSON path (optional)"),
        arg_param_end
    };
    CLIExecWithReturn(ctx, Cmd, argtable, true);

    const char *action = arg_get_str(ctx, 1)->sval[0];
    const char *file = arg_get_str(ctx, 2)->sval[0];
    CLIParserFree(ctx);

    if (strcmp(action, "print") == 0) {
        return emv_term_profile_print(file);
    }
    if (strcmp(action, "validate") == 0) {
        return emv_term_profile_validate(file);
    }

    PrintAndLogEx(ERR, "Unknown action '%s' — use print or validate", action);
    return PM3_EINVARG;
}

static int CmdEMVTerminalLoad(const char *Cmd) {
    CLIParserContext *ctx;
    CLIParserInit(&ctx, "emv terminal load",
                  "Import card TLV subset from prior emv scan JSON (offline replay stub)",
                  "emv terminal load scan.json");

    void *argtable[] = {
        arg_param_begin,
        arg_str1(NULL, NULL, "<file>", "Scan JSON file from emv scan"),
        arg_param_end
    };
    CLIExecWithReturn(ctx, Cmd, argtable, true);
    (void)arg_get_str(ctx, 1);
    CLIParserFree(ctx);

    PrintAndLogEx(WARNING, "emv terminal load not yet implemented — use live card with emv terminal run");
    return PM3_ENOTIMPL;
}

static command_t TerminalCommandTable[] = {
    {"help",    CmdHelp,              AlwaysAvailable, "This help"},
    {"run",     CmdEMVTerminalRun,    IfPm3Iso14443,   "Run terminal phase loop (init→ODA→CVM→AC1)"},
    {"step",    CmdEMVTerminalStep,   IfPm3Iso14443,   "Run single terminal phase"},
    {"pin",     CmdEMVTerminalPin,    IfPm3Iso14443,   "Standalone VERIFY PIN"},
    {"profile", CmdEMVTerminalProfile, AlwaysAvailable, "Print or validate terminal profile JSON"},
    {"load",    CmdEMVTerminalLoad,   AlwaysAvailable, "Load card data from scan JSON (stub)"},
    {NULL, NULL, NULL, NULL}
};

static int CmdHelp(const char *Cmd) {
    (void)Cmd;
    CmdsHelp(TerminalCommandTable);
    return PM3_SUCCESS;
}

int CmdEMVTerminal(const char *Cmd) {
    clearCommandBuffer();
    return CmdsParse(TerminalCommandTable, Cmd);
}
