/*
 * dsock.c - SCO UnixWare socket processing functions for lsof
 */


/*
 * Copyright 1996 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Victor A. Abell
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright 1996 Purdue Research Foundation.\nAll rights reserved.\n";
static char *rcsid = "$Id: dsock.c,v 1.14 2002/10/08 20:18:34 abe Exp $";
#endif


#define	TCPSTATES		/* activate tcpstates[] */
#include "lsof.h"


/*
 * Local function prototypes
 */

#if	UNIXWAREV>=70101 && UNIXWAREV<70103
_PROTOTYPE(static struct sockaddr_un *find_unix_sockaddr_un,(KA_T ka));
#endif	/* UNIXWAREV>=70101 && UNIXWAREV<70103 */


/*
 * print_tcptpi() - print TCP/TPI info
 */

void
print_tcptpi(nl)
	int nl;				/* 1 == '\n' required */
{
	char buf[128];
	char *cp = (char *)NULL;
	int ps = 0;
	int s;

	if (Ftcptpi & TCPTPI_STATE) {
	    s = Lf->lts.state.i;
	    switch (Lf->lts.type) {
	    case 0:
		if (s < 0 || s >= TCP_NSTATES) {
		    (void) snpf(buf, sizeof(buf), "UNKNOWN_TCP_STATE_%d", s);
		    cp = buf;
		} else
		    cp = tcpstates[s];
		break;
	    case 1:
		switch (s) {
		case TS_UNBND:
		    cp = "TS_UNBND";
		    break;
		case TS_WACK_BREQ:
		    cp = "TS_WACK_BREQ";
		    break;
		case TS_WACK_UREQ:
		    cp = "TS_WACK_UREQ";
		    break;
		case TS_IDLE:
		    cp = "TS_IDLE";
		    break;
		case TS_WACK_OPTREQ:
		    cp = "TS_WACK_OPTREQ";
		    break;
		case TS_WACK_CREQ:
		    cp = "TS_WACK_CREQ";
		    break;
		case TS_WCON_CREQ:
		    cp = "TS_WCON_CREQ";
		    break;
		case TS_WRES_CIND:
		    cp = "TS_WRES_CIND";
		    break;
		case TS_WACK_CRES:
		    cp = "TS_WACK_CRES";
		    break;
		case TS_DATA_XFER:
		    cp = "TS_DATA_XFER";
		    break;
		case TS_WIND_ORDREL:
		    cp = "TS_WIND_ORDREL";
		    break;
		case TS_WREQ_ORDREL:
		    cp = "TS_WREQ_ORDREL";
		    break;
		case TS_WACK_DREQ6:
		    cp = "TS_WACK_DREQ6";
		    break;
		case TS_WACK_DREQ7:
		    cp = "TS_WACK_DREQ7";
		    break;
		case TS_WACK_DREQ9:
		    cp = "TS_WACK_DREQ9";
		    break;
		case TS_WACK_DREQ10:
		    cp = "TS_WACK_DREQ10";
		    break;
		case TS_WACK_DREQ11:
		    cp = "TS_WACK_DREQ11";
		    break;
		default:
		    (void) snpf(buf, sizeof(buf), "UNKNOWN_TPI_STATE_%d", s);
		    cp = buf;
		}
	    }
	    if (cp) {
		if (Ffield)
		    (void) printf("%cST=%s%c", LSOF_FID_TCPTPI, cp, Terminator);
		else {
		    putchar('(');
		    (void) fputs(cp, stdout);
		}
		ps++;
	    }
	}

# if	defined(HASTCPTPIQ)
	if (Ftcptpi & TCPTPI_QUEUES) {
	    if (Lf->lts.rqs) {
		if (Ffield)
		    putchar(LSOF_FID_TCPTPI);
		else {
		    if (ps)
			putchar(' ');
		    else
			putchar('(');
		}
		(void) printf("QR=%lu", Lf->lts.rq);
		if (Ffield)
		    putchar(Terminator);
		ps++;
	    }
	    if (Lf->lts.sqs) {
		if (Ffield)
		    putchar(LSOF_FID_TCPTPI);
		else {
		    if (ps)
			putchar(' ');
		    else
			putchar('(');
		}
		(void) printf("QS=%lu", Lf->lts.sq);
		if (Ffield)
		    putchar(Terminator);
		ps++;
	    }
	}
# endif	/* defined(HASTCPTPIQ) */

# if	defined(HASTCPTPIW)
	if (Ftcptpi & TCPTPI_WINDOWS) {
	    if (Lf->lts.rws) {
		if (Ffield)
		    putchar(LSOF_FID_TCPTPI);
		else {
		    if (ps)
			putchar(' ');
		    else
			putchar('(');
		}
		(void) printf("WR=%lu", Lf->lts.rw);
		if (Ffield)
		    putchar(Terminator);
		ps++;
	    }
	    if (Lf->lts.wws) {
		if (Ffield)
		    putchar(LSOF_FID_TCPTPI);
		else {
		    if (ps)
			putchar(' ');
		    else
			putchar('(');
		}
		(void) printf("WW=%lu", Lf->lts.ww);
		if (Ffield)
		    putchar(Terminator);
		ps++;
	    }
	}
# endif	/* defined(HASTCPTPIW) */

	if (!Ffield && ps)
	    putchar(')');
	if (nl)
	    putchar('\n');
}


