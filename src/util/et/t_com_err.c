#include <stdio.h>
#include "com_err.h"
#include "et1.h"
#include "et2.h"

int misc_err, known_err;	/* known_err is err in whether or not
                                   table is 'known' to library */
int fail;

static void
try_one (errcode_t code, int known, int table, int msgno)
{
    const char *msg = error_message (code);
    char buffy[1024];

    sprintf (buffy, "error table %d message %d", table, msgno);
    if (!strcmp (buffy, msg)) {
	if (!known) {
	    known_err++;
	}
	return;
    }
    sprintf (buffy, "Unknown code et%d %d", table, msgno);
    if (!strcmp (buffy, msg)) {
	if (known)
	    known_err++;
	return;
    }
    printf ("error code %ld got unrecognized message '%s',\n"
	    "should have been table %d message %d\n",
	    (long) code, msg, table, msgno);
    misc_err++;
}

static void
try_table (int table, int known, int lineno,
	   errcode_t c0, errcode_t c1, errcode_t c2)
{
    try_one (c0, known, table, 0);
    try_one (c1, known, table, 1);
    try_one (c2, known, table, 2);
    if (misc_err != 0 || known_err != 0) {
	fail++;
	if (known_err)
	    printf ("table list error from line %d, table %d\n", lineno,
		    table);
	if (misc_err)
	    printf ("misc errors from line %d table %d\n", lineno, table);
	misc_err = known_err = 0;
    }
}

static void
try_em_1 (int t1_known, int t2_known, int lineno)
{
    try_table (1, t1_known, lineno, ET1_M0, ET1_M1, ET1_M2);
    try_table (2, t2_known, lineno, ET2_M0, ET2_M1, ET2_M2);
}
#define try_em(A,B) try_em_1(A,B,__LINE__)

int main (int argc, char *argv[])
{
    try_em (0, 0);
    add_error_table (&et_et1_error_table);
    try_em (1, 0);
    add_error_table (&et_et2_error_table);
    try_em (1, 1);
    remove_error_table (&et_et1_error_table);
    try_em (0, 1);
    remove_error_table (&et_et1_error_table);
    try_em (0, 1);
    remove_error_table (&et_et2_error_table);
    try_em (0, 0);

    initialize_et1_error_table ();
    try_em (1, 0);
    add_error_table (&et_et1_error_table);
    try_em (1, 0);
    remove_error_table (&et_et1_error_table);
    try_em (0, 0);

    initialize_et1_error_table ();
    try_em (1, 0);
    add_error_table (&et_et1_error_table);
    try_em (1, 0);
    add_error_table (&et_et2_error_table);
    try_em (1, 1);
    remove_error_table (&et_et1_error_table);
    try_em (0, 1);
    remove_error_table (&et_et2_error_table);
    try_em (0, 0);
    remove_error_table (&et_et2_error_table);
    try_em (0, 0);

    add_error_table (&et_et2_error_table);
    try_em (0, 1);
    initialize_et2_error_table ();
    try_em (0, 1);
    add_error_table (&et_et1_error_table);
    try_em (1, 1);
    remove_error_table (&et_et1_error_table);
    try_em (0, 1);
    remove_error_table (&et_et2_error_table);
    try_em (0, 0);

    return fail;
}
