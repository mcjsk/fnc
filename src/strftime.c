/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.3 $   $State: Exp $
 *
 * Public-domain relatively quick-and-dirty implemenation of
 * ANSI library routine for System V Unix systems.
 *
 * Arnold Robbins
 *
   *****************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 * (Note: this routine is provided as is, without support for those sites that
 *    do not have strftime in their library)
 *
 *    Syd Weinstein, Elm Coordinator
 *    elm@DSI.COM            dsinc!elm
 *
   *****************************************************************************
 * $Log: strftime.c,v $
 * Revision 1.3  1993/10/09  19:38:51  smace
 * Update to elm 2.4 pl23 release version
 *
 * Revision 5.8  1993/08/23  02:46:51  syd
 * Test ANSI_C, not __STDC__ (which is not set on e.g. AIX).
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.7  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.6  1993/08/03  19:20:31  syd
 * Implement new timezone handling.  New file lib/get_tz.c with new timezone
 * routines.  Added new TZMINS_USE_xxxxxx and TZNAME_USE_xxxxxx configuration
 * definitions.  Obsoleted TZNAME, ALTCHECK, and TZ_MINUTESWEST configuration
 * definitions.  Updated Configure.  Modified lib/getarpdate.c and
 * lib/strftime.c to use new timezone routines.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.5  1993/06/10  03:17:45  syd
 * Change from TZNAME_MISSING to TZNAME
 * From: Syd via request from Dan Blanchard
 *
 * Revision 5.4  1993/05/08  19:56:45  syd
 * update to newer version
 * From: Syd
 *
 * Revision 5.3  1993/04/21  01:42:23  syd
 * avoid name conflicts on min and max
 *
 * Revision 5.2  1993/04/16  04:29:34  syd
 * attempt to bsdize a bit strftime
 * From: many via syd
 *
 * Revision 5.1  1993/01/27  18:52:15  syd
 * Initial checkin of contributed public domain routine.
 * This routine is provided as is and not covered by Elm Copyright.
   ****************************************************************************/

/*
 * strftime.c
 *
 * Public-domain relatively quick-and-dirty implementation of
 * ANSI library routine for System V Unix systems.
 *
 * It's written in old-style C for maximal portability.
 * However, since I'm used to prototypes, I've included them too.
 *
 * If you want stuff in the System V ascftime routine, add the SYSV_EXT define.
 * For extensions from SunOS, add SUNOS_EXT.
 * For stuff needed to implement the P1003.2 date command, add POSIX2_DATE.
 * For complete POSIX semantics, add POSIX_SEMANTICS.
 *
 * The code for %c, %x, and %X is my best guess as to what's "appropriate".
 * This version ignores LOCALE information.
 * It also doesn't worry about multi-byte characters.
 * So there.
 *
 * This file is also shipped with GAWK (GNU Awk), gawk specific bits of
 * code are included if GAWK is defined.
 *
 * Arnold Robbins
 * January, February, March, 1991
 * Updated March, April 1992
 * Updated May, 1993
 *
 * Fixes from ado@elsie.nci.nih.gov
 * February 1991, May 1992
 * Fixes from Tor Lillqvist tor@tik.vtt.fi
 * May, 1993
 *
 * Fast-forward to July 2013...
 *
 * stephan@wanderinghorse.net copied these sources from:
 *
 * ftp://ftp-archive.freebsd.org/pub/FreeBSD-Archive/old-releases/i386/1.0-RELEASE/ports/elm/lib/strftime.c
 *
 * And made the following changes:
 *
 * Removed ancient non-ANSI decls. Added some headers to get it to
 * compile for me. Renamed functions to fit into my project. Replaced
 * hard tabs with 4 spaces. Added #undefs for all file-private #defines
 * to safe-ify inclusion from/with other files.
 * 
*/

