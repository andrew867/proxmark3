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
// EMV terminal emulator — extracted transaction flow from CmdEMVExec
//-----------------------------------------------------------------------------

#include "emv_transaction.h"
#include "emv_term_profile.h"
#include "../emvjson.h"
#include "../dol.h"
#include "../emv_tags.h"
#include "ui.h"
#include "proxmark3.h"
#include <string.h>

#define EMVAC_AC_MASK  0xC0
#define EMVAC_AAC      0x00
#define EMVAC_TC       0x40
#define EMVAC_ARQC     0x80
#define EMVAC_CDAREQ   0x10

void emv_transaction_process_gpo_format1(struct tlvdb *tlvRoot, uint8_t *buf, size_t len, bool decodeTLV) {
    if (buf[0] == 0x80) {
        if (decodeTLV) {
            PrintAndLogEx(SUCCESS, "GPO response format1:");
            TLVPrintFromBuffer(buf, len);
        }

        if (len < 4 || (len - 4) % 4) {
            PrintAndLogEx(ERR, "GPO response format 1 parsing error. length = %zu", len);
        } else {
            struct tlvdb *f1AIP = tlvdb_fixed(0x82, 2, buf + 2);
            tlvdb_add(tlvRoot, f1AIP);
            if (decodeTLV) {
                PrintAndLogEx(INFO, "\n* * Decode response format 1 (0x80) AIP and AFL:");
                TLVPrintFromTLV(f1AIP);
            }

            struct tlvdb *f1AFL = tlvdb_fixed(0x94, len - 4, buf + 2 + 2);
            tlvdb_add(tlvRoot, f1AFL);
            if (decodeTLV) {
                TLVPrintFromTLV(f1AFL);
            }
        }
    } else if (decodeTLV) {
        TLVPrintFromBuffer(buf, len);
    }
}

void emv_transaction_process_ac_format1(struct tlvdb *tlvRoot, uint8_t *buf, size_t len, bool decodeTLV) {
    if (buf[0] == 0x80) {
        if (decodeTLV) {
            PrintAndLogEx(SUCCESS, "AC response format1:");
            TLVPrintFromBuffer(buf, len);
        }

        uint8_t elmlen = len - 2;

        if (len < 4 + 2 || (elmlen - 2) % 4 || elmlen != buf[1]) {
            PrintAndLogEx(ERR, "AC response format1 parsing error. length=%zu", len);
        } else {
            struct tlvdb *tlvElm = NULL;
            if (decodeTLV) {
                PrintAndLogEx(NORMAL, "\n------------ Format1 decoded ------------");
            }

            tlvdb_change_or_add_node_ex(tlvRoot, 0x9f27, 1, &buf[2], &tlvElm);
            if (decodeTLV) {
                TLVPrintFromTLV(tlvElm);
            }

            tlvdb_change_or_add_node_ex(tlvRoot, 0x9f36, 2, &buf[3], &tlvElm);
            if (decodeTLV) {
                TLVPrintFromTLV(tlvElm);
            }

            tlvdb_change_or_add_node_ex(tlvRoot, 0x9f26, 8, &buf[5], &tlvElm);
            if (decodeTLV) {
                TLVPrintFromTLV(tlvElm);
            }
        }
    } else if (decodeTLV) {
        TLVPrintFromBuffer(buf, len);
    }
}

emv_term_outcome_t emv_transaction_outcome_from_cid(uint8_t cid) {
    switch (cid & EMVAC_AC_MASK) {
        case EMVAC_AAC:
            return EMV_OUTCOME_DECLINED;
        case EMVAC_TC:
            return EMV_OUTCOME_APPROVED_OFFLINE;
        case EMVAC_ARQC:
            return EMV_OUTCOME_ONLINE_REQUIRED;
        default:
            return EMV_OUTCOME_UNKNOWN;
    }
}

