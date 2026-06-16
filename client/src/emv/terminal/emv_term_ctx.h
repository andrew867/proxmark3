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
// EMV terminal emulator — session context
//-----------------------------------------------------------------------------

#ifndef EMV_TERM_CTX_H__
#define EMV_TERM_CTX_H__

#include "common.h"
#include "proxmark3.h"
#include "fileutils.h"
#include "../emvcore.h"
#include "../tlv.h"

typedef enum {
    EMV_PHASE_INIT = 0,
    EMV_PHASE_ODA,
    EMV_PHASE_RESTRICT,
    EMV_PHASE_CVM,
    EMV_PHASE_TRM,
    EMV_PHASE_TAA,
    EMV_PHASE_CAA,
    EMV_PHASE_ONLINE,
    EMV_PHASE_COMPLETE,
    EMV_PHASE_COUNT,
} emv_term_phase_t;

typedef enum {
    EMV_OUTCOME_UNKNOWN = 0,
    EMV_OUTCOME_APPROVED_OFFLINE,
    EMV_OUTCOME_DECLINED,
    EMV_OUTCOME_ONLINE_REQUIRED,
    EMV_OUTCOME_ABORTED,
} emv_term_outcome_t;

typedef struct {
    emv_term_phase_t id;
    int result;
    uint16_t sw;
    uint64_t ts_ms;
    char note[128];
} emv_phase_event_t;

typedef struct {
    bool activate_field;
    bool show_apdu;
    bool decode_tlv;
    bool param_load_json;
    bool force_search;
    TransactionType_t tr_type;
    bool gen_ac_gpo;
    Iso7816CommandChannel channel;
    bool trace_phases;
    bool cvm_skip_online;
    bool use_terminal_profile;
    const char *pin;
    const char *output_session;
    const char *stop_after;
    const char *profile_path;
    const char *session_path;
} emv_term_cli_opts_t;

typedef struct emv_term_ctx {
    struct tlvdb *terminal;
    struct tlvdb *card;
    struct tlvdb *select;
    Iso7816CommandChannel channel;
    TransactionType_t tr_type;
    emv_term_outcome_t outcome;
    emv_term_phase_t current_phase;
    emv_phase_event_t *events;
    size_t event_count;
    size_t event_cap;
    uint8_t aid[APDU_AID_LEN];
    size_t aid_len;
    bool oda_performed;
    bool oda_success;
    bool cvm_performed;
    bool cvm_success;
    uint8_t cvm_results[3];
    uint8_t oda_list[4096];
    size_t oda_list_len;
    struct tlv *pdol_data_tlv;
    emv_term_cli_opts_t opts;
    char session_file[FILE_PATH_SIZE];
} emv_term_ctx_t;

const char *emv_term_phase_name(emv_term_phase_t phase);
const char *emv_term_outcome_str(emv_term_outcome_t outcome);

int emv_term_ctx_init(emv_term_ctx_t *ctx, const emv_term_cli_opts_t *opts);
void emv_term_ctx_free(emv_term_ctx_t *ctx);

int emv_term_event_add(emv_term_ctx_t *ctx, emv_term_phase_t phase, int result, uint16_t sw, const char *note);

struct tlvdb *emv_term_get_root(emv_term_ctx_t *ctx);

#endif
