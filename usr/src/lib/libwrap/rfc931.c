/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 */

 /*
  * rfc931() speaks a common subset of the RFC 931, AUTH, TAP, IDENT and RFC
  * 1413 protocols. It queries an RFC 931 etc. compatible daemon on a remote
  * host to look up the owner of a connection. The information should not be
  * used for authentication purposes. This routine intercepts alarm signals.
  * 
  * Diagnostics are reported through syslog(3).
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

/* System libraries. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>

/* Local stuff. */

#include "tcpd.h"

#define	RFC931_PORT	113		/* Semi-well-known port */
#define	ANY_PORT	0		/* Any old port will do */

int     rfc931_timeout = RFC931_TIMEOUT;/* Global so it can be changed */

static jmp_buf timebuf;

/* fsocket - open stdio stream on top of socket */

static FILE *fsocket(domain, type, protocol)
int     domain;
int     type;
int     protocol;
{
    int     s;
    FILE   *fp;

    if ((s = socket(domain, type, protocol)) < 0) {
	tcpd_warn("socket: %m");
	return (0);
    } else {
	if ((fp = fdopen(s, "r+")) == 0) {
	    tcpd_warn("fdopen: %m");
	    close(s);
	}
	return (fp);
    }
}

/* timeout - handle timeouts */

static void timeout(sig)
int     sig;
{
    longjmp(timebuf, sig);
}

/* rfc931 - return remote user name, given socket structures */

void    rfc931(rmt_sin, our_sin, dest)
struct sockaddr_gen *rmt_sin;
struct sockaddr_gen *our_sin;
char   *dest;
{
    unsigned rmt_port;
    unsigned our_port;
    struct sockaddr_gen rmt_query_sin;
    struct sockaddr_gen our_query_sin;
    char    user[256];			/* XXX */
    char    buffer[512];		/* XXX */
    char   *cp;
    char   *volatile result = unknown;
    FILE   *fp;
    volatile unsigned saved_timeout = 0;
    struct sigaction nact, oact;

    /*
     * Use one unbuffered stdio stream for writing to and for reading from
     * the RFC931 etc. server. This is done because of a bug in the SunOS
     * 4.1.x stdio library. The bug may live in other stdio implementations,
     * too. When we use a single, buffered, bidirectional stdio stream ("r+"
     * or "w+" mode) we read our own output. Such behaviour would make sense
     * with resources that support random-access operations, but not with
     * sockets.
     */

    if ((fp = fsocket(SGFAM(rmt_sin), SOCK_STREAM, 0)) != 0) {
	setbuf(fp, NULL);

	/*
	 * Set up a timer so we won't get stuck while waiting for the server.
	 */

	if (setjmp(timebuf) == 0) {
	    /*
	     * save the pending time in case the caller has armed an alarm.
	     */

	    saved_timeout = alarm(0);

	    /*
	     * It's guaranteed to enter this 'if' condition on the direct
	     * invocation of setjmp and hence no additional checks while
	     * restoring the signal handler.
	     * Now, get the old handler and set the new one
	     */
	    nact.sa_handler = timeout;
	    nact.sa_flags = 0;
	    (void) sigemptyset(&nact.sa_mask);
	    (void) sigaction(SIGALRM, &nact, &oact);
	    alarm(rfc931_timeout);

	    /*
	     * Bind the local and remote ends of the query socket to the same
	     * IP addresses as the connection under investigation. We go
	     * through all this trouble because the local or remote system
	     * might have more than one network address. The RFC931 etc.
	     * client sends only port numbers; the server takes the IP
	     * addresses from the query socket.
	     */

	    our_query_sin = *our_sin;
	    SGPORT(&our_query_sin) = htons(ANY_PORT);
	    rmt_query_sin = *rmt_sin;
	    SGPORT(&rmt_query_sin) = htons(RFC931_PORT);

	    if (bind(fileno(fp), (struct sockaddr *) &our_query_sin,
		     SGSOCKADDRSZ(&our_query_sin)) >= 0 &&
		connect(fileno(fp), (struct sockaddr *) &rmt_query_sin,
			SGSOCKADDRSZ(&rmt_query_sin)) >= 0) {

		/*
		 * Send query to server. Neglect the risk that a 13-byte
		 * write would have to be fragmented by the local system and
		 * cause trouble with buggy System V stdio libraries.
		 */

		fprintf(fp, "%u,%u\r\n",
			ntohs(SGPORT(rmt_sin)),
			ntohs(SGPORT(our_sin)));
		fflush(fp);

		/*
		 * Read response from server. Use fgets()/sscanf() so we can
		 * work around System V stdio libraries that incorrectly
		 * assume EOF when a read from a socket returns less than
		 * requested.
		 */

		if (fgets(buffer, sizeof(buffer), fp) != 0
		    && ferror(fp) == 0 && feof(fp) == 0
		    && sscanf(buffer, "%u , %u : USERID :%*[^:]:%255s",
			      &rmt_port, &our_port, user) == 3
		    && ntohs(SGPORT(rmt_sin)) == rmt_port
		    && ntohs(SGPORT(our_sin)) == our_port) {

		    /*
		     * Strip trailing carriage return. It is part of the
		     * protocol, not part of the data.
		     */

		    if (cp = strchr(user, '\r'))
			*cp = 0;
		    result = user;
		}
	    }
	    alarm(0);
	}
	/* Restore the old handler */
	(void) sigaction(SIGALRM, &oact, NULL);
	if (saved_timeout > 0)
		alarm(saved_timeout);
	fclose(fp);
    }
    STRN_CPY(dest, result, STRING_LENGTH);
}