int emv_transaction_init(emv_term_ctx_t *ctx) {
    if (!ctx) {
        return PM3_EINVARG;
    }

    uint8_t buf[APDU_RES_LEN] = {0};
    size_t len = 0;
    uint16_t sw = 0;
    int res;
    uint8_t psenum = (ctx->channel == CC_CONTACT) ? 1 : 2;

    SetAPDULogging(ctx->opts.show_apdu);

    if (!ctx->opts.force_search) {
        PrintAndLogEx(NORMAL, "");
        PrintAndLogEx(INFO, "* PPSE.");
        res = EMVSearchPSE(ctx->channel, ctx->opts.activate_field, true, psenum, ctx->opts.decode_tlv, ctx->select);
        if (res) {
            PrintAndLogEx(INFO, "Check PPSE instead of PSE and vice versa...");
            res = EMVSearchPSE(ctx->channel, false, true, psenum == 1 ? 2 : 1, ctx->opts.decode_tlv, ctx->select);
        }
        if (!res) {
            TLVPrintAIDlistFromSelectTLV(ctx->select);
            EMVSelectApplication(ctx->select, ctx->aid, &ctx->aid_len);
        }
    }

    if (!ctx->aid_len) {
        PrintAndLogEx(INFO, "\n* Search AID in list.");
        SetAPDULogging(false);
        if (EMVSearch(ctx->channel, ctx->opts.activate_field, true, ctx->opts.decode_tlv, ctx->select, false)) {
            return PM3_ERFTRANS;
        }
        TLVPrintAIDlistFromSelectTLV(ctx->select);
        EMVSelectApplication(ctx->select, ctx->aid, &ctx->aid_len);
    }

    if (!ctx->aid_len) {
        PrintAndLogEx(WARNING, "Can't select AID. EMV AID not found");
        return PM3_ERFTRANS;
    }

    PrintAndLogEx(INFO, "\n* Selecting AID:%s", sprint_hex_inrow(ctx->aid, ctx->aid_len));
    SetAPDULogging(ctx->opts.show_apdu);
    res = EMVSelect(ctx->channel, false, true, ctx->aid, ctx->aid_len, buf, sizeof(buf), &len, &sw, ctx->card);
    if (res) {
        PrintAndLogEx(WARNING, "Can't select AID (%d). Exit...", res);
        return PM3_ERFTRANS;
    }

    if (ctx->opts.decode_tlv) {
        TLVPrintFromBuffer(buf, len);
    }
    PrintAndLogEx(INFO, "* Selected.");

    PrintAndLogEx(INFO, "\n* Init transaction parameters.");
    emv_term_init_transaction_params(ctx->card, false, NULL, ctx->tr_type, ctx->opts.gen_ac_gpo);
    if (ctx->opts.param_load_json) {
        if (ctx->opts.use_terminal_profile) {
            if (!emv_term_profile_load(ctx->card, ctx->opts.profile_path)) {
                PrintAndLogEx(WARNING, "Terminal profile not found, loading emv_defparams.json...");
                ParamLoadFromJson(ctx->card);
            }
            emv_term_profile_load(ctx->terminal, ctx->opts.profile_path);
        } else {
            ParamLoadFromJson(ctx->card);
        }
    }
    if (ctx->opts.decode_tlv) {
        TLVPrintFromTLV(ctx->card);
    }

    PrintAndLogEx(INFO, "\n* Calc PDOL.");
    ctx->pdol_data_tlv = dol_process(tlvdb_get(ctx->card, 0x9f38, NULL), ctx->card, 0x83);
    if (!ctx->pdol_data_tlv) {
        PrintAndLogEx(ERR, "Error: can't create PDOL TLV.");
        return PM3_ESOFT;
    }

    size_t pdol_data_tlv_data_len;
    unsigned char *pdol_data_tlv_data = tlv_encode(ctx->pdol_data_tlv, &pdol_data_tlv_data_len);
    if (!pdol_data_tlv_data) {
        PrintAndLogEx(ERR, "Error: can't create PDOL data.");
        return PM3_ESOFT;
    }
    PrintAndLogEx(INFO, "PDOL data[%zu]: %s", pdol_data_tlv_data_len, sprint_hex(pdol_data_tlv_data, pdol_data_tlv_data_len));

    PrintAndLogEx(INFO, "\n* GPO.");
    res = EMVGPO(ctx->channel, true, pdol_data_tlv_data, pdol_data_tlv_data_len, buf, sizeof(buf), &len, &sw, ctx->card);
    free(pdol_data_tlv_data);

    if (res) {
        PrintAndLogEx(ERR, "GPO error(%d): %4x. Exit...", res, sw);
        return PM3_ERFTRANS;
    }

    emv_transaction_process_gpo_format1(ctx->card, buf, len, ctx->opts.decode_tlv);

    const struct tlv *track2 = tlvdb_get(ctx->card, 0x57, NULL);
    if (!tlvdb_get(ctx->card, 0x5a, NULL) && track2 && track2->len >= 8) {
        struct tlvdb *pan = GetPANFromTrack2(track2);
        if (pan) {
            tlvdb_add(ctx->card, pan);
            const struct tlv *pantlv = tlvdb_get(ctx->card, 0x5a, NULL);
            PrintAndLogEx(INFO, "\n* * Extracted PAN from track2: %s", sprint_hex(pantlv->value, pantlv->len));
        } else {
            PrintAndLogEx(WARNING, "\n* * WARNING: Can't extract PAN from track2.");
        }
    }

    PrintAndLogEx(INFO, "\n* Read records from AFL.");
    const struct tlv *AFL = tlvdb_get(ctx->card, 0x94, NULL);
    if (!AFL || !AFL->len) {
        PrintAndLogEx(WARNING, "WARNING: AFL not found.");
    }

    ctx->oda_list_len = 0;

    while (AFL && AFL->len) {
        if (AFL->len % 4) {
            PrintAndLogEx(WARNING, "Warning: Wrong AFL length: %zu", AFL->len);
            break;
        }

        for (int i = 0; i < AFL->len / 4; i++) {
            uint8_t SFI = AFL->value[i * 4 + 0] >> 3;
            uint8_t SFIstart = AFL->value[i * 4 + 1];
            uint8_t SFIend = AFL->value[i * 4 + 2];
            uint8_t SFIoffline = AFL->value[i * 4 + 3];

            PrintAndLogEx(INFO, "* * SFI[%02x] start:%02x end:%02x offline count:%02x", SFI, SFIstart, SFIend, SFIoffline);
            if (SFI == 0 || SFI == 31 || SFIstart == 0 || SFIstart > SFIend) {
                PrintAndLogEx(WARNING, "SFI ERROR! Skipped...");
                continue;
            }

            for (int n = SFIstart; n <= SFIend; n++) {
                PrintAndLogEx(INFO, "* * * SFI[%02x] %d", SFI, n);
                res = EMVReadRecord(ctx->channel, true, SFI, n, buf, sizeof(buf), &len, &sw, ctx->card);
                if (res) {
                    PrintAndLogEx(WARNING, "Error SFI[%02x]. APDU error %4x", SFI, sw);
                    continue;
                }

                if (ctx->opts.decode_tlv) {
                    TLVPrintFromBuffer(buf, len);
                    PrintAndLogEx(NORMAL, "");
                }

                if (SFIoffline > 0) {
                    if (SFI < 11) {
                        const unsigned char *abuf = buf;
                        size_t elmlen = len;
                        struct tlv e;
                        if (tlv_parse_tl(&abuf, &elmlen, &e)) {
                            memcpy(&ctx->oda_list[ctx->oda_list_len], &buf[len - elmlen], elmlen);
                            ctx->oda_list_len += elmlen;
                        } else {
                            PrintAndLogEx(WARNING, "Error SFI[%02x]. Creating ODA input list error.", SFI);
                        }
                    } else {
                        memcpy(&ctx->oda_list[ctx->oda_list_len], buf, len);
                        ctx->oda_list_len += len;
                    }
                    SFIoffline--;
                }
            }
        }
        break;
    }

    if (ctx->oda_list_len) {
        struct tlvdb *oda = tlvdb_fixed(0x21, ctx->oda_list_len, ctx->oda_list);
        tlvdb_add(ctx->card, oda);
        PrintAndLogEx(INFO, "* Input list for Offline Data Authentication added to TLV. len=%zu \n", ctx->oda_list_len);
    }

    return PM3_SUCCESS;
}