/*
 * process_socket() - process socket
 */

void
process_socket(pr, q)
	char *pr;			/* protocol name */
	struct queue *q;		/* queue at end of stream */
{
	unsigned char *fa = (unsigned char *)NULL;
	int fp, ipv, lp;
	struct inpcb inp;
	unsigned char *la = (unsigned char *)NULL;
	struct tcpcb t;
	int tcp = 0;
	short ts = 0;
	int udp = 0;

/*
 * Process protocol specification.
 */
	Lf->inp_ty = 2;
	(void) snpf(Lf->iproto, sizeof(Lf->iproto), "%s", pr);
	Lf->is_stream = 0;
	if (strcasecmp(pr, "TCP") == 0) {
	    ipv = 4;
	    tcp = 1;
	} else if (strcasecmp(pr, "UDP") == 0) {
	    ipv = 4;
	    udp = 1;
	}

#if	defined(HASIPv6)
	else if (strcasecmp(pr, "TCP6") == 0) {
	    ipv = 6;
	    tcp = 1;
	    Lf->iproto[3] = '\0';
	} else if (strcasecmp(pr, "UDP6") == 0) {
	    ipv = 6;
	    udp = 1;
	    Lf->iproto[3] = '\0';
	}
#endif	/* defined(HASIPv6) */

