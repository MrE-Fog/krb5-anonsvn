/*
 * lib/krb5/krb/mk_req.c
 *
 * Copyright 1990,1991 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
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
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 * 
 *
 * krb5_mk_req() routine.
 */

#include "k5-int.h"
#include "auth_con.h"

/*
 Formats a KRB_AP_REQ message into outbuf.

 server specifies the principal of the server to receive the message; if
 credentials are not present in the credentials cache for this server, the
 TGS request with default parameters is used in an attempt to obtain
 such credentials, and they are stored in ccache.

 kdc_options specifies the options requested for the 
 ap_req_options specifies the KRB_AP_REQ options desired.

 checksum specifies the checksum to be used in the authenticator.

 The outbuf buffer storage is allocated, and should be freed by the
 caller when finished.

 returns system errors
*/

extern krb5_flags krb5_kdc_default_options;

krb5_error_code
krb5_mk_req(context, auth_context, ap_req_options, service, hostname, in_data,
	      ccache, outbuf)
    krb5_context          context;
    krb5_auth_context  ** auth_context;
    const krb5_flags      ap_req_options;
    char		* service;
    char		* hostname;
    krb5_data           * in_data;
    krb5_ccache 	  ccache;
    krb5_data 		* outbuf;
{
    krb5_error_code 	  retval;
    krb5_principal	  server;
    krb5_creds 		* credsp;
    krb5_creds 		  creds;
    char	       ** realm;

    /* get realm */
    if (retval = krb5_get_host_realm(context, hostname, &realm)) 
	return retval;

    /* build principal */
    if (retval = krb5_build_principal(context, &server, strlen(realm[0]),
				      realm[0], service, hostname, NULL))
	goto cleanup_realm;
				      
    /* obtain ticket & session key */
    memset((char *)&creds, 0, sizeof(creds));
    if (retval = krb5_copy_principal(context, server, &creds.server))
	goto cleanup_princ;

    if (retval = krb5_cc_get_principal(context, ccache, &creds.client))
	goto cleanup_creds;

    if (retval = krb5_get_credentials(context, krb5_kdc_default_options,
				      ccache, &creds, &credsp))
	goto cleanup_creds;

    retval = krb5_mk_req_extended(context, auth_context, ap_req_options, 
				  in_data, credsp, outbuf);

    krb5_free_creds(context, credsp);

cleanup_creds:
    krb5_free_cred_contents(context, &creds);

cleanup_princ:
    krb5_free_principal(context, server);

cleanup_realm:
    krb5_free_host_realm(context, realm);
    return retval;
}
