/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* plugins/preauth/securid_sam2/securid_sam2_main.c */
/*
 * Copyright (C) 2009, 2010 by the Massachusetts Institute of Technology.
 * All rights reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original M.I.T. software.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */
/*
 * Copyright (c) 2002 Naval Research Laboratory (NRL/CCS)
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the software,
 * derivative works or modified versions, and any portions thereof.
 *
 * NRL ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
 * RESULTING FROM THE USE OF THIS SOFTWARE.
 */

#include <k5-int.h>
#include <krb5/preauth_plugin.h>
#include <kdb.h>
#include "extern.h"

static struct {
    char* name;
    int   sam_type;
} *sam_ptr, sam_inst_map[] = {
    { "SECURID", PA_SAM_TYPE_SECURID, },
    { "GRAIL", PA_SAM_TYPE_GRAIL, },
    { 0, 0 },
};

krb5_error_code
sam_get_db_entry(krb5_context context, krb5_principal client,
                 int *sam_type, struct _krb5_db_entry_new **db_entry)
{
    struct _krb5_db_entry_new *assoc = NULL;
    krb5_principal newp = NULL;
    int probeslot;
    void *ptr = NULL;
    krb5_error_code retval;

    if (db_entry)
       *db_entry = NULL;
    retval = krb5_copy_principal(context, client, &newp);
    if (retval) {
        com_err("krb5kdc", retval, "copying client name for preauth probe");
        return retval;
    }

    probeslot = krb5_princ_size(context, newp)++;
    ptr = realloc(krb5_princ_name(context, newp),
                  krb5_princ_size(context, newp) * sizeof(krb5_data));
   if (ptr == NULL) {
       retval = ENOMEM;
       goto cleanup;
   }
   krb5_princ_name(context, newp) = ptr;

   for(sam_ptr = sam_inst_map; sam_ptr->name; sam_ptr++) {
       if (*sam_type && *sam_type != sam_ptr->sam_type)
           continue;

       krb5_princ_component(context,newp,probeslot)->data = sam_ptr->name;
       krb5_princ_component(context,newp,probeslot)->length =
           strlen(sam_ptr->name);
       retval = krb5_db_get_principal(context, newp, 0, &assoc);
       if (!retval)
           break;
   }
cleanup:
   if (ptr) {
       krb5_princ_component(context,newp,probeslot)->data = 0;
       krb5_princ_component(context,newp,probeslot)->length = 0;
       krb5_free_principal(context, newp);
   }
   if (probeslot)
       krb5_princ_size(context, newp)--;
   if (retval)
       return retval;
   if (sam_ptr->sam_type)  {
       /* Found entry of type sam_ptr->sam_type */
       if (sam_type)
           *sam_type = sam_ptr->sam_type;
       if (db_entry)
           *db_entry = assoc;
       else
           krb5_db_free_principal(context, assoc);
       return 0;
   } else {
       return KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN;
   }
}

static krb5_error_code
kdc_include_padata(krb5_context context, krb5_kdc_req *request,
                   struct _krb5_db_entry_new *client,
                   struct _krb5_db_entry_new *server,
                   preauth_get_entry_data_proc get_entry_proc,
                   void *pa_module_context, krb5_pa_data *pa_data)
{
    krb5_error_code retval;
    krb5_data *client_keys_data = NULL;
    krb5_keyblock *client_key = NULL;
    krb5_sam_challenge_2 sc2;
    krb5_sam_challenge_2_body sc2b;
    int sam_type = 0;             /* unknown */
    krb5_db_entry *sam_db_entry = NULL;
    krb5_data *encoded_challenge = NULL;

    memset(&sc2, 0, sizeof(sc2));
    memset(&sc2b, 0, sizeof(sc2b));
    sc2b.magic = KV5M_SAM_CHALLENGE_2;
    sc2b.sam_type = sam_type;

    retval = sam_get_db_entry(context, client->princ, &sam_type,
                              &sam_db_entry);
    if (retval)
        return retval;
    retval = get_entry_proc(context, request, client,
                            krb5plugin_preauth_keys, &client_keys_data);
    if (retval)
        goto cleanup;
    client_key = (krb5_keyblock *) client_keys_data->data;
    if (client_key->enctype == 0) {
        retval = KRB5KDC_ERR_ETYPE_NOSUPP;
        com_err("krb5kdc", retval,
                "No client keys found in processing SAM2 challenge");
        goto cleanup;
    }

    if (sam_type == 0) {
        retval = KRB5_PREAUTH_BAD_TYPE;
        goto cleanup;
    }

    /*
     * Defer getting the key for the SAM principal associated with the client
     * until the mechanism-specific code.  The mechanism may want to get a
     * specific keytype.
     */

    switch (sam_type) {
#ifdef ARL_SECURID_PREAUTH
    case PA_SAM_TYPE_SECURID:
        retval = get_securid_edata_2(context, client, client_key, &sc2b, &sc2);
        if (retval)
            goto cleanup;

        retval = encode_krb5_sam_challenge_2(&sc2, &encoded_challenge);
        if (retval) {
            com_err("krb5kdc", retval,
                    "while encoding SECURID SAM_CHALLENGE_2");
            goto cleanup;
        }

        pa_data->magic = KV5M_PA_DATA;
        pa_data->pa_type = KRB5_PADATA_SAM_CHALLENGE_2;
        pa_data->contents = (krb5_octet *) encoded_challenge->data;
        pa_data->length = encoded_challenge->length;
        encoded_challenge->data = NULL;

        retval = 0;
        break;
#endif  /* ARL_SECURID_PREAUTH */
    default:
        retval = KRB5_PREAUTH_BAD_TYPE;
        goto cleanup;
    }

cleanup:
    krb5_free_data(context, encoded_challenge);
    if (sam_db_entry)
        krb5_db_free_principal(context, sam_db_entry);
    if (client_keys_data) {
        while (client_key->enctype) {
            krb5_free_keyblock_contents(context, client_key);
            client_key++;
        }
        krb5_free_data(context, client_keys_data);
    }
    return retval;
}