#include <time.h>
/* #include <sys/time.h> */
#include <string.h> /* strchr() and friends */
#include <stdio.h> /* sprintf() */
#include <ctype.h> /* toupper(), islower() */

#include "fossil-scm/fossil-config.h"
#include "fossil-scm/fossil-util.h"

#define HAVE_GET_TZ_NAME 0
#if HAVE_GET_TZ_NAME
extern char *get_tz_name();
#endif

#if !defined(BSD) && !defined(_MSC_VER)
extern void tzset (void);
#endif
static int weeknumber (const struct tm *timeptr, int firstweekday);

/* defaults: season to taste */
#define SYSV_EXT    1    /* stuff in System V ascftime routine */
#define SUNOS_EXT    1    /* stuff in SunOS strftime routine */
#define POSIX2_DATE    1    /* stuff in Posix 1003.2 date command */
#define VMS_EXT        1    /* include %v for VMS date format */

#if 0
#define POSIX_SEMANTICS    1    /* call tzset() if TZ changes */
#endif

#ifdef POSIX_SEMANTICS
#include <stdlib.h> /* malloc() and friends */
#endif

#if defined(POSIX2_DATE)
#if ! defined(SYSV_EXT)
#define SYSV_EXT    1
#endif
#if ! defined(SUNOS_EXT)
#define SUNOS_EXT    1
#endif
#endif

#if defined(POSIX2_DATE)
static int iso8601wknum(const struct tm *timeptr);
#endif

#undef strchr    /* avoid AIX weirdness */


#ifdef __GNUC__
#define inline    __inline__
#else
#define inline    /**/
#endif

#define range(low, item, hi)    maximum(low, minimum(item, hi))

/* minimum --- return minimum of two numbers */

static inline int
minimum(int a, int b)
{
    return (a < b ? a : b);
}

/* maximum --- return maximum of two numbers */

static inline int
maximum(int a, int b)
{
    return (a > b ? a : b);
}

/* strftime --- produce formatted time */

