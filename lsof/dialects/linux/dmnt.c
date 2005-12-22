/*
 * dmnt.c -- Linux mount support functions for /proc-based lsof
 */


/*
 * Copyright 1997 Purdue Research Foundation, West Lafayette, Indiana
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

#ifndef	lint
static char copyright[] =
"@(#) Copyright 1997 Purdue Research Foundation.\nAll rights reserved.\n";
static char *rcsid = "$Id: dmnt.c,v 1.12 2003/06/13 09:53:13 abe Exp $";
#endif


#include "lsof.h"


/*
 * Local function prototypes
 */

_PROTOTYPE(static char *cvtoe,(char *os));


/*
 * Local static definitions
 */

static struct mounts *Lmi = (struct mounts *)NULL;	/* local mount info */
static int Lmist = 0;					/* Lmi status */


/*
 * cvtoe() -- convert octal-escaped characters in string
 */

static char *
cvtoe(os)
	char *os;			/* original string */
{
	int c, cl, cx, ol, ox, tx;
	char *cs;
	int tc;
/*
 * Allocate space for a copy of the string in which octal-escaped characters
 * can be replaced by the octal value -- e.g., \040 with ' '.  Leave room for
 * a '\0' terminator.
 */
	if (!(ol = (int)strlen(os)))
	   return((char *)NULL);
	if (!(cs = (char *)malloc(ol + 1))) {
	    (void) fprintf(stderr,
		"%s: can't allocate %d bytes for octal-escaping.\n",
		Pn, ol + 1);
	    Exit(1);
	}
/*
 * Copy the string, replacing octal-escaped characters as they are found.
 */
	for (cx = ox = 0, cl = ol; ox < ol; ox++) {
	    if (((c = (int)os[ox]) == (int)'\\') && ((ox + 3) < ol)) {

	    /*
	     * The beginning of an octal-escaped character has been found.
	     *
	     * Convert the octal value to a character value.
	     */
		for (tc = 0, tx = 1; os[ox + tx] && (tx < 4); tx++) {
		    if (((int)os[ox + tx] < (int)'0')
		    ||  ((int)os[ox + tx] > (int)'7'))
		    {

		    /*
		     * The escape isn't followed by octets, so ignore the
		     * escape and just copy it.
		     */
			break;
		    }
		    tc <<= 3;
		    tc += (int)(os[ox + tx] - '0');
		}
		if (tx == 4) {

		/*
		 * If three octets (plus the escape) were assembled, use their
		 * character-forming result.
		 *
		 * Otherwise copy the escape and what follows it until another
		 * escape is found.
		 */
		    ox += 3;
		    c = (tc & 0xff);
		}
	    }
	    if (cx >= cl) {

	    /*
	     * Expand the copy string, as required.  Leave room for a '\0'
	     * terminator.
	     */
		cl += 64;		/* (Make an arbitrary increase.) */
		if (!(cs = (char *)realloc(cs, cl + 1))) {
		    (void) fprintf(stderr,
			"%s: can't realloc %d bytes for octal-escaping.\n",
			Pn, cl + 1);
		    Exit(1);
		}
	    }
	/*
	 * Copy the character.
	 */
	    cs[cx++] = (char)c;
	}
/*
 * Terminate the copy and return its pointer.
 */
	cs[cx] = '\0';
	return(cs);
}


/*
 * readmnt() - read mount table
 */