	if (Fnet && (tcp || udp)) {
	    if (!FnetTy || (FnetTy == ipv))
		Lf->sf |= SELNET;
	}

#if	defined(HASIPv6)
        (void) snpf(Lf->type, sizeof(Lf->type), (ipv == 6) ? "IPv6" : "IPv4");
#else	/* !defined(HASIPv6) */
        (void) snpf(Lf->type, sizeof(Lf->type), "inet");
#endif	/* defined(HASIPv6) */

/*
 * The PCB address is found in the private data structure at the end
 * of the queue.
 */
	if (q->q_ptr) {
	    enter_dev_ch(print_kptr((KA_T)q->q_ptr, (char *)NULL, 0));
	    if (tcp || udp) {
		if (kread((KA_T)q->q_ptr, (char *)&inp, sizeof(inp))) {
		    (void) snpf(Namech, Namechl, "can't read inpcb from %s",
			print_kptr((KA_T)q->q_ptr, (char *)NULL, 0));
		    enter_nm(Namech);
		    return;
		}
		la = (unsigned char *)&inp.inp_laddr;
		lp = (int)ntohs(inp.inp_lport);
		if (inp.inp_faddr.s_addr != INADDR_ANY || inp.inp_fport != 0) {
		    fa = (unsigned char *)&inp.inp_faddr;
		    fp = (int)ntohs(inp.inp_fport);
		}
		if (fa || la)
		    (void) ent_inaddr(la, lp, fa, fp, AF_INET);
		if (tcp) {
		    if (inp.inp_ppcb
		    &&  !kread((KA_T)inp.inp_ppcb, (char *)&t, sizeof(t))) {
			ts = 1;
			Lf->lts.type = 0;
			Lf->lts.state.i = (int)t.t_state;
		    }
		} else {
		    Lf->lts.type = 1;
		    Lf->lts.state.i = (int)inp.inp_tstate;
		} 
	    } else
		enter_nm("no address for this protocol");
	} else
	    enter_nm("no address");
/*
 * Save size information.
 */
	if (ts) {
	    if (Fsize) {

#if	UNIXWAREV>=70000
#define	t_outqsize	t_qsize
#endif	/* UNIXWAREV>=70000 */

		if (Lf->access == 'r')
		    Lf->sz = (SZOFFTYPE)t.t_iqsize;
		else if (Lf->access == 'w')
		    Lf->sz = (SZOFFTYPE)t.t_outqsize;
		else
		    Lf->sz = (SZOFFTYPE)(t.t_iqsize + t.t_outqsize);
		Lf->sz_def = 1;

	    } else
		Lf->off_def = 1;

#if	defined(HASTCPTPIQ)
		Lf->lts.rq = (unsigned long)t.t_iqsize;
		Lf->lts.sq = (unsigned long)t.t_outqsize;
		Lf->lts.rqs = Lf->lts.sqs = 1;
#endif	/* defined(HASTCPTPIQ) */

	}
	else if (Fsize) {
	    Lf->sz = (SZOFFTYPE)q->q_count;
	    Lf->sz_def = 1;
	} else
	    Lf->off_def = 1;
	enter_nm(Namech);
	return;
}


#if	UNIXWAREV>=70101
/*
 * process_unix_sockstr() - process a UNIX socket stream, if applicable
 */