int emv_transaction_oda(emv_term_ctx_t *ctx) {
    if (!ctx) {
        return PM3_EINVARG;
    }

    uint16_t AIP = 0;
    const struct tlv *AIPtlv = tlvdb_get(ctx->card, 0x82, NULL);
    if (AIPtlv) {
        AIP = AIPtlv->value[0] + AIPtlv->value[1] * 0x100;
        PrintAndLogEx(INFO, "* * AIP=%04x", AIP);
    } else {
        PrintAndLogEx(ERR, "Can't find AIP.");
        return PM3_ESOFT;
    }

    ctx->oda_performed = false;
    ctx->oda_success = true;

    if (AIP & 0x0040) {
        PrintAndLogEx(INFO, "\n* SDA");
        ctx->oda_performed = true;
        if (trSDA(ctx->card)) {
            ctx->oda_success = false;
        }
    }

    if (AIP & 0x0020) {
        PrintAndLogEx(INFO, "\n* DDA");
        ctx->oda_performed = true;
        if (trDDA(ctx->channel, ctx->opts.decode_tlv, ctx->card)) {
            ctx->oda_success = false;
        }
    }

    return PM3_SUCCESS;
}

int emv_transaction_genac1(emv_term_ctx_t *ctx) {
    if (!ctx) {
        return PM3_EINVARG;
    }

    uint8_t buf[APDU_RES_LEN] = {0};
    size_t len = 0;
    uint16_t sw = 0;
    int res;

    uint16_t AIP = 0;
    const struct tlv *AIPtlv = tlvdb_get(ctx->card, 0x82, NULL);
    if (AIPtlv) {
        AIP = AIPtlv->value[0] + AIPtlv->value[1] * 0x100;
    }

    if (ctx->tr_type == TT_QVSDCMCHIP || ctx->tr_type == TT_CDA) {
        const struct tlv *AC = tlvdb_get(ctx->card, 0x9F26, NULL);
        if (AC) {
            PrintAndLogEx(INFO, "\n--> qVSDC transaction.");
            PrintAndLogEx(INFO, "* AC path");
            const struct tlv *ATC = tlvdb_get(ctx->card, 0x9F36, NULL);
            if (ATC) {
                PrintAndLogEx(INFO, "ATC: %s", sprint_hex(ATC->value, ATC->len));
                PrintAndLogEx(INFO, "AC: %s", sprint_hex(AC->value, AC->len));
            }
            ctx->outcome = EMV_OUTCOME_APPROVED_OFFLINE;
            return PM3_SUCCESS;
        }
    }

    if (GetCardPSVendor(ctx->aid, ctx->aid_len) == CV_MASTERCARD &&
            (ctx->tr_type == TT_QVSDCMCHIP || ctx->tr_type == TT_CDA)) {

        const struct tlv *CDOL1 = tlvdb_get(ctx->card, 0x8c, NULL);
        if (CDOL1) {
            PrintAndLogEx(INFO, "\n--> Mastercard M/Chip transaction.");

            res = EMVGenerateChallenge(ctx->channel, true, buf, sizeof(buf), &len, &sw, ctx->card);
            if (res) {
                PrintAndLogEx(ERR, "Error GetChallenge. APDU error %4x", sw);
                return PM3_ERFTRANS;
            }
            if (len < 4) {
                PrintAndLogEx(ERR, "Error GetChallenge. Wrong challenge length %zu", len);
                return PM3_ESOFT;
            }

            struct tlvdb *ICCDynN = tlvdb_fixed(0x9f4c, len, buf);
            tlvdb_add(ctx->card, ICCDynN);

            struct tlv *cdol_data_tlv = dol_process(tlvdb_get(ctx->card, 0x8c, NULL), ctx->card, 0x01);
            if (!cdol_data_tlv) {
                PrintAndLogEx(ERR, "Error: can't create CDOL1 TLV.");
                return PM3_ESOFT;
            }

            PrintAndLogEx(INFO, "CDOL1 data[%zu]: %s", cdol_data_tlv->len, sprint_hex(cdol_data_tlv->value, cdol_data_tlv->len));
            PrintAndLogEx(INFO, "* * AC1");
            res = EMVAC(ctx->channel, true,
                        (ctx->tr_type == TT_CDA) ? EMVAC_TC + EMVAC_CDAREQ : EMVAC_TC,
                        (uint8_t *)cdol_data_tlv->value, cdol_data_tlv->len,
                        buf, sizeof(buf), &len, &sw, ctx->card);
            if (res) {
                PrintAndLogEx(ERR, "AC1 error(%d): %4x. Exit...", res, sw);
                free(cdol_data_tlv);
                return PM3_ERFTRANS;
            }

            if (ctx->opts.decode_tlv) {
                TLVPrintFromBuffer(buf, len);
            }

            if (ctx->tr_type == TT_CDA) {
                PrintAndLogEx(INFO, "\n* CDA:");
                struct tlvdb *ac_tlv = tlvdb_parse_multi(buf, len);
                if (tlvdb_get(ac_tlv, 0x9f4b, NULL)) {
                    trCDA(ctx->card, ac_tlv, ctx->pdol_data_tlv, cdol_data_tlv);
                }
                free(ac_tlv);
            }
            free(cdol_data_tlv);

            uint8_t CID = 0;
            tlvdb_get_uint8(ctx->card, 0x9f27, &CID);
            ctx->outcome = emv_transaction_outcome_from_cid(CID);
            return PM3_SUCCESS;
        }
    }

    if (AIP & 0x8000 && ctx->tr_type == TT_MSD) {
        PrintAndLogEx(INFO, "\n--> MSD transaction.");
        ctx->outcome = EMV_OUTCOME_APPROVED_OFFLINE;
        return PM3_SUCCESS;
    }

    if (GetCardPSVendor(ctx->aid, ctx->aid_len) == CV_VISA &&
            (ctx->tr_type == TT_VSDC || ctx->tr_type == TT_CDA)) {

        PrintAndLogEx(INFO, "\n--> VSDC transaction.");
        struct tlv *cdol1_data_tlv = dol_process(tlvdb_get(ctx->card, 0x8c, NULL), ctx->card, 0x01);
        if (!cdol1_data_tlv) {
            PrintAndLogEx(ERR, "Error: can't create CDOL1 TLV.");
            return PM3_ESOFT;
        }

        PrintAndLogEx(INFO, "CDOL1 data[%zu]: %s", cdol1_data_tlv->len, sprint_hex(cdol1_data_tlv->value, cdol1_data_tlv->len));
        PrintAndLogEx(INFO, "* * AC1");
        res = EMVAC(ctx->channel, true,
                    (ctx->tr_type == TT_CDA) ? EMVAC_TC + EMVAC_CDAREQ : EMVAC_TC,
                    (uint8_t *)cdol1_data_tlv->value, cdol1_data_tlv->len,
                    buf, sizeof(buf), &len, &sw, ctx->card);
        if (res) {
            PrintAndLogEx(ERR, "AC1 error(%d): %4x. Exit...", res, sw);
            free(cdol1_data_tlv);
            return PM3_ERFTRANS;
        }

        emv_transaction_process_ac_format1(ctx->card, buf, len, ctx->opts.decode_tlv);

        uint8_t CID = 0;
        tlvdb_get_uint8(ctx->card, 0x9f27, &CID);
        ctx->outcome = emv_transaction_outcome_from_cid(CID);

        if ((CID & EMVAC_AC_MASK) == EMVAC_AAC) {
            PrintAndLogEx(INFO, "AC1 result: AAC (Transaction declined)");
        } else if ((CID & EMVAC_AC_MASK) == EMVAC_TC) {
            PrintAndLogEx(INFO, "AC1 result: TC (Transaction approved)");
        } else if ((CID & EMVAC_AC_MASK) == EMVAC_ARQC) {
            PrintAndLogEx(INFO, "AC1 result: ARQC (Online authorisation requested)");
        }

        free(cdol1_data_tlv);
        return PM3_SUCCESS;
    }

    if (GetCardPSVendor(ctx->aid, ctx->aid_len) == CV_INTERAC &&
            (ctx->tr_type == TT_VSDC || ctx->tr_type == TT_QVSDCMCHIP || ctx->tr_type == TT_CDA)) {

        const struct tlv *CDOL1 = tlvdb_get(ctx->card, 0x8c, NULL);
        if (CDOL1) {
            PrintAndLogEx(INFO, "\n--> Interac transaction.");
            struct tlv *cdol1_data_tlv = dol_process(CDOL1, ctx->card, 0x01);
            if (!cdol1_data_tlv) {
                PrintAndLogEx(ERR, "Error: can't create CDOL1 TLV.");
                return PM3_ESOFT;
            }

            PrintAndLogEx(INFO, "CDOL1 data[%zu]: %s", cdol1_data_tlv->len, sprint_hex(cdol1_data_tlv->value, cdol1_data_tlv->len));
            PrintAndLogEx(INFO, "* * AC1");
            res = EMVAC(ctx->channel, true, EMVAC_ARQC,
                        (uint8_t *)cdol1_data_tlv->value, cdol1_data_tlv->len,
                        buf, sizeof(buf), &len, &sw, ctx->card);
            if (res) {
                PrintAndLogEx(ERR, "AC1 error(%d): %4x. Exit...", res, sw);
                free(cdol1_data_tlv);
                return PM3_ERFTRANS;
            }

            emv_transaction_process_ac_format1(ctx->card, buf, len, ctx->opts.decode_tlv);
            uint8_t CID = 0;
            tlvdb_get_uint8(ctx->card, 0x9f27, &CID);
            ctx->outcome = emv_transaction_outcome_from_cid(CID);
            free(cdol1_data_tlv);
            return PM3_SUCCESS;
        }
    }

    PrintAndLogEx(INFO, "No GEN AC1 path for current transaction type / card.");
    return PM3_SUCCESS;
}