struct mounts *
readmnt()
{
	char buf[MAXPATHLEN], *cp, **fp;
	char *dn = (char *)NULL;
	char *fp0 = (char *)NULL;
	char *fp1 = (char *)NULL;
	char *ln;
	struct mounts *mp;
	FILE *ms;
	struct stat sb;

	if (Lmi || Lmist)
	    return(Lmi);
/*
 * Open access to /proc/mounts
 */
	(void) snpf(buf, sizeof(buf), "%s/mounts", PROCFS);
	if (!(ms = fopen(buf, "r"))) {
	    (void) fprintf(stderr, "%s: can't fopen(%s)\n", Pn, buf);
	    Exit(1);
	}
/*
 * Read mount table entries.
 */
	while (fgets(buf, sizeof(buf), ms)) {
	    if (get_fields(buf, (char *)NULL, &fp) < 3
	    ||  !fp[0] || !fp[1] || !fp[2])
		continue;
	/*
	 * Convert octal-escaped characters in the device name and mounted-on
	 * path name.
	 */
	    if (fp0) {
		(void) free((FREE_P *)fp0);
		fp0 = (char *)NULL;
	    }
	    if (fp1) {
		(void) free((FREE_P *)fp1);
		fp1 = (char *)NULL;
	    }
	    if (!(fp0 = cvtoe(fp[0])) || !(fp1 = cvtoe(fp[1])))
		continue;
	/*
	 * Ignore an entry with a colon in the device name, followed by
	 * "(pid*" -- it's probably an automounter entry.
	 *
	 * Ignore autofs, pipefs, and sockfs entries.
	 */
	    if ((cp = strchr(fp0, ':')) && !strncasecmp(++cp, "(pid", 4))
		continue;
	    if (!strcasecmp(fp[2], "autofs") || !strcasecmp(fp[2], "pipefs")
	    ||  !strcasecmp(fp[2], "sockfs"))
		continue;
	/*
	 * Interpolate a possible symbolic directory link.
	 */
	    if (dn)
		(void) free((FREE_P *)dn);
	    dn = fp1;
	    fp1 = (char *)NULL;
	    if (!(ln = Readlink(dn))) {
		if (!Fwarn){
		    (void) fprintf(stderr,
			"      Output information may be incomplete.\n");
		}
		continue;
	    }
	    if (ln != dn) {
		(void) free((FREE_P *)dn);
		dn = ln;
	    }
	    if (*dn != '/')
		continue;
	/*
	 * Stat() the directory.
	 */
	    if (statsafely(dn, &sb)) {
		if (!Fwarn) {
		    (void) fprintf(stderr, "%s: WARNING: can't stat() ", Pn);
		    safestrprt(fp[2], stderr, 0);
		    (void) fprintf(stderr, " file system ");
		    safestrprt(dn, stderr, 1);
		    (void) fprintf(stderr,
			"      Output information may be incomplete.\n");
		}
		continue;
	    }
	/*
	 * Allocate and fill a local mount structure.
	 */
	    if (!(mp = (struct mounts *)malloc(sizeof(struct mounts)))) {
		(void) fprintf(stderr,
		    "%s: can't allocate mounts struct for: ", Pn);
		safestrprt(dn, stderr, 1);
		Exit(1);
	    }
	    mp->dir = dn;
	    dn = (char *)NULL;
	    mp->next = Lmi;
	    mp->dev = sb.st_dev;
	    mp->rdev = sb.st_rdev;
	    mp->inode = sb.st_ino;
	    mp->mode = sb.st_mode;
	    if (strcasecmp(fp[2], "nfs") == 0) {
		HasNFS = 1;
		mp->ty = N_NFS;
	    } else
		mp->ty = N_REGLR;
	/*
	 * Interpolate a possible file system (mounted-on) device name link.
	 */
	    dn = fp0;
	    fp0 = (char *)NULL;
	    mp->fsname = dn;
	    ln = Readlink(dn);
	    dn = (char *)NULL;
	/*
	 * Stat() the file system (mounted-on) name and add file system
	 * information to the local mount table entry.
	 */
	    if (!ln || statsafely(ln, &sb))
		sb.st_mode = 0;
	    mp->fsnmres = ln;
	    mp->fs_mode = sb.st_mode;
	    Lmi = mp;
	}
/*
 * Clean up and return the local mount info table address.
 */
	(void) fclose(ms);
	if (dn)
	    (void) free((FREE_P *)dn);
	if (fp0)
	    (void) free((FREE_P *)fp0);
	if (fp1)
	    (void) free((FREE_P *)fp1);
	Lmist = 1;
	return(Lmi);
}