int
process_unix_sockstr(v, na)
	struct vnode *v;		/* the stream's vnode */
	KA_T na;			/* kernel vnode address */
{
	int as;
	char *ep, tbuf[32], tbuf1[32], *ty;
	KA_T ka, sa, sh;
	struct stdata sd;
	struct ss_socket ss;
	size_t sz;

# if	UNIXWAREV<70103
	struct sockaddr_un *la, *ra;
# else	/* UNIXWAREV>=70103 */
	struct sockaddr_un la, ra;
	unsigned char las = 0;
	unsigned char ras = 0;
	int up = (int)(sizeof(la.sun_path) - 1);
/*
 * It's serious if the sizeof(sun_path) in sockaddr_un isn't greater than zero.
 */
	if (up < 0) {
	    (void) snpf(Namech, Namechl, "sizeof(sun_path) < 1 (%d)", up);
	    enter_nm(Namech);
	    return(1);
	}
# endif	/* UNIXWAREV<70103 */

/*
 * Read the stream head, if possible.
 */
	if (!(sh = (KA_T)v->v_stream))
	    return(0);
	if (readstdata(sh, &sd)) {
	    (void) snpf(Namech, Namechl,
		"vnode at %s; can't read stream head at %s",
		print_kptr(na, (char *)NULL, 0),
		print_kptr(sh, tbuf, sizeof(tbuf)));
	    enter_nm(Namech);
	    return(1);
	}
/*
 * If the stream head has pointer to a socket, read the socket structure
 */
	if (!(sa = (KA_T)sd.sd_socket))
	    return(0);
	if (kread(sa, (char *)&ss, sizeof(ss))) {
	    (void) snpf(Namech, Namechl,
		"vnode at %s; stream head at %s; can't read socket at %s",
		print_kptr(na, (char *)NULL, 0),
		print_kptr(sh, tbuf, sizeof(tbuf)),
		print_kptr(sa, tbuf1, sizeof(tbuf1)));
	    enter_nm(Namech);
	    return(1);
	}
/*
 * If the socket is bound to the PF_UNIX protocol family, process it as
 * a UNIX socket.  Otherwise, return and let the vnode be processed as a
 * stream.
 */
	if (ss.family != PF_UNIX)
	    return(0);
	(void) snpf(Lf->type, sizeof(Lf->type), "unix");
	if (Funix)
	    Lf->sf |= SELUNX;
	Lf->is_stream = 0;
	if (!Fsize)
	    Lf->off_def = 1;
	enter_dev_ch(print_kptr(sa, (char *)NULL, 0));
/*
 * Process the local address.
 */

# if	UNIXWAREV<70103
	if ((la = find_unix_sockaddr_un((KA_T)sd.sd_socket))) {
	    if (Sfile && is_file_named(la->sun_path, 0))
		Lf->sf = SELNM;
	}
# else	/* UNIXWAREV>=70103 */
	if (((as = (KA_T)ss.local_addrsz) > 0) && (ka = (KA_T)ss.local_addr))
	{
	    if (as > sizeof(la))
		as = (int)sizeof(la);
	    if (!kread(ka, (char *)&la, as)) {
		la.sun_path[up] = '\0';
		if (la.sun_path[0]) {
		    las = 1;
		    if (Sfile && is_file_named(la.sun_path, 0))
			Lf->sf = SELNM;
		}
	    }
	}
# endif	/* UNIXWAREV<70103 */

/*
 * Process the remote address.
 */

# if	UNIXWAREV<70103
	if ((ra = find_unix_sockaddr_un((KA_T)ss.conn_ux))) {
	    if (Sfile && is_file_named(ra->sun_path, 0))
		Lf->sf = SELNM;
	}
# else	/* UNIXWAREV>=70103 */
	if (((as = (KA_T)ss.remote_addrsz) > 0) && (ka = (KA_T)ss.remote_addr))
	{
	    if (as > sizeof(la))
		as = (int)sizeof(ra);
	    if (!kread(ka, (char *)&ra, as)) {
		ra.sun_path[up] = '\0';
		if (ra.sun_path[0]) {
		    ras = 1;
		    if (Sfile && is_file_named(ra.sun_path, 0))
			Lf->sf = SELNM;
		}
	    }
	}
# endif	/* UNIXWAREV<70103 */

/*
 * Start Namech[] with the service type, converted to a name, ala netstat.
 */
	switch (ss.servtype) {
	case T_COTS:
	case T_COTS_ORD:
	    ty = "stream";
	    break;
	case T_CLTS:
	    ty = "dgram";
	    break;
	default:
	    ty = (char *)NULL;
	}
	if (ty) {
	    (void) snpf(Namech, Namechl, "%s", ty);
	    ty = ":";
	} else {
	    Namech[0] = '\0';
	    ty = "";
	}
/*
 * Add names to Namech[].
 */

#if	UNIXWAREV<70103
	if (la && la->sun_path[0]) {
	    ep = endnm(&sz);
	    (void) snpf(ep, sz, "%s%s", ty, la->sun_path);
	}
#else	/* UNIXWAREV>=70103 */
	if (las) {
	    ep = endnm(&sz);
	    (void) snpf(ep, sz, "%s%s", ty, la.sun_path);
	}
#endif	/* UNIXWAREV<70103 */

	ep = endnm(&sz);

#if	UNIXWAREV<70103
	if (ra && ra->sun_path[0])
	    (void) snpf(ep, sz, "->%s", ra->sun_path);
#else	/* UNIXWAREV>=70103 */
	if (ras)
	    (void) snpf(ep, sz, "->%s", ra.sun_path);
#endif	/* UNIXWAREV<70103 */

	else if ((ka = (KA_T)ss.conn_ux))
	    (void) snpf(ep, sz, "->%s", print_kptr(ka, (char *)NULL, 0));
	if (Namech[0])
	    enter_nm(Namech);
	return(1);
}


