
#ident  "@(#)g_imp_sec_context.c 1.2     96/01/18 SMI"

/*
 *  glue routine gss_export_sec_context
 *
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#include "mglueP.h"

OM_uint32
gss_import_sec_context(minor_status,
                       interprocess_token,
                       context_handle)

OM_uint32 *		minor_status;
gss_buffer_t		interprocess_token;
gss_ctx_id_t *		context_handle;

{
    OM_uint32		status;
    gss_union_ctx_id_t	ctx;
    gss_mechanism	mech;
    
    gss_initialize();

    if (context_handle == NULL || *context_handle==GSS_C_NO_CONTEXT)
	return GSS_S_NO_CONTEXT;

    /*
     * select the approprate underlying mechanism routine and
     * call it.
     */
    
    ctx = (gss_union_ctx_id_t) *context_handle;
    mech = get_mechanism (ctx->mech_type);

    if (mech) {

	if (mech->gss_import_sec_context)
	   status = mech->gss_import_sec_context(
						    mech->context,
						    minor_status,
						    interprocess_token,
						    &ctx->internal_ctx_id);

	else
	    status = GSS_S_BAD_BINDINGS;

	return(status);
    }
    
    return(GSS_S_NO_CONTEXT);
}
