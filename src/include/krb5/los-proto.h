/*
 * $Source$
 * $Author$
 * $Id$
 *
 * Copyright 1990 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <krb5/mit-copyright.h>.
 *
 * Function prototypes for Kerberos V5 library (libos)
 */

#include <krb5/copyright.h>

#ifndef __KRB5_LIBOS_PROTO__
#define __KRB5_LIBOS_PROTO__

/* requires <stdio.h> */

/* libos.spec */
krb5_error_code krb5_read_password
	PROTOTYPE((char *,
		   char *,
		   char *,
		   int * ));
krb5_error_code krb5_lock_file
	PROTOTYPE((FILE *,
		   char *,
		   int  ));
krb5_error_code krb5_unlock_file
	PROTOTYPE((FILE *,
		   char * ));
krb5_error_code krb5_timeofday
	PROTOTYPE((krb5_int32 * ));
krb5_error_code krb5_ms_timeofday
	PROTOTYPE((krb5_int32 *,
		   krb5_int16 * ));
int krb5_net_read
	PROTOTYPE((int ,
		   char *,
		   int  ));
int krb5_net_write
	PROTOTYPE((int ,
		   char *,
		   int  ));
		 /* get all the addresses of this host */
krb5_error_code krb5_os_localaddr
	PROTOTYPE((krb5_address ***));


#endif /* __KRB5_LIBOS_PROTO__ */