static krb5_error_code
kdc_verify_preauth(krb5_context context, struct _krb5_db_entry_new *client,
                   krb5_data *req_pkt, krb5_kdc_req *request,
                   krb5_enc_tkt_part *enc_tkt_reply, krb5_pa_data *pa_data,
                   preauth_get_entry_data_proc get_entry_proc,
                   void *pa_module_context, void **opaque,
                   krb5_data **e_data, krb5_authdata ***authz_data)
{
    krb5_error_code retval, saved_retval = 0;
    krb5_sam_response_2 *sr2 = NULL;
    krb5_data scratch, *scratch2;
    char *client_name = NULL;
    krb5_sam_challenge_2 *out_sc2 = NULL;

    scratch.data = (char *) pa_data->contents;
    scratch.length = pa_data->length;

    retval = krb5_unparse_name(context, client->princ, &client_name);
    if (retval)
        goto cleanup;

    retval = decode_krb5_sam_response_2(&scratch, &sr2);
    if (retval) {
        com_err("krb5kdc",  retval,
                "while decoding SAM_RESPONSE_2 in verify_sam_response_2");
        sr2 = NULL;
        goto cleanup;
    }

    switch (sr2->sam_type) {
#ifdef ARL_SECURID_PREAUTH
    case PA_SAM_TYPE_SECURID:
        retval = verify_securid_data_2(context, client, sr2, enc_tkt_reply,
                                       pa_data, &out_sc2);
        if (retval)
            goto cleanup;
        break;
#endif  /* ARL_SECURID_PREAUTH */
    default:
        retval = KRB5_PREAUTH_BAD_TYPE;
        com_err("krb5kdc", retval, "while verifying SAM 2 data");
        break;
    }

  /*
   * It is up to the method-specific verify routine to set the ticket flags to
   * indicate TKT_FLG_HW_AUTH and/or TKT_FLG_PRE_AUTH.  Some methods may
   * require more than one round of dialog with the client and must return
   * successfully from their verify routine.  If does not set the TGT flags,
   * the required_preauth conditions will not be met and it will try again to
   * get enough preauth data from the client.  Do not set TGT flags here.
   */
cleanup:
    /*
     * Note that e_data is an output even in error conditions.  If we
     * successfully encode the output e_data, we return whatever error is
     * received above.  Otherwise we return the encoding error.
     */
    saved_retval = retval;
    if (out_sc2) {
        krb5_pa_data pa_out;
        krb5_pa_data *pa_array[2];
        pa_array[0] = &pa_out;
        pa_array[1] = NULL;
        pa_out.pa_type = KRB5_PADATA_SAM_CHALLENGE_2;
        retval = encode_krb5_sam_challenge_2(out_sc2, &scratch2);
        krb5_free_sam_challenge_2(context, out_sc2);
        if (retval)
            goto encode_error;
        pa_out.contents = (krb5_octet *) scratch2->data;
        pa_out.length = scratch2->length;
        retval = encode_krb5_padata_sequence(pa_array, e_data);
        krb5_free_data(context, scratch2);
    }
encode_error:
    krb5_free_sam_response_2(context, sr2);
    free(client_name);
    if (retval == 0)
        retval = saved_retval;
    return retval;
}


static int
kdc_preauth_flags(krb5_context context, krb5_preauthtype patype)
{
    return PA_HARDWARE;
}

krb5_preauthtype supported_pa_types[] = {
    KRB5_PADATA_SAM_RESPONSE_2, 0};

struct krb5plugin_preauth_server_ftable_v1 preauthentication_server_1 = {
    "SAM2",
    &supported_pa_types[0],
    NULL,
    NULL,
    kdc_preauth_flags,
    kdc_include_padata,
    kdc_verify_preauth,
    NULL,
    NULL
};