# if	UNIXWAREV<70103
/*
 * find_unix_sockaddr_un() -- find UNIX socket address structure
 *
 */

static struct sockaddr_un *
find_unix_sockaddr_un(ka)
	KA_T ka;			/* socket's kernel address */
{
	static struct soreq *al = (struct soreq *)NULL;
	static int alct = 0;
	int i;

	if (!al) {
	    MALLOC_S alen, len;
	    char *ch = (char *)NULL;
	    int ct, pct;
	    struct strioctl ioc;
	    int sock = -1;

	    if (alct < 0)
		return((struct sockaddr_un *)NULL);
	/*
	 * If there has been no attempt to acquire the  address list yet,
	 * do so.
	 *
	 * Get a SOCK_STREAM PF_UNIX socket descriptor and use ioctl() to
	 * send a stream message to acquire the list of PF_UNIX addresses.
	 */
	    if ((sock = socket (PF_UNIX, SOCK_STREAM, 0)) < 0) {

	    /*
	     * Some error was detected.  Return allocated resources and
	     * indicate that no further attempts need be made.
	     */

find_err_exit:

		alct = -1;
		if (sock >= 0)
		    close(sock);
		if (ch)
		    (void) free((FREE_P *)ch);
		return((struct sockaddr_un *)NULL);
	    }
	/*
	 * Read the address list.  Before starting, get an estimate of its
	 * size and add a small safety margin.
	 */
	    if ((ct = ioctl(sock, SI_UX_COUNT, 0)) < 0)
		goto find_err_exit;
	    ct += 32;
	    pct = 0;
	    do {
		if (ct > pct) {

		/*
		 * If the previously allocated space is insufficient,
		 * or if none has been allocated, allocate space.
		 */
		    alen = (MALLOC_S)(ct * sizeof(struct soreq));
		    if (ch)
			ch = (char *)realloc((MALLOC_P *)ch, alen);
		    else
			ch = (char *)malloc(alen);
		    if (!ch)
			goto find_err_exit;
		    pct = ct;
		}
	   /*
	    * Read the address list into the allocated space.
	    */
		ioc.ic_cmd = SI_UX_LIST;
		ioc.ic_dp = ch;
		ioc.ic_len = (int)alen;
		ioc.ic_timout = 0;
		if ((ct = ioctl(sock, I_STR, &ioc)) < 0)
		    goto find_err_exit;
	    } while (ct > pct);
	/*
	 * The list has been acquired.  Free any excess space pre-allocated to
	 * it, then save its address.   Close the stream socket.
	 */
	    alct = ct;
	    if ((len = (MALLOC_S)(alct * sizeof(struct soreq))) < alen) {
		if (!(ch = (char *)realloc((MALLOC_P *)ch, len)))
		    goto find_err_exit;
	    }
	    al = (struct soreq *)ch;
	    close(sock);
	}
/*
 * Search a previously acquired address list, based on the supplied kernel
 * socket address.  If an entry is found, return a pointer to it, making
 * sure the path it contains is terminated.
 */
	for (i = 0; i < alct; i++) {
	    if ((KA_T)al[i].so_addr == ka)
		break;
	}
	if (i >= alct || !al[i].sockaddr.sun_path[0])
	    return((struct sockaddr_un *)NULL);
	al[i].sockaddr.sun_path[(int)(sizeof(al[i].sockaddr.sun_path) - 1)]
	    = '\0';
	return(&al[i].sockaddr);
}
# endif	/* UNIXWAREV<70103 */
#endif	/* UNIXWAREV>=70101 */