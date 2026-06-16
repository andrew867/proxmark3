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

#include "emv_terminal.h"
#include "phase_init.h"
#include "phase_oda.h"
#include "phase_cvm.h"
#include "emv_transaction.h"
#include "emv_term_session.h"
#include "comms.h"
#include "ui.h"
#include <string.h>

static int phase_restrict_run(emv_term_ctx_t *ctx) {
    (void)ctx;
    return PM3_ENOTIMPL;
}

static int phase_trm_run(emv_term_ctx_t *ctx) {
    (void)ctx;
    return PM3_ENOTIMPL;
}

static int phase_taa_run(emv_term_ctx_t *ctx) {
    (void)ctx;
    return PM3_ENOTIMPL;
}

static int phase_caa_run(emv_term_ctx_t *ctx) {
    return emv_transaction_genac1(ctx);
}

static int phase_online_run(emv_term_ctx_t *ctx) {
    (void)ctx;
    return PM3_ENOTIMPL;
}

static int phase_complete_run(emv_term_ctx_t *ctx) {
    (void)ctx;
    return PM3_SUCCESS;
}

static bool stop_after_phase(const emv_term_ctx_t *ctx, emv_term_phase_t phase) {
    if (!ctx->opts.stop_after || !ctx->opts.stop_after[0]) {
        return false;
    }
    return strcmp(ctx->opts.stop_after, emv_term_phase_name(phase)) == 0;
}

int emv_terminal_step(emv_term_ctx_t *ctx, emv_term_phase_t phase) {
    if (!ctx) {
        return PM3_EINVARG;
    }

    int res = PM3_ENOTIMPL;
    uint16_t sw = 0;

    ctx->current_phase = phase;

    switch (phase) {
        case EMV_PHASE_INIT:
            res = phase_init_run(ctx);
            break;
        case EMV_PHASE_ODA:
            res = phase_oda_run(ctx);
            break;
        case EMV_PHASE_RESTRICT:
            res = phase_restrict_run(ctx);
            break;
        case EMV_PHASE_CVM:
            res = phase_cvm_run(ctx);
            break;
        case EMV_PHASE_TRM:
            res = phase_trm_run(ctx);
            break;
        case EMV_PHASE_TAA:
            res = phase_taa_run(ctx);
            break;
        case EMV_PHASE_CAA:
            res = phase_caa_run(ctx);
            break;
        case EMV_PHASE_ONLINE:
            res = phase_online_run(ctx);
            break;
        case EMV_PHASE_COMPLETE:
            res = phase_complete_run(ctx);
            break;
        case EMV_PHASE_COUNT:
        default:
            return PM3_EINVARG;
    }

    emv_term_event_add(ctx, phase, res, sw, NULL);
    return res;
}

int emv_terminal_run(emv_term_ctx_t *ctx) {
    if (!ctx) {
        return PM3_EINVARG;
    }

    emv_term_phase_t pipeline[] = {
        EMV_PHASE_INIT,
        EMV_PHASE_ODA,
        EMV_PHASE_CVM,
        EMV_PHASE_CAA,
        EMV_PHASE_COMPLETE,
    };

    int last_res = PM3_SUCCESS;

    for (size_t i = 0; i < sizeof(pipeline) / sizeof(pipeline[0]); i++) {
        emv_term_phase_t phase = pipeline[i];
        int res = emv_terminal_step(ctx, phase);
        if (res == PM3_ENOTIMPL && phase != EMV_PHASE_RESTRICT && phase != EMV_PHASE_TRM &&
                phase != EMV_PHASE_TAA && phase != EMV_PHASE_ONLINE) {
            last_res = res;
            ctx->outcome = EMV_OUTCOME_ABORTED;
            break;
        }
        if (res && res != PM3_ENOTIMPL) {
            if (phase == EMV_PHASE_CVM && res == PM3_ESOFT) {
                PrintAndLogEx(WARNING, "CVM phase failed — continuing to GEN AC1 for lab visibility");
            } else {
                last_res = res;
                ctx->outcome = EMV_OUTCOME_ABORTED;
                break;
            }
        }
        if (stop_after_phase(ctx, phase)) {
            PrintAndLogEx(INFO, "Stopped after phase: %s", emv_term_phase_name(phase));
            break;
        }
        last_res = PM3_SUCCESS;
    }

    if (ctx->outcome == EMV_OUTCOME_UNKNOWN) {
        uint8_t cid = 0;
        if (tlvdb_get_uint8(ctx->card, 0x9f27, &cid)) {
            ctx->outcome = emv_transaction_outcome_from_cid(cid);
        } else if (ctx->cvm_success) {
            ctx->outcome = EMV_OUTCOME_APPROVED_OFFLINE;
        }
    }

    const char *outpath = ctx->opts.output_session;
    if (!outpath || !outpath[0]) {
        outpath = ctx->session_file[0] ? ctx->session_file : NULL;
    }
    if (outpath && outpath[0]) {
        emv_term_session_save_json(ctx, outpath);
    }

    DropFieldEx(ctx->channel);
    SetAPDULogging(false);

    PrintAndLogEx(SUCCESS, "[+] Terminal outcome: %s", emv_term_outcome_str(ctx->outcome));
    return last_res;
}
