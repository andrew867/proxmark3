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
// EMV terminal emulator — session JSON export
//-----------------------------------------------------------------------------

#include "emv_term_session.h"
#include "../emvjson.h"
#include "ui.h"
#include "util.h"
#include "commonutil.h"
#include <jansson.h>
#include <string.h>

static void mask_pan(char *out, size_t outlen, const uint8_t *pan, size_t panlen) {
    if (!out || outlen == 0) {
        return;
    }
    out[0] = '\0';
    if (!pan || panlen == 0) {
        return;
    }

    size_t pos = 0;
    for (size_t i = 0; i < panlen && pos + 1 < outlen; i++) {
        uint8_t hi = (pan[i] >> 4) & 0x0F;
        uint8_t lo = pan[i] & 0x0F;
        if (hi <= 9) {
            out[pos++] = '0' + hi;
        }
        if (lo <= 9 && pos + 1 < outlen) {
            out[pos++] = '0' + lo;
        }
    }
    out[pos] = '\0';

    if (pos > 10) {
        for (size_t i = 6; i < pos - 4; i++) {
            out[i] = '*';
        }
    }
}

int emv_term_session_save_json(const emv_term_ctx_t *ctx, const char *path) {
    if (!ctx || !path || !path[0]) {
        return PM3_EINVARG;
    }

    json_t *root = json_object();
    json_t *file = json_object();
    json_t *terminal = json_object();
    json_t *phases = json_array();
    json_t *card = json_object();
    json_t *crypto = json_object();

    JsonSaveStr(file, "Created", "proxmark3 emv terminal");
    JsonSaveStr(file, "Version", "1");
    json_object_set_new(root, "File", file);

    JsonSaveStr(terminal, "Profile", ctx->opts.profile_path ? ctx->opts.profile_path : "default");
    JsonSaveStr(terminal, "Channel", ctx->channel == CC_CONTACT ? "contact" : "contactless");
    json_object_set_new(root, "Terminal", terminal);

    JsonSaveStr(root, "Outcome", emv_term_outcome_str(ctx->outcome));
    JsonSaveStr(root, "TransactionType", TransactionTypeStr[ctx->tr_type]);

    for (size_t i = 0; i < ctx->event_count; i++) {
        json_t *pe = json_object();
        JsonSaveInt(pe, "id", ctx->events[i].id);
        JsonSaveStr(pe, "name", emv_term_phase_name(ctx->events[i].id));
        JsonSaveInt(pe, "result", ctx->events[i].result);
        JsonSaveHex(pe, "sw", ctx->events[i].sw, 2);
        if (ctx->events[i].note[0]) {
            JsonSaveStr(pe, "notes", ctx->events[i].note);
        }
        json_array_append_new(phases, pe);
    }
    json_object_set_new(root, "Phases", phases);

    if (ctx->aid_len) {
        JsonSaveBufAsHexCompact(card, "AID", (uint8_t *)ctx->aid, ctx->aid_len);
    }

    const struct tlv *pan = tlvdb_get(ctx->card, 0x5a, NULL);
    if (pan && pan->len) {
        char masked[32] = {0};
        mask_pan(masked, sizeof(masked), pan->value, pan->len);
        JsonSaveStr(card, "PAN", masked);
    }

    const struct tlv *cvmres = tlvdb_get(ctx->card, 0x9f34, NULL);
    if (cvmres && cvmres->len == 3) {
        JsonSaveBufAsHexCompact(card, "CVMResults", (uint8_t *)cvmres->value, cvmres->len);
    }

    json_object_set_new(root, "Card", card);

    const struct tlv *cid = tlvdb_get(ctx->card, 0x9f27, NULL);
    const struct tlv *ac = tlvdb_get(ctx->card, 0x9f26, NULL);
    const struct tlv *atc = tlvdb_get(ctx->card, 0x9f36, NULL);

    if (cid && cid->len) {
        uint8_t ctype = cid->value[0] & 0xC0;
        if (ctype == 0x00) {
            JsonSaveStr(crypto, "Type", "AAC");
        } else if (ctype == 0x40) {
            JsonSaveStr(crypto, "Type", "TC");
        } else if (ctype == 0x80) {
            JsonSaveStr(crypto, "Type", "ARQC");
        }
    }
    if (atc && atc->len) {
        JsonSaveBufAsHexCompact(crypto, "ATC", (uint8_t *)atc->value, atc->len);
    }
    if (ac && ac->len) {
        JsonSaveBufAsHexCompact(crypto, "AC", (uint8_t *)ac->value, ac->len);
    }
    json_object_set_new(root, "Cryptogram", crypto);

    int res = json_dump_file(root, path, JSON_INDENT(2));
    json_decref(root);

    if (res) {
        PrintAndLogEx(ERR, "Failed to write session JSON: %s", path);
        return PM3_ESOFT;
    }

    PrintAndLogEx(SUCCESS, "Session saved: %s", path);
    return PM3_SUCCESS;
}

int emv_term_session_load_json(emv_term_ctx_t *ctx, const char *path) {
    if (!ctx || !path || !path[0]) {
        return PM3_EINVARG;
    }

    json_error_t error;
    json_t *root = json_load_file(path, 0, &error);
    if (!root) {
        PrintAndLogEx(ERR, "Session load error line %d: %s", error.line, error.text);
        return PM3_ESOFT;
    }

    char outcome[32] = {0};
    if (JsonLoadStr(root, "Outcome", outcome) == 0) {
        if (strcmp(outcome, "approved_offline") == 0) {
            ctx->outcome = EMV_OUTCOME_APPROVED_OFFLINE;
        } else if (strcmp(outcome, "declined") == 0) {
            ctx->outcome = EMV_OUTCOME_DECLINED;
        } else if (strcmp(outcome, "online_required") == 0) {
            ctx->outcome = EMV_OUTCOME_ONLINE_REQUIRED;
        } else if (strcmp(outcome, "aborted") == 0) {
            ctx->outcome = EMV_OUTCOME_ABORTED;
        }
    }

    json_t *phases = json_object_get(root, "Phases");
    if (json_is_array(phases)) {
        size_t idx = json_array_size(phases);
        for (size_t i = 0; i < idx; i++) {
            json_t *pe = json_array_get(phases, i);
            if (!json_is_object(pe)) {
                continue;
            }
            json_t *jid = json_object_get(pe, "id");
            json_t *jres = json_object_get(pe, "result");
            json_t *jsw = json_object_get(pe, "sw");
            json_t *jnote = json_object_get(pe, "notes");

            emv_term_phase_t phase = EMV_PHASE_INIT;
            if (json_is_integer(jid)) {
                phase = (emv_term_phase_t)(int)json_integer_value(jid);
            }
            int result = json_is_integer(jres) ? (int)json_integer_value(jres) : PM3_SUCCESS;
            uint16_t sw = 0;
            if (json_is_string(jsw)) {
                uint8_t swbuf[2] = {0};
                int swlen = 0;
                param_gethex_to_eol(json_string_value(jsw), 0, swbuf, sizeof(swbuf), &swlen);
                if (swlen == 2) {
                    sw = (swbuf[0] << 8) | swbuf[1];
                }
            }
            const char *note = json_is_string(jnote) ? json_string_value(jnote) : NULL;
            emv_term_event_add(ctx, phase, result, sw, note);
        }
    }

    str_copy(ctx->session_file, sizeof(ctx->session_file), path);
    json_decref(root);
    PrintAndLogEx(SUCCESS, "Session loaded: %s", path);
    return PM3_SUCCESS;
}
