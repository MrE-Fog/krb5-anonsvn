/*
 * Copyright (C) 1998 by the FundsXpress, INC.
 * 
 * All rights reserved.
 * 
 * Export of this software from the United States of America may require
 * a specific license from the United States Government.  It is the
 * responsibility of any person or organization contemplating export to
 * obtain such a license before exporting.
 * 
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of FundsXpress. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  FundsXpress makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "k5-int.h"
#include "etypes.h"
#include "aead.h"

krb5_error_code KRB5_CALLCONV
krb5_k_encrypt(krb5_context context, krb5_key key,
	       krb5_keyusage usage, const krb5_data *ivec,
	       const krb5_data *input, krb5_enc_data *output)
{
    const struct krb5_keytypes *ktp;

    ktp = find_enctype(key->keyblock.enctype);
    if (ktp == NULL)
	return KRB5_BAD_ENCTYPE;

    output->magic = KV5M_ENC_DATA;
    output->kvno = 0;
    output->enctype = key->keyblock.enctype;

    if (ktp->encrypt == NULL) {
	assert(ktp->aead != NULL);

	return krb5int_c_encrypt_aead_compat(ktp->aead, ktp->enc, ktp->hash,
					     key, usage, ivec, input,
					     &output->ciphertext);
    }

    return (*ktp->encrypt)(ktp->enc, ktp->hash, key, usage, ivec, input,
			   &output->ciphertext);
}

krb5_error_code KRB5_CALLCONV
krb5_c_encrypt(krb5_context context, const krb5_keyblock *keyblock,
	       krb5_keyusage usage, const krb5_data *ivec,
	       const krb5_data *input, krb5_enc_data *output)
{
    krb5_key key;
    krb5_error_code ret;

    ret = krb5_k_create_key(context, keyblock, &key);
    if (ret != 0)
	return ret;
    ret = krb5_k_encrypt(context, key, usage, ivec, input, output);
    krb5_k_free_key(context, key);
    return ret;
}