fsl_size_t
fsl_strftime(char *s, fsl_size_t maxsize, const char *format, const struct tm *timeptr)
{
    char *endp = s + maxsize;
    char *start = s;
    char tbuf[100];
    int i;
    static short first = 1;
#ifdef POSIX_SEMANTICS
    static char *savetz = NULL;
    static int savetzlen = 0;
    char *tz;
#endif /* POSIX_SEMANTICS */

    /* various tables, useful in North America */
    static char *days_a[] = {
        "Sun", "Mon", "Tue", "Wed",
        "Thu", "Fri", "Sat",
    };
    static char *days_l[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday",
    };
    static char *months_a[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    static char *months_l[] = {
        "January", "February", "March", "April",
        "May", "June", "July", "August", "September",
        "October", "November", "December",
    };
    static char *ampm[] = { "AM", "PM", };

    if (s == NULL || format == NULL || timeptr == NULL || maxsize == 0)
        return 0;

    if (strchr(format, '%') == NULL && strlen(format) + 1 >= maxsize)
        return 0;

#ifndef POSIX_SEMANTICS
    if (first) {
        tzset();
        first = 0;
    }
#else    /* POSIX_SEMANTICS */
    tz = getenv("TZ");
    if (first) {
        if (tz != NULL) {
            int tzlen = strlen(tz);

            savetz = (char *) malloc(tzlen + 1);
            if (savetz != NULL) {
                savetzlen = tzlen + 1;
                strcpy(savetz, tz);
            }
        }
        tzset();
        first = 0;
    }
    /* if we have a saved TZ, and it is different, recapture and reset */
    if (tz && savetz && (tz[0] != savetz[0] || strcmp(tz, savetz) != 0)) {
        i = strlen(tz) + 1;
        if (i > savetzlen) {
            savetz = (char *) realloc(savetz, i);
            if (savetz) {
                savetzlen = i;
                strcpy(savetz, tz);
            }
        } else
            strcpy(savetz, tz);
        tzset();
    }
#endif    /* POSIX_SEMANTICS */

    for (; *format && s < endp - 1; format++) {
        tbuf[0] = '\0';
        if (*format != '%') {
            *s++ = *format;
            continue;
        }
    again:
        switch (*++format) {
        case '\0':
            *s++ = '%';
            goto out;

        case '%':
            *s++ = '%';
            continue;

        case 'a':    /* abbreviated weekday name */
            if (timeptr->tm_wday < 0 || timeptr->tm_wday > 6)
                strcpy(tbuf, "?");
            else
                strcpy(tbuf, days_a[timeptr->tm_wday]);
            break;

        case 'A':    /* full weekday name */
            if (timeptr->tm_wday < 0 || timeptr->tm_wday > 6)
                strcpy(tbuf, "?");
            else
                strcpy(tbuf, days_l[timeptr->tm_wday]);
            break;

#ifdef SYSV_EXT
        case 'h':    /* abbreviated month name */
#endif
        case 'b':    /* abbreviated month name */
            if (timeptr->tm_mon < 0 || timeptr->tm_mon > 11)
                strcpy(tbuf, "?");
            else
                strcpy(tbuf, months_a[timeptr->tm_mon]);
            break;

        case 'B':    /* full month name */
            if (timeptr->tm_mon < 0 || timeptr->tm_mon > 11)
                strcpy(tbuf, "?");
            else
                strcpy(tbuf, months_l[timeptr->tm_mon]);
            break;

        case 'c':    /* appropriate date and time representation */
            sprintf(tbuf, "%s %s %2d %02d:%02d:%02d %d",
                days_a[range(0, timeptr->tm_wday, 6)],
                months_a[range(0, timeptr->tm_mon, 11)],
                range(1, timeptr->tm_mday, 31),
                range(0, timeptr->tm_hour, 23),
                range(0, timeptr->tm_min, 59),
                range(0, timeptr->tm_sec, 61),
                timeptr->tm_year + 1900);
            break;

        case 'd':    /* day of the month, 01 - 31 */
            i = range(1, timeptr->tm_mday, 31);
            sprintf(tbuf, "%02d", i);
            break;

        case 'H':    /* hour, 24-hour clock, 00 - 23 */
            i = range(0, timeptr->tm_hour, 23);
            sprintf(tbuf, "%02d", i);
            break;

        case 'I':    /* hour, 12-hour clock, 01 - 12 */
            i = range(0, timeptr->tm_hour, 23);
            if (i == 0)
                i = 12;
            else if (i > 12)
                i -= 12;
            sprintf(tbuf, "%02d", i);
            break;

        case 'j':    /* day of the year, 001 - 366 */
            sprintf(tbuf, "%03d", timeptr->tm_yday + 1);
            break;

        case 'm':    /* month, 01 - 12 */
            i = range(0, timeptr->tm_mon, 11);
            sprintf(tbuf, "%02d", i + 1);
            break;

        case 'M':    /* minute, 00 - 59 */
            i = range(0, timeptr->tm_min, 59);
            sprintf(tbuf, "%02d", i);
            break;

        case 'p':    /* am or pm based on 12-hour clock */
            i = range(0, timeptr->tm_hour, 23);
            if (i < 12)
                strcpy(tbuf, ampm[0]);
            else
                strcpy(tbuf, ampm[1]);
            break;

        case 'S':    /* second, 00 - 61 */
            i = range(0, timeptr->tm_sec, 61);
            sprintf(tbuf, "%02d", i);
            break;

        case 'U':    /* week of year, Sunday is first day of week */
            sprintf(tbuf, "%d", weeknumber(timeptr, 0));
            break;

        case 'w':    /* weekday, Sunday == 0, 0 - 6 */
            i = range(0, timeptr->tm_wday, 6);
            sprintf(tbuf, "%d", i);
            break;

        case 'W':    /* week of year, Monday is first day of week */
            sprintf(tbuf, "%d", weeknumber(timeptr, 1));
            break;

        case 'x':    /* appropriate date representation */
            sprintf(tbuf, "%s %s %2d %d",
                days_a[range(0, timeptr->tm_wday, 6)],
                months_a[range(0, timeptr->tm_mon, 11)],
                range(1, timeptr->tm_mday, 31),
                timeptr->tm_year + 1900);
            break;

        case 'X':    /* appropriate time representation */
            sprintf(tbuf, "%02d:%02d:%02d",
                range(0, timeptr->tm_hour, 23),
                range(0, timeptr->tm_min, 59),
                range(0, timeptr->tm_sec, 61));
            break;

        case 'y':    /* year without a century, 00 - 99 */
            i = timeptr->tm_year % 100;
            sprintf(tbuf, "%d", i);
            break;

        case 'Y':    /* year with century */
            sprintf(tbuf, "%d", 1900 + timeptr->tm_year);
            break;

#if HAVE_GET_TZ_NAME
          case 'Z':    /* time zone name or abbrevation */
            strcpy(tbuf, get_tz_name(timeptr));
            break;
#endif
            
#ifdef SYSV_EXT
        case 'n':    /* same as \n */
            tbuf[0] = '\n';
            tbuf[1] = '\0';
            break;

        case 't':    /* same as \t */
            tbuf[0] = '\t';
            tbuf[1] = '\0';
            break;

        case 'D':    /* date as %m/%d/%y */
            fsl_strftime(tbuf, sizeof tbuf, "%m/%d/%y", timeptr);
            break;

        case 'e':    /* day of month, blank padded */
            sprintf(tbuf, "%2d", range(1, timeptr->tm_mday, 31));
            break;

        case 'r':    /* time as %I:%M:%S %p */
            fsl_strftime(tbuf, sizeof tbuf, "%I:%M:%S %p", timeptr);
            break;

        case 'R':    /* time as %H:%M */
            fsl_strftime(tbuf, sizeof tbuf, "%H:%M", timeptr);
            break;

        case 'T':    /* time as %H:%M:%S */
            fsl_strftime(tbuf, sizeof tbuf, "%H:%M:%S", timeptr);
            break;
#endif

#ifdef SUNOS_EXT
        case 'k':    /* hour, 24-hour clock, blank pad */
            sprintf(tbuf, "%2d", range(0, timeptr->tm_hour, 23));
            break;

        case 'l':    /* hour, 12-hour clock, 1 - 12, blank pad */
            i = range(0, timeptr->tm_hour, 23);
            if (i == 0)
                i = 12;
            else if (i > 12)
                i -= 12;
            sprintf(tbuf, "%2d", i);
            break;
#endif


#ifdef VMS_EXT
        case 'v':    /* date as dd-bbb-YYYY */
            sprintf(tbuf, "%2d-%3.3s-%4d",
                range(1, timeptr->tm_mday, 31),
                months_a[range(0, timeptr->tm_mon, 11)],
                timeptr->tm_year + 1900);
            for (i = 3; i < 6; i++)
                if (islower((int)tbuf[i]))
                    tbuf[i] = toupper((int)tbuf[i]);
            break;
#endif


#ifdef POSIX2_DATE
        case 'C':
            sprintf(tbuf, "%02d", (timeptr->tm_year + 1900) / 100);
            break;


        case 'E':
        case 'O':
            /* POSIX locale extensions, ignored for now */
            goto again;

        case 'V':    /* week of year according ISO 8601 */
#if defined(GAWK) && defined(VMS_EXT)
        {
            extern int do_lint;
            extern void warning();
            static int warned = 0;

            if (! warned && do_lint) {
                warned = 1;
                warning(
    "conversion %%V added in P1003.2/11.3; for VMS style date, use %%v");
            }
        }
#endif
            sprintf(tbuf, "%d", iso8601wknum(timeptr));
            break;

        case 'u':
        /* ISO 8601: Weekday as a decimal number [1 (Monday) - 7] */
            sprintf(tbuf, "%d", timeptr->tm_wday == 0 ? 7 :
                    timeptr->tm_wday);
            break;
#endif    /* POSIX2_DATE */
        default:
            tbuf[0] = '%';
            tbuf[1] = *format;
            tbuf[2] = '\0';
            break;
        }
        i = strlen(tbuf);
        if (i){
            if (s + i < endp - 1) {
                strcpy(s, tbuf);
                s += i;
            } else return 0;
            /* reminder: above IF originally had ambiguous else
               placement (no braces).  This placement _appears_ to be
               correct.*/
        }
    }
out:
    if (s < endp && *format == '\0') {
        *s = '\0';
        return (s - start);
    } else
        return 0;
}

#ifdef POSIX2_DATE
/* iso8601wknum --- compute week number according to ISO 8601 */

static int
iso8601wknum(const struct tm *timeptr)
{
    /*
     * From 1003.2 D11.3:
     *    If the week (Monday to Sunday) containing January 1
     *    has four or more days in the new year, then it is week 1;
     *    otherwise it is week 53 of the previous year, and the
     *    next week is week 1.
     *
     * ADR: This means if Jan 1 was Monday through Thursday,
     *    it was week 1, otherwise week 53.
     */

    int simple_wknum, jan1day, diff, ret;

    /* get week number, Monday as first day of the week */
    simple_wknum = weeknumber(timeptr, 1) + 1;

    /*
     * With thanks and tip of the hatlo to tml@tik.vtt.fi
     *
     * What day of the week does January 1 fall on?
     * We know that
     *    (timeptr->tm_yday - jan1.tm_yday) MOD 7 ==
     *        (timeptr->tm_wday - jan1.tm_wday) MOD 7
     * and that
     *     jan1.tm_yday == 0
     * and that
     *     timeptr->tm_wday MOD 7 == timeptr->tm_wday
     * from which it follows that. . .
      */
    jan1day = timeptr->tm_wday - (timeptr->tm_yday % 7);
    if (jan1day < 0)
        jan1day += 7;

    /*
     * If Jan 1 was a Monday through Thursday, it was in
     * week 1.  Otherwise it was last year's week 53, which is
     * this year's week 0.
     */
    if (jan1day >= 1 && jan1day <= 4)
        diff = 0;
    else
        diff = 1;
    ret = simple_wknum - diff;
    if (ret == 0)    /* we're in the first week of the year */
        ret = 53;
    return ret;
}
#endif

/* weeknumber --- figure how many weeks into the year */

/* With thanks and tip of the hatlo to ado@elsie.nci.nih.gov */

static int
weeknumber(const struct tm *timeptr, int firstweekday)
{
    if (firstweekday == 0)
        return (timeptr->tm_yday + 7 - timeptr->tm_wday) / 7;
    else
        return (timeptr->tm_yday + 7 -
            (timeptr->tm_wday ? (timeptr->tm_wday - 1) : 6)) / 7;
}

#undef SYSV_EXT
#undef SUNOS_EXT
#undef POSIX2_DATE
#undef VMS_EXT
#undef POSIX_SEMANTICS
#undef adddecl
#undef inline
#undef range
#undef maximum
#undef minimum
#undef HAVE_GET_TZ_NAME
#undef GAWK


/**
   A convenience form of fsl_strftime() which takes its timestamp in
   the form of a Unix Epoc time.
*/
fsl_size_t fsl_strftime_unix(char * dest, fsl_size_t destLen, char const * format,
                             fsl_time_t epochTime, bool convertToLocal){
  time_t orig = (time_t)epochTime;
  struct tm * tim = convertToLocal ? localtime(&orig) : gmtime(&orig);
  return fsl_strftime( dest, destLen, format, tim );
}
