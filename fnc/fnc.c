/*
 * Copyright (c) 2021 Mark Jamsek <mark@jamsek.com>
 * Copyright (c) 2013-2021 Stephan Beal <https://wanderinghorse.net>
 * Copyright (c) 2020 Stefan Sperling <stsp@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This _POSIX_C_SOURCE bit really belongs in a config.h, but in the
 * name of expedience...
 */
#if defined __linux__
#  if !defined(_XOPEN_SOURCE)
#    define _XOPEN_SOURCE 700
/*
 * _POSIX_C_SOURCE >= 199309L needed for sigaction(), sigemptyset() on Linux,
 * but glibc docs claim that _XOPEN_SOURCE>=700 has the same effect, PLUS
 * we need _XOPEN_SOURCE>=500 for ncurses wide-char APIs on linux.
 */
#  endif
#  if !defined(_DEFAULT_SOURCE)
#    define _DEFAULT_SOURCE
/* Needed for strsep() on glibc >= 2.19. */
#  endif
#endif


#include <sys/queue.h>
#include <sys/ioctl.h>

#ifdef _WIN32
#include <windows.h>
#define ssleep(x) Sleep(x)
#else
#define ssleep(x) usleep((x) * 1000)
#endif
#include <ctype.h>
#include <curses.h>
#include <panel.h>
#include <locale.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <libgen.h>
#include <regex.h>
#include <signal.h>
#include <wchar.h>
#include <langinfo.h>

#include "libfossil.h"

#define FNC_VERSION	0.2a

/* Utility macros. */
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#if !defined(CTRL)
#define CTRL(key)	((key) & 037)	/* CTRL+<key> input. */
#endif
#define nitems(a)	(sizeof((a)) / sizeof((a)[0]))
#define STRINGIFYOUT(s)	#s
#define STRINGIFY(s)	STRINGIFYOUT(s)

/* Application macros. */
#define PRINT_VERSION	STRINGIFY(FNC_VERSION)
#define DIFF_MAX_CTXT	64		/* Max diff context lines. */
#define SPIN_INTERVAL	200		/* Status line progress indicator. */
#define SPINNER		"\\|/-\0"
#define NULL_DEVICE	"/dev/null"
#define NULL_DEVICELEN (sizeof(NULL_DEVICE)-1)

/* Portability macros. */
#ifdef __OpenBSD__
#define strtol(s, p, b)	strtonum(s, INT_MIN, INT_MAX, (const char **)p)
#endif

#if !defined(__dead)
#define __dead
#endif

#ifndef TAILQ_FOREACH_SAFE
/* Rewrite of OpenBSD 6.9 sys/queue.h for Linux builds. */
#define TAILQ_FOREACH_SAFE(var, head, field, tmp)			\
	for ((var) = ((head)->tqh_first);				\
		(var) != (NULL) && ((tmp) = TAILQ_NEXT(var, field), 1);	\
		(var) = (tmp))
#endif

__dead static void	usage(void);
static void		usage_timeline(void);
static void		usage_diff(void);
static void		usage_blame(void);
static int		fcli_flag_type_arg_cb(fcli_cliflag const *);
static int		cmd_timeline(fcli_command const *);
static int		cmd_diff(fcli_command const *);
static int		cmd_blame(fcli_command const *);

/*
 * Singleton initialising global configuration and state for app startup.
 */
static struct fnc_setup {
	/* Global options. */
	const char	*cmdarg;	/* Retain argv[1] for use/err report. */
	int		 err;		/* Indicate fnc error state. */
	bool		 hflag;		/* Flag if --help is requested. */
	bool		 vflag;		/* Flag if --version is requested. */

	/* Timeline options. */
	struct artifact_types {
		const char	**values;
		short		  nitems;
	} *filter_types;		/* Only load commits of <type>. */
	union {
		const char	 *zlimit;
		int limit;
	} nrecords;			/* Number of commits to load. */
	union {
		const char	 *sym;
		fsl_id_t	  rid;
	} start_commit;			/* Open timeline from this commit. */
	const char	*filter_tag;	/* Only load commits with <tag>. */
	const char	*filter_branch;	/* Only load commits from <branch>. */
	const char	*filter_user;	/* Only load commits from <user>. */
	const char	*filter_type;	/* Placeholder for repeatable types. */
	bool		 utc;		/* Display UTC sans user local time. */

	/* Diff options. */
	const char	*context;	/* Number of context lines. */
	bool		 ws;		/* Ignore whitespace-only changes. */
	bool		 nocolour;	/* Disable colour in diff output. */
	bool		 quiet;		/* Disable verbose diff output. */
	bool		 invert;	/* Toggle inverted diff output. */

	/* Blame options. */
	const char	*path;		/* Show blame of REQUIRED <path> arg. */

	/* Command line flags and help. */
	fcli_help_info	  fnc_help;			/* Global help. */
	fcli_cliflag	  cliflags_global[3];		/* Global options. */
	fcli_command	  cmd_args[4];			/* App commands. */
	void		(*fnc_usage_cb[3])(void);	/* Command usage. */
	fcli_cliflag	  cliflags_timeline[10];	/* Timeline options. */
	fcli_cliflag	  cliflags_diff[7];		/* Diff options. */
	fcli_cliflag	  cliflags_blame[2];		/* Blame options. */
} fnc_init = {
	NULL,		/* cmdarg copy of argv[1] to aid usage/error report. */
	0,		/* err fnc error state. */
	false,		/* hflag if --help is requested. */
	false,		/* vflag if --version is requested. */
	NULL,		/* filter_types defaults to indiscriminate. */
	{0},		/* nrecords defaults to all commits. */
	{0},		/* start_commit defaults to latest leaf. */
	NULL,		/* filter_tag defaults to indiscriminate. */
	NULL,		/* filter_branch defaults to indiscriminate. */
	NULL,		/* filter_user defaults to indiscriminate. */
	NULL,		/* filter_type temporary placeholder. */
	false,		/* utc defaults to off (i.e., show user local time). */
	NULL,		/* context defaults to five context lines. */
	false,		/* ws defaults to acknowledge whitespace. */
	false,		/* nocolour defaults to off (i.e., use diff colours). */
	false,		/* quiet defaults to off (i.e., verbose diff is on). */
	false,		/* invert diff defaults to off. */
	NULL,		/* path blame command required argument. */

	{ /* fnc_help global app help details. */
	    "A read-only ncurses browser for Fossil repositories in the "
	    "terminal.", NULL, usage
	},

	{ /* cliflags_global global app options. */
	    FCLI_FLAG_BOOL("h", "help", &fnc_init.hflag,
	    "Display program help and usage then exit."),
	    FCLI_FLAG_BOOL("v", "version", &fnc_init.vflag,
	    "Display program version number and exit."),
	    fcli_cliflag_empty_m
	},

	{ /* cmd_args available app commands. */
	    {"timeline", "Show chronologically descending commit history of "
	    "the repository.", cmd_timeline, fnc_init.cliflags_timeline},
	    {"diff", "Show differences between versioned files in the "
	    "repository.", cmd_diff, fnc_init.cliflags_diff},
	    {"blame", "Show commit attribution history for each line of a "
	    "file.", cmd_blame, fnc_init.cliflags_blame},
	    {NULL,NULL,NULL, NULL}	/* Sentinel. */
	},

	/* fnc_usage_cb individual command usage details. */
	{&usage_timeline, &usage_diff, &usage_blame},

	{ /* cliflags_timeline timeline command related options. */
	    FCLI_FLAG("T", "tag", "<tag>", &fnc_init.filter_tag,
            "Only display commits with T cards containing <tag>."),
	    FCLI_FLAG("b", "branch", "<branch>", &fnc_init.filter_branch,
            "Only display commits that reside on the given <branch>."),
	    FCLI_FLAG("c", "commit", "<sym>", &fnc_init.start_commit.sym,
            "Open the timeline from commit <sym>. Common valid symbols are:\n"
            "\tSHA{1,3} hash\n"
            "\tSHA{1,3} unique prefix\n"
            "\tbranch\n"
            "\ttag:TAG\n"
            "\troot:BRANCH\n"
            "\tISO8601 date\n"
            "\tISO8601 timestamp\n"
            "\t{tip,current,prev,next}\n    "
            "For a complete list of symbols see Fossil's Check-in Names:\n    "
            "https://fossil-scm.org/home/doc/trunk/www/checkin_names.wiki"),
	    FCLI_FLAG_BOOL("h", "help", NULL,
            "Display timeline command help and usage."),
	    FCLI_FLAG("n", "limit", "<n>", &fnc_init.nrecords.zlimit,
            "Limit display to <n> latest commits; defaults to entire history "
            "of\n    current checkout. Negative values are a no-op."),
	    FCLI_FLAG_X("t", "type", "<type>", &fnc_init.filter_type,
	    fcli_flag_type_arg_cb,
            "Only display <type> commits. Valid types are:\n"
            "\tci - check-in\n"
            "\tw  - wiki\n"
            "\tt  - ticket\n"
            "\te  - technote\n"
            "\tf  - forum post\n"
            "    n.b. This is a repeatable flag (e.g., -t ci -t w)."),
	    FCLI_FLAG("u", "username", "<user>", &fnc_init.filter_user,
            "Only display commits authored by <username>."),
	    FCLI_FLAG_BOOL("z", "utc", &fnc_init.utc,
            "Use UTC (instead of local) time."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_timeline. */

	{ /* cliflags_diff diff command related options. */
	    FCLI_FLAG_BOOL("c", "no-colour", &fnc_init.nocolour,
            "Disable coloured diff output, which is enabled by default on\n    "
            "supported terminals. Colour can also be toggled with the 'c' "
            "\n    key binding in diff view when this option is not used."),
	    FCLI_FLAG_BOOL("h", "help", NULL,
            "Display diff command help and usage."),
	    FCLI_FLAG_BOOL("i", "invert", &fnc_init.invert,
            "Invert difference between artifacts. Inversion can also be "
            "toggled\n    with the 'i' key binding in diff view."),
	    FCLI_FLAG_BOOL("q", "quiet", &fnc_init.quiet,
            "Disable verbose diff output; that is, do not output complete"
            " content\n    of newly added or deleted files. Verbosity can also"
            " be toggled with\n    the 'v' key binding in diff view."),
	    FCLI_FLAG_BOOL("w", "whitespace", &fnc_init.ws,
            "Ignore whitespace-only changes when displaying diff. This option "
            "can\n    also be toggled with the 'w' key binding in diff view."),
	    FCLI_FLAG("x", "context", "<n>", &fnc_init.context,
            "Show <n> context lines when displaying diff; <n> is capped at 64."
            "\n    Negative values are a no-op."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_diff. */

	{ /* cliflags_blame blame command related options. */
	    FCLI_FLAG_BOOL("h", "help", NULL,
            "Display blame command help and usage."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_blame. */
};

enum fsl_list_object {
	FNC_ARTIFACT_OBJ,
	FNC_COLOUR_OBJ
};

enum date_string {
	ISO8601_DATE_ONLY = 10,
	ISO8601_TIMESTAMP = 20
};

enum fnc_view_id {
	FNC_VIEW_TIMELINE,
	FNC_VIEW_DIFF,
	FNC_VIEW_BLAME,
};

enum fnc_search_mvmnt {
	SEARCH_DONE,
	SEARCH_FORWARD,
	SEARCH_REVERSE
};

enum fnc_search_state {
	SEARCH_WAITING,
	SEARCH_CONTINUE,
	SEARCH_COMPLETE,
	SEARCH_NO_MATCH,
	SEARCH_FOR_END
};

enum diff_colours {
	FNC_DIFF_META = 1,
	FNC_DIFF_MINUS,
	FNC_DIFF_PLUS,
	FNC_DIFF_CHNK,
};

struct fnc_colour {
	regex_t	regex;
	uint8_t	scheme;
};

struct fsl_list_state {
	enum fsl_list_object	obj;
};

struct fnc_commit_artifact {
	fsl_buffer	 wiki;
	fsl_buffer	 pwiki;
	fsl_list	 changeset;
	fsl_uuid_str	 uuid;
	fsl_uuid_str	 puuid;
	fsl_id_t	 rid;
	fsl_id_t	 prid;
	char		*user;
	char		*timestamp;
	char		*comment;
	char		*branch;
	char		*type;
};

struct fsl_file_artifact {
	fsl_card_F		*fc;
	enum fsl_ckout_change_e	 change;
};

TAILQ_HEAD(commit_tailhead, commit_entry);
struct commit_entry {
	TAILQ_ENTRY(commit_entry)	 entries;
	struct fnc_commit_artifact	*commit;
	int				 idx;
};

struct commit_queue {
	struct commit_tailhead	head;
	int			ncommits;
};

pthread_mutex_t fnc_mutex = PTHREAD_MUTEX_INITIALIZER;

struct fnc_tl_thread_cx {
	struct commit_queue	 *commits;
	struct commit_entry	**first_commit_onscreen;
	struct commit_entry	**selected_commit;
	fsl_db			 *db;
	fsl_stmt		 *q;
	regex_t			 *regex;
	enum fnc_search_state	 *search_status;
	enum fnc_search_mvmnt	 *searching;
	int			  spin_idx;
	int			  ncommits_needed;
	bool			  endjmp;
	bool			  timeline_end;
	sig_atomic_t		 *quit;
	pthread_cond_t		  commit_consumer;
	pthread_cond_t		  commit_producer;
};

struct fnc_tl_view_state {
	struct fnc_tl_thread_cx	 thread_cx;
	struct commit_queue	 commits;
	struct commit_entry	*first_commit_onscreen;
	struct commit_entry	*last_commit_onscreen;
	struct commit_entry	*selected_commit;
	struct commit_entry	*matched_commit;
	struct commit_entry	*search_commit;
	const char		*curr_ckout_uuid;
	int			 selected_idx;
	sig_atomic_t		 quit;
	pthread_t		 thread_id;
};

struct fnc_diff_view_state {
	struct fnc_view			*timeline_view;
	struct fnc_commit_artifact	*selected_commit;
	fsl_buffer			 buf;
	fsl_list			 colours;
	FILE				*f;
	int				 first_line_onscreen;
	int				 last_line_onscreen;
	int				 diff_flags;
	int				 context;
	int				 sbs;
	int				 matched_line;
	int				 current_line;
	size_t				 ncols;
	size_t				 nlines;
	off_t				*line_offsets;
	bool				 eof;
	bool				 colour;
};

TAILQ_HEAD(view_tailhead, fnc_view);
struct fnc_view {
	TAILQ_ENTRY(fnc_view)	 entries;
	WINDOW			*window;
	PANEL			*panel;
	struct fnc_view		*parent;
	struct fnc_view		*child;
	union {
		struct fnc_diff_view_state	diff;
		struct fnc_tl_view_state	timeline;
		/* struct fnc_blame_view_state	blame; */
	} state;
	enum fnc_view_id	 vid;
	enum fnc_search_state	 search_status;
	enum fnc_search_mvmnt	 searching;
	int			 nlines;
	int			 ncols;
	int			 start_ln;
	int			 start_col;
	int			 lines;	/* Duplicate curses LINES macro. */
	int			 cols;	/* Duplicate curses COLS macro. */
	bool			 focus_child;
	bool			 active; /* Only 1 parent or child at a time. */
	bool			 egress;
	bool			 started_search;
	regex_t			 regex;
	regmatch_t		 regmatch;

	int	(*show)(struct fnc_view *);
	int	(*input)(struct fnc_view **, struct fnc_view *, int);
	int	(*close)(struct fnc_view *);
	int	(*search_init)(struct fnc_view *);
	int	(*search_next)(struct fnc_view *);
};

static volatile sig_atomic_t rec_sigwinch;
static volatile sig_atomic_t rec_sigpipe;
static volatile sig_atomic_t rec_sigcont;

static void		 fnc_show_version(void);
static int		 init_curses(void);
static struct fnc_view	*view_open(int, int, int, int, enum fnc_view_id);
static int		 open_timeline_view(struct fnc_view *);
static int		 view_loop(struct fnc_view *);
static int		 show_timeline_view(struct fnc_view *);
static void		*tl_producer_thread(void *);
static int		 block_main_thread_signals(void);
static int		 build_commits(struct fnc_tl_thread_cx *);
static int		 signal_tl_thread(struct fnc_view *, int);
static int		 draw_commits(struct fnc_view *);
static void		 parse_emailaddr_username(char **);
static int		 formatln(wchar_t **, int *, const char *, int, int);
static int		 multibyte_to_wchar(const char *, wchar_t **, size_t *);
static int		 write_commit_line(struct fnc_view *,
			    struct fnc_commit_artifact *, int);
static int		 view_input(struct fnc_view **, int *,
			    struct fnc_view *, struct view_tailhead *);
static int		 tl_input_handler(struct fnc_view **, struct fnc_view *,
			    int);
static int		 timeline_scroll_down(struct fnc_view *, int);
static void		 timeline_scroll_up(struct fnc_tl_view_state *, int);
static void		 select_commit(struct fnc_tl_view_state *);
static int		 view_is_parent(struct fnc_view *);
static int		 make_splitscreen(struct fnc_view *);
static int		 make_fullscreen(struct fnc_view *);
static int		 view_split_start_col(int);
static int		 view_search_start(struct fnc_view *);
static int		 tl_search_init(struct fnc_view *);
static int		 tl_search_next(struct fnc_view *);
static bool		 find_commit_match(struct fnc_commit_artifact *,
			    regex_t *);
static int		 init_diff_commit(struct fnc_view **, int,
			    struct fnc_commit_artifact *, struct fnc_view *);
static int		 open_diff_view(struct fnc_view *,
			    struct fnc_commit_artifact *, int, bool, bool, bool,
			    struct fnc_view *);
static int		 set_diff_colours(fsl_list *);
static void		 show_diff_status(struct fnc_view *);
static int		 create_diff(struct fnc_diff_view_state *);
static int		 create_changeset(struct fnc_commit_artifact *);
static int		 write_commit_meta(struct fnc_diff_view_state *);
static int		 wrapline(char *, fsl_size_t ncols_avail,
			    struct fnc_diff_view_state *, off_t *);
static int		 add_line_offset(off_t **, size_t *, off_t);
static int		 diff_commit(fsl_buffer *, struct fnc_commit_artifact *,
			    int, int, int);
static int		 diff_checkout(fsl_buffer *, fsl_id_t, int, int, int);
static int		 write_diff_meta(fsl_buffer *, const char *,
			    fsl_uuid_str, const char *, fsl_uuid_str, int,
			    enum fsl_ckout_change_e);
static int		 diff_file(fsl_buffer *, fsl_buffer *, const char *,
			    fsl_uuid_str, const char *, enum fsl_ckout_change_e,
			    int, int, bool);
static int		 diff_non_checkin(fsl_buffer *, struct
			    fnc_commit_artifact *, int, int, int);
static int		 diff_file_artifact(fsl_buffer *, fsl_id_t,
			    const fsl_card_F *, fsl_id_t, const fsl_card_F *,
			    fsl_ckout_change_e, int, int, int);
static int		 fsl_ckout_file_content(fsl_cx *, char const *,
			    fsl_buffer *);
static int		 show_diff(struct fnc_view *);
static int		 write_diff(struct fnc_view *, char *);
static int		 match_line(const void *, const void *);
static int		 write_matched_line(int *, const char *, int, int,
			    WINDOW *, regmatch_t *);
static void		 draw_vborder(struct fnc_view *);
static int		 diff_input_handler(struct fnc_view **,
			    struct fnc_view *, int);
static int		 set_selected_commit(struct fnc_diff_view_state *,
			    struct commit_entry *);
static int		 diff_search_init(struct fnc_view *);
static int		 diff_search_next(struct fnc_view *);
static int		 view_close(struct fnc_view *);
static int		 close_timeline_view(struct fnc_view *);
static int		 close_diff_view(struct fnc_view *);
static int		 view_resize(struct fnc_view *);
static int		 screen_is_split(struct fnc_view *);
static bool		 screen_is_shared(struct fnc_view *);
static void		 fnc_resizeterm(void);
static int		 join_tl_thread(struct fnc_tl_view_state *);
static void		 fnc_free_commits(struct commit_queue *);
static void		 fnc_commit_artifact_close(struct fnc_commit_artifact*);
static int		 fsl_list_object_free(void *, void *);
static void		 sigwinch_handler(int);
static void		 sigpipe_handler(int);
static void		 sigcont_handler(int);
static char		*fnc_strsep (char **, const char *);

int
main(int argc, const char **argv)
{
	fcli_command	*cmd = NULL;
	fsl_cx		*f = NULL;
	fsl_error	 e = fsl_error_empty;	/* DEBUG */
	int		 rc = 0;

	fnc_init.filter_types =
	    (struct artifact_types *)fsl_malloc(sizeof(struct artifact_types));
	fnc_init.filter_types->values = fsl_malloc(sizeof(char *));
	fnc_init.filter_types->nitems = 0;
	fcli_fax(fnc_init.filter_types->values);
	fcli_fax(fnc_init.filter_types);

	if (!setlocale(LC_CTYPE, ""))
		fsl_fprintf(stderr, "[!] Warning: Can't set locale.\n");

	fnc_init.cmdarg = argv[1];	/* Which cmd to show usage if needed. */
	fcli.clientFlags.verbose = 2;	/* Verbose error reporting. */
	fcli.cliFlags = fnc_init.cliflags_global;
	fcli.appHelp = &fnc_init.fnc_help;
	rc = fcli_setup(argc, argv);
	if (rc)
		goto end;

	if (fnc_init.vflag) {
		fnc_show_version();
		goto end;
	} else if (fnc_init.hflag)
		usage();

	if (argc == 1)
		cmd = &fnc_init.cmd_args[FNC_VIEW_TIMELINE];
	else if (((rc = fcli_dispatch_commands(fnc_init.cmd_args, false)
	    == FSL_RC_NOT_FOUND)) || (rc = fcli_has_unused_args(false))) {
		fnc_init.err = rc;
		usage();
	} else if (rc)
		goto end;

	rc = fcli_fingerprint_check(true);
	if (rc)
		goto end;

	f = fcli_cx();
	if (!fsl_cx_db_repo(f)) {
		rc = fsl_cx_err_set(f, FSL_RC_MISUSE, "repo database required");
		goto end;
	}

	if (cmd != NULL)
		rc = cmd->f(cmd);
end:
	endwin();
	putchar('\n');
	if (rc) {
		if (rc == FCLI_RC_HELP)
			rc = 0;
		else {
			fsl_error_set(&e, rc, NULL);
			fsl_fprintf(stderr, "%s: %s %d\n", fcli_progname(),
			    fsl_rc_cstr(rc), rc);
			fsl_fprintf(stderr, "-> %s\n", e.msg.mem);
		}
	}
	return fcli_end_of_main(rc);
}

static int
cmd_timeline(fcli_command const *argv)
{
	struct fnc_view	*v;
	fsl_cx		*f = fcli_cx();
	fsl_id_t	 rid = -1;
	int		 rc = 0;

	rc = fcli_process_flags(argv->flags);
	if (rc || (rc = fcli_has_unused_flags(false)))
		return rc;

	if (fnc_init.nrecords.zlimit) {
		const char	*ptr, *s;
		s = fnc_init.nrecords.zlimit;
		ptr = NULL;
		errno = 0;

		fnc_init.nrecords.limit = strtol(s, (char **)&ptr, 10);
		if (errno == ERANGE)
			return fsl_cx_err_set(f, FSL_RC_RANGE,
			    "<n> out of range: -n|--limit=%s [%s]", s, ptr);
		else if (errno != 0 || errno == EINVAL)
			return fsl_cx_err_set(f, FSL_RC_MISUSE,
			    "<n> not a number: -n|--limit=%s [%s]", s, ptr);
		else if (ptr && *ptr != '\0')
			return fsl_cx_err_set(f, FSL_RC_MISUSE,
			    "invalid char in <n>: -n|--limit=%s [%s]", s, ptr);
	}

	if (fnc_init.start_commit.sym != NULL) {
		rc = fsl_sym_to_rid(f, fnc_init.start_commit.sym,
		    FSL_SATYPE_CHECKIN, &rid);
		if (rc || rid < 0)
			return fcli_err_set(FSL_RC_TYPE,
			    "artifact [%s] not resolvable to a commit",
			    fnc_init.start_commit.sym);
		else
			fnc_init.start_commit.rid = rid;
	}

	rc = init_curses();
	if (rc)
		return fsl_cx_err_set(f, fsl_errno_to_rc(rc, FSL_RC_ERROR),
		    "can not initialise ncurses");
	v = view_open(0, 0, 0, 0, FNC_VIEW_TIMELINE);
	if (v == NULL)
		return fsl_cx_err_set(f, FSL_RC_ERROR, "view_open() failed");
	rc = open_timeline_view(v);
	if (rc)
		return rc;

	return view_loop(v);
}

static int
init_curses(void)
{
	int rc = 0;

	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	curs_set(0);
#ifndef __linux__
	typeahead(-1);	/* Don't disrupt screen update operations. */
#endif

	if (!fnc_init.nocolour && has_colors()) {
		start_color();
		use_default_colors();
	}

	if ((rc = sigaction(SIGPIPE,
	    &(struct sigaction){{sigpipe_handler}}, NULL)))
		return rc;
	if ((rc = sigaction(SIGWINCH,
	    &(struct sigaction){{sigwinch_handler}}, NULL)))
		return rc;
	if ((rc = sigaction(SIGCONT,
	    &(struct sigaction){{sigcont_handler}}, NULL)))
		return rc;

	return rc;
}

static struct fnc_view *
view_open(int nlines, int ncols, int start_ln, int start_col,
    enum fnc_view_id vid)
{
	struct fnc_view *view = calloc(1, sizeof(*view));

	if (view == NULL)
		return NULL;

	view->vid = vid;
	view->lines = LINES;
	view->cols = COLS;
	view->nlines = nlines ? nlines : LINES - start_ln;
	view->ncols = ncols ? ncols : COLS - start_col;
	view->start_ln = start_ln;
	view->start_col = start_col;
	view->window = newwin(nlines, ncols, start_ln, start_col);
	if (view->window == NULL) {
		view_close(view);
		return NULL;
	}
	view->panel = new_panel(view->window);
	if (view->panel == NULL || set_panel_userptr(view->panel, view) != OK) {
		view_close(view);
		return NULL;
	}

	keypad(view->window, TRUE);
	return view;
}

static int
open_timeline_view(struct fnc_view *view)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	fsl_cx				*f = fcli_cx();
	fsl_db				*db = fsl_cx_db_repo(f);
	fsl_buffer			 sql = fsl_buffer_empty;
	char				*startdate = NULL;
	fsl_id_t			 idtag = 0;
	int				 idx, rc = 0;

	s->thread_cx.q = NULL;
	/* s->selected_idx = 0; */	/* Unnecessary? */

	TAILQ_INIT(&s->commits.head);
	s->commits.ncommits = 0;

	if (fnc_init.start_commit.rid)
		startdate = fsl_mprintf("(SELECT mtime FROM event "
		    "WHERE objid=%d)", fnc_init.start_commit.rid);
	else
		fsl_ckout_version_info(f, NULL, &s->curr_ckout_uuid);

	if ((rc = pthread_cond_init(&s->thread_cx.commit_consumer, NULL))) {
		fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "pthread_cond_init consumer");
		goto end;
	}
	if ((rc = pthread_cond_init(&s->thread_cx.commit_producer, NULL))) {
		fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "pthread_cond_init producer");
		goto end;
	}

	fsl_buffer_appendf(&sql, "SELECT "
            /* 0 */"uuid, "
	    /* 1 */"datetime(event.mtime%s), "
	    /* 2 */"coalesce(euser, user), "
	    /* 3 */"rid AS rid, "
	    /* 4 */"event.type AS eventtype, "
	    /* 5 */"(SELECT group_concat(substr(tagname,5), ',') "
	    "FROM tag, tagxref WHERE tagname GLOB 'sym-*' "
	    "AND tag.tagid=tagxref.tagid AND tagxref.rid=blob.rid "
	    "AND tagxref.tagtype > 0) as tags, "
	    /*6*/"coalesce(ecomment, comment) AS comment FROM event JOIN blob "
	    "WHERE blob.rid=event.objid", fnc_init.utc ? "" : ", 'localtime'");

	if (fnc_init.filter_types->nitems) {
		fsl_buffer_appendf(&sql, " AND (");
		for (idx = 0; idx < fnc_init.filter_types->nitems; ++idx) {
			fsl_buffer_appendf(&sql, " eventtype=%Q%s",
			    fnc_init.filter_types->values[idx], (idx + 1) <
			    fnc_init.filter_types->nitems ? " OR " : ")");
			/* This produces a double-free? */
			/* fsl_free((char *)fnc_init.filter_types->values[idx]); */
			fnc_init.filter_types->values[idx] = NULL;
		}
		fsl_free(fnc_init.filter_types->values);
		fsl_free(fnc_init.filter_types);
	}

	if (fnc_init.filter_branch) {
		idtag = fsl_db_g_id(db, 0,
                    "SELECT tagid FROM tag WHERE tagname='sym-%q'",
                    fnc_init.filter_branch);
		if (idtag > 0)
			fsl_buffer_appendf(&sql,
                             " AND EXISTS(SELECT 1 FROM tagxref"
                             " WHERE tagid=%"FSL_ID_T_PFMT
                             " AND tagtype > 0 AND rid=blob.rid)", idtag);
		else {
			rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
			    "Invalid branch name [%s]",
			    fnc_init.filter_branch);
			goto end;
		}
	}

	if (fnc_init.filter_tag) {
		idtag = fsl_db_g_id(db, 0,
                    "SELECT tagid FROM tag WHERE tagname GLOB 'sym-%q'",
                    fnc_init.filter_tag);
                if (idtag == 0)
			idtag = fsl_db_g_id(db, 0,
			    "SELECT tagid FROM tag WHERE tagname='%q'",
			    fnc_init.filter_tag);
		if (idtag > 0)
			fsl_buffer_appendf(&sql,
                             " AND EXISTS(SELECT 1 FROM tagxref"
                             " WHERE tagid=%"FSL_ID_T_PFMT
                             " AND tagtype > 0 AND rid=blob.rid)", idtag);
		else {
			rc = fsl_cx_err_set(f, FSL_RC_NOT_FOUND,
			    "Invalid tag [%s]", fnc_init.filter_tag);
			goto end;
		}
	}

	if (fnc_init.filter_user)
		if ((rc = fsl_buffer_appendf(&sql,
                    " AND coalesce(euser, user) GLOB lower('*%q*')",
                    fnc_init.filter_user)))
		goto end;

	if (startdate) {
		fsl_buffer_appendf(&sql, " AND event.mtime <= %s", startdate);
		fsl_free(startdate);
	}

	fsl_buffer_appendf(&sql, " ORDER BY event.mtime DESC");

	if (fnc_init.nrecords.limit > 0)
		fsl_buffer_appendf(&sql, " LIMIT %d", fnc_init.nrecords.limit);

	view->show = show_timeline_view;
	view->input = tl_input_handler;
	view->close = close_timeline_view;
	view->search_init = tl_search_init;
	view->search_next = tl_search_next;

	if ((rc = fsl_db_prepare_cached(db, &s->thread_cx.q, "%b", &sql)))
		goto end;
	fsl_stmt_step(s->thread_cx.q);

	s->thread_cx.db = db;
	s->thread_cx.spin_idx = 0;
	s->thread_cx.ncommits_needed = view->nlines - 1;
	s->thread_cx.commits = &s->commits;
	s->thread_cx.timeline_end = false;
	s->thread_cx.quit = &s->quit;
	s->thread_cx.first_commit_onscreen = &s->first_commit_onscreen;
	s->thread_cx.selected_commit = &s->selected_commit;
	s->thread_cx.searching = &view->searching;
	s->thread_cx.search_status = &view->search_status;
	s->thread_cx.regex = &view->regex;

end:
	fsl_buffer_clear(&sql);
	if (rc) {
		close_timeline_view(view);
		if (db->error.code)
			rc = fsl_cx_uplift_db_error(f, db);
	}
	return rc;
}

static int
view_loop(struct fnc_view *view)
{
	struct view_tailhead	 views;
	struct fnc_view		*new_view;
	fsl_cx			*f = fcli_cx();
	int			 done = 0, rc = 0;

	if ((rc = pthread_mutex_lock(&fnc_mutex)))
		return fsl_cx_err_set(f, rc, "mutex lock");

	TAILQ_INIT(&views);
	TAILQ_INSERT_HEAD(&views, view, entries);

	view->active = true;
	rc = view->show(view);
	if (rc)
		return rc;

	while (!TAILQ_EMPTY(&views) && !done && !rec_sigpipe) {
		rc = view_input(&new_view, &done, view, &views);
		if (rc)
			break;
		if (view->egress) {
			struct fnc_view *v, *prev = NULL;

			if (view_is_parent(view))
				prev = TAILQ_PREV(view, view_tailhead, entries);
			else if (view->parent)
				prev = view->parent;

			if (view->parent) {
				view->parent->child = NULL;
				view->parent->focus_child = false;
			} else
				TAILQ_REMOVE(&views, view, entries);

			rc = view_close(view);
			if (rc)
				goto end;

			view = NULL;
			TAILQ_FOREACH(v, &views, entries) {
				if (v->active)
					break;
			}
			if (view == NULL && new_view == NULL) {
				/* No view is active; try to pick one. */
				if (prev)
					view = prev;
				else if (!TAILQ_EMPTY(&views))
					view = TAILQ_LAST(&views,
					    view_tailhead);
				if (view) {
					if (view->focus_child) {
						view->child->active = true;
						view = view->child;
					} else
						view->active = true;
				}
			}
		}
		if (new_view) {
			struct fnc_view *v, *t;
			/* Allow only one parent view per type. */
			TAILQ_FOREACH_SAFE(v, &views, entries, t) {
				if (v->vid != new_view->vid)
					continue;
				TAILQ_REMOVE(&views, v, entries);
				rc = view_close(v);
				if (rc)
					goto end;
				break;
			}
			TAILQ_INSERT_TAIL(&views, new_view, entries);
			view = new_view;
		}
		if (view) {
			if (view_is_parent(view)) {
				if (view->child && view->child->active)
					view = view->child;
			} else {
				if (view->parent && view->parent->active)
					view = view->parent;
			}
			show_panel(view->panel);
			if (view->child && screen_is_split(view->child))
				show_panel(view->child->panel);
			if (view->parent && screen_is_split(view)) {
				rc = view->parent->show(view->parent);
				if (rc)
					goto end;
			}
			rc = view->show(view);
			if (rc)
				goto end;
			if (view->child) {
				rc = view->child->show(view->child);
#ifdef __linux__
				wnoutrefresh(view->child->window);
#endif
				if (rc)
					goto end;
			}
#ifdef __linux__
			wnoutrefresh(view->window);
#else
			update_panels();
#endif
			doupdate();
		}
	}
end:
	while (!TAILQ_EMPTY(&views)) {
		view = TAILQ_FIRST(&views);
		TAILQ_REMOVE(&views, view, entries);
		view_close(view);
	}

	rc = pthread_mutex_unlock(&fnc_mutex);
	if (rc)
		fsl_cx_err_set(f, rc, "mutex unlock");

	return rc;
}

static int
show_timeline_view(struct fnc_view *view)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	int				 rc = 0;

	if (!s->thread_id) {
		rc = pthread_create(&s->thread_id, NULL, tl_producer_thread,
		    &s->thread_cx);
		if (rc)
			return fsl_errno_to_rc(errno, rc);
		if (s->thread_cx.ncommits_needed > 0) {
			rc = signal_tl_thread(view, 1);
			if (rc)
				return rc;
		}
	}

	return draw_commits(view);
}

static void *
tl_producer_thread(void *state)
{
	struct fnc_tl_thread_cx	*cx = state;
	fsl_cx			*f = fcli_cx();
	int			 rc;
	bool			 done = false;

	rc = block_main_thread_signals();
	if (rc)
		return (void *)(intptr_t)rc;

	while (!done && !rc && !rec_sigpipe) {
		switch (rc = build_commits(cx)) {
		case FSL_RC_STEP_DONE:
			done = true;
			/* FALL THROUGH */
		case FSL_RC_STEP_ROW:
			rc = 0;
			/* FALL THROUGH */
		default:
			if (rc)
				return (void *)(intptr_t)rc;
			else if (cx->ncommits_needed > 0)
				cx->ncommits_needed--;
			break;
		}

		if (pthread_mutex_lock(&fnc_mutex)) {
			rc = fsl_cx_err_set(f, FSL_RC_ERROR, "mutex lock fail");
			break;
		} else if (*cx->first_commit_onscreen == NULL) {
			*cx->first_commit_onscreen =
			    TAILQ_FIRST(&cx->commits->head);
			*cx->selected_commit = *cx->first_commit_onscreen;
		} else if (*cx->quit)
			done = true;

		if (pthread_cond_signal(&cx->commit_producer)) {
			rc = fsl_cx_err_set(f, FSL_RC_ERROR,
			    "pthread_cond_signal");
			pthread_mutex_unlock(&fnc_mutex);
			break;
		}

		if (done)
			cx->ncommits_needed = 0;
		else if (cx->ncommits_needed == 0) {
			if (pthread_cond_wait(&cx->commit_consumer, &fnc_mutex))
				rc = fsl_cx_err_set(f, FSL_RC_ERROR,
				    "pthread_cond_wait");
			if (*cx->quit)
				done = true;
		}

		if (pthread_mutex_unlock(&fnc_mutex))
			rc = fsl_cx_err_set(f, FSL_RC_ERROR,
			    "mutex unlock fail");
	}

	cx->timeline_end = true;
	return (void *)(intptr_t)rc;
}

static int
block_main_thread_signals(void)
{
	fsl_cx		*f = NULL;
	sigset_t	 set;

	f = fcli_cx();

	if (sigemptyset(&set) == -1)
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "sigemptyset() fail");

	/* Bespoke signal handlers for SIGWINCH and SIGCONT. */
	if (sigaddset(&set, SIGWINCH) == -1)
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "sigaddset() fail");
	if (sigaddset(&set, SIGCONT) == -1)
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "sigaddset() fail");

	/* ncurses handles SIGTSTP. */
	if (sigaddset(&set, SIGTSTP) == -1)
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "sigaddset() fail");

	if (pthread_sigmask(SIG_BLOCK, &set, NULL))
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "pthread_sigmask() fail");

	return 0;
}

static int
build_commits(struct fnc_tl_thread_cx *cx)
{
	fsl_buffer	 buf = fsl_buffer_empty;
	fsl_id_t	 rid = 0;
	int		 rc = 0;
	fsl_cx		*f = fcli_cx();

	/*
	 * Step through the result set returned from our SQL query,
	 * parsing columns required to build commits for the timeline.
	 */
	do {
		struct commit_entry	*dup_entry;
		const char		*comment, *prefix, *type, *uuid;

		uuid = fsl_stmt_g_text(cx->q, 0, NULL);
		/*
		 * TODO: Find out why, without this, fnc reads and displays
		 * the first (i.e., latest) commit twice. This hack checks to
		 * see if the current row returned a UUID matching the last
		 * commit added to the list to avoid adding a duplicate entry.
		 */
		dup_entry = TAILQ_FIRST(&cx->commits->head);
		if (dup_entry && !fsl_strcmp(dup_entry->commit->uuid, uuid)) {
			cx->ncommits_needed++;
			continue;
		}
		struct commit_entry		*entry;
		struct fnc_commit_artifact	*commit;

		if ((rc = fsl_stmt_get_id(cx->q, 3, &rid)))
			break;

		type = fsl_stmt_g_text(cx->q, 4, NULL);
		/* Should we only build check-in commits for the timeline? */
		/* if (*type != 'c') */
		/*	continue; */

		comment = fsl_stmt_g_text(cx->q, 6, NULL);
		prefix = NULL;

		switch (*type) {
		case 'c':
			type = "checkin";
			break;
		case 'w':
			type = "wiki";
			if (comment)
				switch (*comment) {
				case '+':
					prefix = "Added: ";
					++comment;
					break;
				case '-':
					prefix = "Deleted: ";
					++comment;
					break;
				case ':':
					prefix = "Edited: ";
					++comment;
					break;
				default:
					break;
				}
			break;
		case 'g':
			  type = "tag";
			  break;
		case 'e':
			  type = "technote";
			  break;
		case 't':
			  type = "ticket";
			  break;
		case 'f':
			  type = "forum";
			  break;
		};

		commit = calloc(1, sizeof(*commit));
		if (commit == NULL)
			return fsl_cx_err_set(f, FSL_RC_OOM, "calloc fail");

		fsl_buffer_append(&buf, prefix, -1);
		fsl_buffer_append(&buf, comment, -1);
		commit->comment = fsl_strdup(fsl_buffer_str(&buf));
		fsl_buffer_clear(&buf);

		/* Is there are more efficient way to get the parent? */
		commit->puuid = fsl_db_g_text(cx->db, NULL,
		    "SELECT uuid FROM plink, blob WHERE plink.cid=%d "
		    "AND blob.rid=plink.pid AND plink.isprim", rid);
		commit->uuid = fsl_strdup(uuid);
		commit->timestamp = fsl_strdup(fsl_stmt_g_text(cx->q, 1, NULL));
		commit->user = fsl_strdup(fsl_stmt_g_text(cx->q, 2, NULL));
		commit->branch = fsl_strdup(fsl_stmt_g_text(cx->q, 5, NULL));
		commit->type = fsl_strdup(type);
		commit->rid = rid;

		entry = fsl_malloc(sizeof(*entry));
		if (entry == NULL)
			return fsl_cx_err_set(f, FSL_RC_OOM, "malloc fail");

		entry->commit = commit;
		fsl_stmt_cached_yield(cx->q);

		if (pthread_mutex_lock(&fnc_mutex))
			return fsl_cx_err_set(f, FSL_RC_ERROR, "mutex lock");

		entry->idx = cx->commits->ncommits;
		TAILQ_INSERT_TAIL(&cx->commits->head, entry, entries);
		cx->commits->ncommits++;

		if (!cx->endjmp && *cx->searching == SEARCH_FORWARD &&
		    *cx->search_status == SEARCH_WAITING) {
			if (find_commit_match(commit, cx->regex))
				*cx->search_status = SEARCH_CONTINUE;
		}

		if (pthread_mutex_unlock(&fnc_mutex))
			return fsl_cx_err_set(f, FSL_RC_ERROR, "mutex unlock");

	} while ((rc = fsl_stmt_step(cx->q)) == FSL_RC_STEP_ROW
	    && *cx->searching == SEARCH_FORWARD
	    && *cx->search_status == SEARCH_WAITING);

	return rc;
}

static int
signal_tl_thread(struct fnc_view *view, int wait)
{
	struct fnc_tl_thread_cx	*ta = &view->state.timeline.thread_cx;
	fsl_cx			*f = fcli_cx();
	int			 rc = 0;

	while (ta->ncommits_needed > 0) {
		if (ta->timeline_end)
			break;

		/* Wake timeline thread. */
		if ((rc = pthread_cond_signal(&ta->commit_consumer)))
			return fsl_cx_err_set(f, fsl_errno_to_rc(errno,
			    FSL_RC_ERROR), "pthread_cond_signal consumer");

		/*
		 * Mutex will be released while view_loop().view_input() waits
		 * in wgetch(), at which point the timeline thread will run.
		 */
		if (!wait)
			break;

		/* Show status update in timeline view. */
		show_timeline_view(view);
		update_panels();
		doupdate();

		/* Wait while the next commit is being loaded. */
		if ((rc = pthread_cond_wait(&ta->commit_producer, &fnc_mutex)))
			return fsl_cx_err_set(f, fsl_errno_to_rc(errno,
			    FSL_RC_ERROR), "pthread_cond_signal producer");

		/* Show status update in timeline view. */
		show_timeline_view(view);
		update_panels();
		doupdate();
	}

	return rc;
}

static int
draw_commits(struct fnc_view *view)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct fnc_tl_thread_cx		*tcx = &s->thread_cx;
	struct commit_entry		*entry = s->selected_commit;
	fsl_cx				*f = fcli_cx();
	char				*headln = NULL, *idxstr = NULL;
	char				*branch = NULL, *type = NULL;
	char				*uuid = NULL;
	wchar_t				*wcstr;
	int				 ncommits = 0, rc = 0, wstrlen = 0;
	int				 max_usrlen = -1;

	if (s->selected_commit && !(view->searching != SEARCH_DONE &&
	    view->search_status == SEARCH_WAITING)) {
		uuid = fsl_strdup(s->selected_commit->commit->uuid);
		branch = fsl_strdup(s->selected_commit->commit->branch);
		type = fsl_strdup(s->selected_commit->commit->type);
	}

	if (s->thread_cx.ncommits_needed > 0) {
		if ((idxstr = fsl_mprintf(" [%d/%d] %s",
		    entry ? entry->idx + 1 : 0, s->commits.ncommits,
		    (view->searching && !view->search_status) ?
		    "searching..." : "loading...")) == NULL) {
			rc = fsl_cx_err_set(f, FSL_RC_RANGE, "mprintf idx");
			goto end;
		}
	} else {
		const char	*search_str = NULL;

		if (view->searching) {
			if (view->search_status == SEARCH_COMPLETE)
				search_str = "no more matches";
			else if (view->search_status == SEARCH_NO_MATCH)
				search_str = "no matches found";
			else if (view->search_status == SEARCH_WAITING)
				search_str = "searching...";
		}

		if ((idxstr = fsl_mprintf(" [%d/%d] %s",
		    entry ? entry->idx + 1 : 0, s->commits.ncommits,
		    search_str ? search_str :
		    (branch ? branch : ""))) == NULL) {
			rc = fsl_cx_err_set(f, FSL_RC_RANGE, "mprintf idx");
			goto end;
		}
	}
	if ((headln = fsl_mprintf("%s%c%s%s%s", type ? type : "", type ?
	    ' ' : SPINNER[tcx->spin_idx], uuid ? uuid :
	    "........................................", !fsl_strcmp(uuid,
	    s->curr_ckout_uuid) ? " [current]" : "", idxstr)) == NULL) {
		rc = fsl_cx_err_set(f, FSL_RC_RANGE, "mprintf headln");
		headln = NULL;
		goto end;
	}
	if (SPINNER[++tcx->spin_idx] == '\0')
		tcx->spin_idx = 0;
	rc = formatln(&wcstr, &wstrlen, headln, view->ncols, 0);
	if (rc)
		goto end;

	werase(view->window);

	if (screen_is_shared(view))
		wstandout(view->window);
	waddwstr(view->window, wcstr);
	while (wstrlen < view->ncols) {
		waddch(view->window, ' ');
		++wstrlen;
	}
	if (screen_is_shared(view))
		wstandend(view->window);
	fsl_free(wcstr);
	if (view->nlines <= 1)
		goto end;

	/* Parse commits to be written on screen for the longest username. */
	entry = s->first_commit_onscreen;
	while (entry) {
		wchar_t		*usr_wcstr;
		char		*user;
		int		 usrlen;
		if (ncommits >= view->nlines - 1)
			break;
		user = fsl_strdup(entry->commit->user);
		if (user == NULL) {
			rc = fsl_errno_to_rc(errno, FSL_RC_OOM);
			return rc;
		}
		if (strpbrk(user, "<@>") != NULL)
			parse_emailaddr_username(&user);
		rc = formatln(&usr_wcstr, &usrlen, user, view->ncols, 0);
		if (max_usrlen < usrlen)
			max_usrlen = usrlen;
		fsl_free(usr_wcstr);
		fsl_free(user);
		++ncommits;
		entry = TAILQ_NEXT(entry, entries);
	}

	ncommits = 0;
	entry = s->first_commit_onscreen;
	s->last_commit_onscreen = s->first_commit_onscreen;
	while (entry) {
		if (ncommits >= view->nlines - 1)
			break;
		if (ncommits == s->selected_idx)
			wstandout(view->window);
		rc = write_commit_line(view, entry->commit, max_usrlen);
		if (ncommits == s->selected_idx)
			wstandend(view->window);
		++ncommits;
		s->last_commit_onscreen = entry;
		entry = TAILQ_NEXT(entry, entries);
	}
	draw_vborder(view);

end:
	free(branch);
	free(type);
	free(uuid);
	free(idxstr);
	free(headln);
	return rc;
}

static void
parse_emailaddr_username(char **username)
{
	char	*lt, *usr;

	lt = strchr(*username, '<');
	if (lt && lt[1] != '\0') {
		usr = fsl_strdup(++lt);
		fsl_free(*username);
	} else
		usr = *username;
	usr[strcspn(usr, "@>")] = '\0';

	*username = usr;
}

static int
formatln(wchar_t **ptr, int *wstrlen, const char *mbstr, int column_limit,
    int start_column)
{
	fsl_cx		*f = fcli_cx();
	wchar_t		*wline = NULL;
	size_t		 i, wlen;
	int		 rc = 0, cols = 0;

	*ptr = NULL;
	*wstrlen = 0;

	rc = multibyte_to_wchar(mbstr, &wline, &wlen);
	if (rc)
		return rc;

	if (wlen > 0 && wline[wlen - 1] == L'\n') {
		wline[wlen - 1] = L'\0';
		wlen--;
	}
	if (wlen > 0 && wline[wlen - 1] == L'\r') {
		wline[wlen - 1] = L'\0';
		wlen--;
	}

	i = 0;
	while (i < wlen) {
		int width = wcwidth(wline[i]);

		if (width == 0) {
			i++;
			continue;
		}

		if (width == 1 || width == 2) {
			if (cols + width > column_limit)
				break;
			cols += width;
			i++;
		} else if (width == -1) {
			if (wline[i] == L'\t') {
				width = TABSIZE -
				((cols + column_limit) % TABSIZE);
			} else {
				width = 1;
				wline[i] = L'.';
			}
			if (cols + width > column_limit)
				break;
			cols += width;
			i++;
		} else {
			rc = fsl_cx_err_set(f, FSL_RC_RANGE, "wcwidth");
			goto end;
		}
	}
	wline[i] = L'\0';
	if (wstrlen)
		*wstrlen = cols;
end:
	if (rc)
		free(wline);
	else
		*ptr = wline;
	return rc;
}

static int
multibyte_to_wchar(const char *src, wchar_t **dst, size_t *dstlen)
{
	fsl_cx	*f = fcli_cx();

	/*
	 * mbstowcs POSIX extension specifies that the number of wchar that
	 * would be written are returned when first arg is a null pointer:
	 * https://en.cppreference.com/w/cpp/string/multibyte/mbstowcs
	 */
	*dstlen = mbstowcs(NULL, src, 0);
	if (*dstlen < 0)
		return fsl_cx_err_set(f, FSL_RC_MISUSE,
		    "invalid multibyte character");

	*dst = NULL;
	*dst = fsl_malloc(sizeof(wchar_t) * (*dstlen + 1));
	if (*dst == NULL)
		return fsl_cx_err_set(f, FSL_RC_OOM, "malloc");

	if (mbstowcs(*dst, src, *dstlen) != *dstlen)
		return fsl_cx_err_set(f, FSL_RC_RANGE, "mbstowcs mismatch");

	return 0;
}

/*
 * When the terminal is >= 110 columns wide, the commit summary line in the
 * timeline view will take the form:
 *
 *   DATE UUID USERNAME  COMMIT-COMMENT
 *
 * Assuming an 8-character username, this scheme provides 80 characters for the
 * comment, which should be sufficient considering it's suggested good practice
 * to limit commit comment summary lines to a maximum 50 characters, and most
 * plaintext-based conventions suggest not exceeding 72-80 characters.
 *
 * When < 120 columns, the (abbreviated 9-character) UUID will be elided.
 */
static int
write_commit_line(struct fnc_view *view, struct fnc_commit_artifact *commit,
    int max_usrlen)
{
	fsl_cx		*f = fcli_cx();
	wchar_t		*usr_wcstr = NULL, *wcomment = NULL;
	char		*comment0 = NULL, *comment = NULL, *date = NULL;
	char		*eol = NULL, *pad = NULL, *user = NULL;
	size_t		 i = 0;
	int		 col_pos, ncols_avail, usrlen, commentlen, rc = 0;

	/* Trim time component from timestamp for the date field. */
	date = fsl_strdup(commit->timestamp);
	while (!fsl_isspace(date[i++])) {}
	date[i] = '\0';
	col_pos = MIN(view->ncols, ISO8601_DATE_ONLY + 1);
	waddnstr(view->window, date, col_pos);
	if (col_pos > view->ncols)
		goto end;

	/* If enough columns, write abbreviated commit hash. */
	if (view->ncols >= 110) {
		wprintw(view->window, "%.9s ", commit->uuid);
		col_pos += 10;
		if (col_pos > view->ncols)
			goto end;
	}

	/*
	 * Parse username from emailaddr if needed, and postfix username
	 * with as much whitespace as needed to fill two spaces beyond
	 * the longest username on the screen.
	 */
	user = fsl_strdup(commit->user);
	if (user == NULL)
		goto end;
	if (strpbrk(user, "<@>") != NULL)
		parse_emailaddr_username(&user);
	rc = formatln(&usr_wcstr, &usrlen, user, view->ncols - col_pos,
	    col_pos);
	if (rc)
		goto end;
	waddwstr(view->window, usr_wcstr);
	pad = fsl_mprintf("%*s",  max_usrlen - usrlen + 2, " ");
	waddstr(view->window, pad);
	col_pos += (max_usrlen + 2);
	if (col_pos > view->ncols)
		goto end;

	/* Only show comment up to the first newline character. */
	comment0 = strdup(commit->comment);
	comment = comment0;
	if (comment == NULL)
		return fsl_cx_err_set(f, FSL_RC_OOM, "strdup");
	while (*comment == '\n')
		++comment;
	eol = strchr(comment, '\n');
	if (eol)
		*eol = '\0';
	ncols_avail = view->ncols - col_pos;
	rc = formatln(&wcomment, &commentlen, comment, ncols_avail, col_pos);
	if (rc)
		goto end;
	waddwstr(view->window, wcomment);
	col_pos += commentlen;
	while (col_pos < view->ncols) {
		waddch(view->window, ' ');
		++col_pos;
	}
end:
	fsl_free(date);
	fsl_free(user);
	fsl_free(usr_wcstr);
	fsl_free(pad);
	fsl_free(comment0);
	fsl_free(wcomment);
	return rc;
}

static int
view_input(struct fnc_view **new, int *done, struct fnc_view *view,
    struct view_tailhead *views)
{
	struct fnc_view	*v;
	fsl_cx		*f = fcli_cx();
	int		 ch, rc = 0;

	*new = NULL;

	/* Clear search indicator string. */
	if (view->search_status == SEARCH_COMPLETE ||
	    view->search_status == SEARCH_NO_MATCH)
		view->search_status = SEARCH_CONTINUE;

	if (view->searching && view->search_status == SEARCH_WAITING) {
		if ((rc = pthread_mutex_unlock(&fnc_mutex)))
			return fsl_cx_err_set(f, rc, "mutex unlock");
		sched_yield();
		if ((rc = pthread_mutex_lock(&fnc_mutex)))
			return fsl_cx_err_set(f, rc, "mutex lock");
		view->search_next(view);
		return rc;
	}

	nodelay(stdscr, FALSE);
	/* Allow thread to make progress while waiting for input. */
	if ((rc = pthread_mutex_unlock(&fnc_mutex)))
		return fsl_cx_err_set(f, rc, "mutex unlock");
	ch = wgetch(view->window);
	if ((rc = pthread_mutex_lock(&fnc_mutex)))
		return fsl_cx_err_set(f, rc, "mutex lock");

	if (rec_sigwinch || rec_sigcont) {
		fnc_resizeterm();
		rec_sigwinch = 0;
		rec_sigcont = 0;
		TAILQ_FOREACH(v, views, entries) {
			if ((rc = view_resize(v)))
				return rc;
			if ((rc = v->input(new, v, KEY_RESIZE)))
				return rc;
			if (v->child) {
				if ((rc = view_resize(v->child)))
					return rc;
				rc = v->child->input(new, v->child, KEY_RESIZE);
				if (rc)
					return rc;
			}
		}
	}

	switch (ch) {
	case '\t':
		if (view->child) {
			view->active = false;
			view->child->active = true;
			view->focus_child = true;
		} else if (view->parent) {
			view->active = false;
			view->parent->active = true;
			view->parent->focus_child = false;
		}
		break;
	case 'q':
		rc = view->input(new, view, ch);
		view->egress = true;
		break;
	case 'f':
		if (view_is_parent(view)) {
			if (view->child == NULL)
				break;
			if (screen_is_split(view->child)) {
				view->active = false;
				view->child->active = true;
				rc = make_fullscreen(view->child);
			} else
				rc = make_splitscreen(view->child);
			if (rc)
				break;
			rc = view->child->input(new, view->child, KEY_RESIZE);
		} else {
			if (screen_is_split(view)) {
				view->parent->active = false;
				view->active = true;
				rc = make_fullscreen(view);
			} else
				rc = make_splitscreen(view);
			if (rc)
				break;
			rc = view->input(new, view, KEY_RESIZE);
		}
		break;
	case '/':
		if (view->search_init)
			view_search_start(view);
		else
			rc = view->input(new, view, ch);
		break;
	case 'N':
	case 'n':
		if (view->started_search && view->search_next) {
			view->searching = (ch == 'n' ?
			    SEARCH_FORWARD : SEARCH_REVERSE);
			view->search_status = SEARCH_WAITING;
			view->search_next(view);
		} else
			rc = view->input(new, view, ch);
		break;
	case KEY_RESIZE:
		break;
	case ERR:
		break;
	case 'Q':
		*done = 1;
		break;
	default:
		rc = view->input(new, view, ch);
		break;
	}

	return rc;
}

static int
tl_input_handler(struct fnc_view **new_view, struct fnc_view *view, int ch)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct fnc_view			*diff_view = NULL;
	int				 rc = 0, start_col = 0;

	switch (ch) {
	case KEY_DOWN:
	case 'j':
	case '.':
	case '>':
		if (s->first_commit_onscreen == NULL)
			break;
		if (s->selected_idx < MIN(view->nlines - 2,
		    s->commits.ncommits - 1))
			++s->selected_idx;
		else if ((rc = timeline_scroll_down(view, 1)))
			break;
		select_commit(s);
		break;
	case KEY_NPAGE:
	case CTRL('f'): {
		struct commit_entry *first;
		if ((first = s->first_commit_onscreen) == NULL)
			break;
		if ((rc = timeline_scroll_down(view, view->nlines - 1)))
			break;
		if (first == s->first_commit_onscreen && s->selected_idx <
		    MIN(view->nlines - 2, s->commits.ncommits - 1))
			/* At bottom of timeline. */
			s->selected_idx = MIN(view->nlines - 2,
			    s->commits.ncommits - 1);
		select_commit(s);
		break;
	}
	case KEY_END:
	case 'G':
		view->search_status = SEARCH_FOR_END;
		view_search_start(view);
		break;
	case 'k':
	case KEY_UP:
	case '<':
	case ',':
		if (s->first_commit_onscreen == NULL)
			break;
		if (s->selected_idx > 0)
			--s->selected_idx;
		else
			timeline_scroll_up(s, 1);
		select_commit(s);
		break;
	case KEY_PPAGE:
	case CTRL('b'):
		if (s->first_commit_onscreen == NULL)
			break;
		if (TAILQ_FIRST(&s->commits.head) == s->first_commit_onscreen)
			s->selected_idx = 0;
		else
			timeline_scroll_up(s, view->nlines - 1);
		select_commit(s);
		break;
	case 'g': {
		bool home = true;
		halfdelay(10);	/* Block for 1 second, then return ERR. */
		if (wgetch(view->window) != 'g')
			home = false;
		cbreak();	/* Return to blocking mode on user input. */
		if (!home)
			break;
		/* FALL THROUGH */
	}
	case KEY_HOME:
		if (s->first_commit_onscreen == NULL)
			break;
		s->selected_idx = 0;
		timeline_scroll_up(s, s->commits.ncommits);
		select_commit(s);
		break;
	case KEY_RESIZE:
		if (s->selected_idx > view->nlines - 2)
			s->selected_idx = view->nlines - 2;
		if (s->selected_idx > s->commits.ncommits - 1)
			s->selected_idx = s->commits.ncommits - 1;
		select_commit(s);
		if (s->commits.ncommits < view->nlines - 1 &&
		    !s->thread_cx.timeline_end) {
			s->thread_cx.ncommits_needed += (view->nlines - 1) -
			    s->commits.ncommits;
			rc = signal_tl_thread(view, 1);
		}
		break;
	case KEY_ENTER:
	case ' ':
	case '\r':
		if (s->selected_commit == NULL)
			break;
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		if ((rc = init_diff_commit(&diff_view, start_col,
		    s->selected_commit->commit, view)))
			break;
		view->active = false;
		diff_view->active = true;
		if (view_is_parent(view)) {
			if (view->child != NULL) {
				rc = view_close(view->child);
				view->child = NULL;
			}
			if (rc)
				return rc;
			view->child = diff_view;
			diff_view->parent = view;
			view->focus_child = true;
		} else
			*new_view = diff_view;
		break;
	case 'q':
		s->quit = 1;
		break;
	default:
		break;
	}

	return rc;
}

static int
timeline_scroll_down(struct fnc_view *view, int maxscroll)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct commit_entry		*pentry;
	int				 rc = 0, nscrolled = 0, ncommits_needed;

	if (s->last_commit_onscreen == NULL)
		return 0;

	ncommits_needed = s->last_commit_onscreen->idx + 1 + maxscroll;
	if (s->commits.ncommits < ncommits_needed &&
	    !s->thread_cx.timeline_end) {
		/* Signal timeline thread for n commits needed. */
		s->thread_cx.ncommits_needed += maxscroll;
		rc = signal_tl_thread(view, 1);
		if (rc)
			return rc;
	}

	do {
		pentry = TAILQ_NEXT(s->last_commit_onscreen, entries);
		if (pentry == NULL)
			break;

		s->last_commit_onscreen = pentry;

		pentry = TAILQ_NEXT(s->first_commit_onscreen, entries);
		if (pentry == NULL)
			break;
		s->first_commit_onscreen = pentry;
	} while (++nscrolled < maxscroll);

	return rc;
}

static void
timeline_scroll_up(struct fnc_tl_view_state *s, int maxscroll)
{
	struct commit_entry	*entry;
	int			 nscrolled = 0;

	entry = TAILQ_FIRST(&s->commits.head);
	if (s->first_commit_onscreen == entry)
		return;

	entry = s->first_commit_onscreen;
	while (entry && nscrolled < maxscroll) {
		entry = TAILQ_PREV(entry, commit_tailhead, entries);
		if (entry) {
			s->first_commit_onscreen = entry;
			++nscrolled;
		}
	}
}

static void
select_commit(struct fnc_tl_view_state *s)
{
	struct commit_entry	*entry;
	int			 ncommits = 0;

	entry = s->first_commit_onscreen;
	while (entry) {
		if (ncommits == s->selected_idx) {
			s->selected_commit = entry;
			break;
		}
		entry = TAILQ_NEXT(entry, entries);
		++ncommits;
	}
}

static int
make_splitscreen(struct fnc_view *view)
{
	fsl_cx	*f = fcli_cx();
	int	 rc = 0;

	view->start_ln = 0;
	view->start_col = view_split_start_col(0);
	view->nlines = LINES;
	view->ncols = COLS - view->start_col;
	view->lines = LINES;
	view->cols = COLS;
	if ((rc = view_resize(view)))
		return rc;

	if (mvwin(view->window, view->start_ln, view->start_col) == ERR)
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "curses mvwin -> %s", __func__);

	return rc;
}

static int
make_fullscreen(struct fnc_view *view)
{
	fsl_cx	*f = fcli_cx();
	int	 rc = 0;

	view->start_col = 0;
	view->start_ln = 0;
	view->nlines = LINES;
	view->ncols = COLS;
	view->lines = LINES;
	view->cols = COLS;
	if ((rc = view_resize(view)))
		return rc;

	if (mvwin(view->window, view->start_ln, view->start_col) == ERR)
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "curses mvwin -> %s", __func__);

	return rc;
}

/*
 * Only open a new view in a splitscreen if the console is >= 120 columns wide.
 * Otherwise, open in the current view. If splitting the screen, make the new
 * panel the largest of 80 columns or half the current column width.
 */
static int
view_split_start_col(int start_col)
{
	if (start_col > 0 || COLS < 120)
		return 0;
	return (COLS - MAX(COLS / 2, 80));
}

static int
view_search_start(struct fnc_view *view)
{
	char	pattern[BUFSIZ];
	int	retval;
	int	rc = 0;

	if (view->started_search) {
		regfree(&view->regex);
		view->searching = SEARCH_DONE;
		memset(&view->regmatch, 0, sizeof(view->regmatch));
	}
	view->started_search = false;

	if (view->nlines < 1)
		return rc;

	if (view->search_status == SEARCH_FOR_END) {
		view->search_init(view);
		view->started_search = true;
		view->searching = SEARCH_FORWARD;
		view->search_status = SEARCH_WAITING;
		view->state.timeline.thread_cx.endjmp = true;
		view->search_next(view);

		return rc;
	}

	mvwaddstr(view->window, view->start_ln + view->nlines - 1, 0, "/");
	wclrtoeol(view->window);

	nocbreak();
	echo();
	retval = wgetnstr(view->window, pattern, sizeof(pattern));
	cbreak();
	noecho();
	if (retval == ERR)
		return rc;

	if (regcomp(&view->regex, pattern, REG_EXTENDED | REG_NEWLINE) == 0) {
		if ((rc = view->search_init(view))) {
			regfree(&view->regex);
			return rc;
		}
		view->started_search = true;
		view->searching = SEARCH_FORWARD;
		view->search_status = SEARCH_WAITING;
		view->search_next(view);
	}

	return rc;
}

static int
tl_search_init(struct fnc_view *view)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;

	s->matched_commit = NULL;
	s->search_commit = NULL;
	return 0;
}

static int
tl_search_next(struct fnc_view *view)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct commit_entry		*entry;
	fsl_cx				*f = fcli_cx();
	int				 rc = 0;

	if (!s->thread_cx.ncommits_needed && view->started_search)
		halfdelay(1);

	/* Show status update in timeline view. */
	show_timeline_view(view);
	update_panels();
	doupdate();

	if (s->search_commit) {
		int	ch;
		if ((rc = pthread_mutex_unlock(&fnc_mutex)))
			return fsl_cx_err_set(f, rc, "mutex unlock fail");
		ch = wgetch(view->window);
		if ((rc = pthread_mutex_lock(&fnc_mutex)))
			return fsl_cx_err_set(f, rc, "mutex lock fail");
		if (ch == KEY_BACKSPACE) {
			view->search_status = SEARCH_CONTINUE;
			return rc;
		}
		if (view->searching == SEARCH_FORWARD)
			entry = TAILQ_NEXT(s->search_commit, entries);
		else
			entry = TAILQ_PREV(s->search_commit, commit_tailhead,
			    entries);
	} else if (s->matched_commit) {
		if (view->searching == SEARCH_FORWARD)
			entry = TAILQ_NEXT(s->matched_commit, entries);
		else
			entry = TAILQ_PREV(s->matched_commit, commit_tailhead,
			    entries);
	} else {
		if (view->searching == SEARCH_FORWARD)
			entry = TAILQ_FIRST(&s->commits.head);
		else
			entry = TAILQ_LAST(&s->commits.head, commit_tailhead);
	}

	while (1) {
		if (entry == NULL) {
			if (s->thread_cx.timeline_end && s->thread_cx.endjmp) {
				s->matched_commit = TAILQ_LAST(&s->commits.head,
				    commit_tailhead);
				view->search_status = SEARCH_COMPLETE;
				s->thread_cx.endjmp = false;
				break;
			}
			if (s->thread_cx.timeline_end || view->searching ==
			    SEARCH_REVERSE) {
				view->search_status = (s->matched_commit ==
				    NULL ?  SEARCH_NO_MATCH : SEARCH_COMPLETE);
				s->search_commit = NULL;
				cbreak();
				return rc;
			}
			/*
			 * Wake the timeline thread to produce more commits.
			 * Search will resume at s->search_commit upon return.
			 */
			++s->thread_cx.ncommits_needed;
			return signal_tl_thread(view, 0);
		}

		if (!s->thread_cx.endjmp && find_commit_match(entry->commit,
		    &view->regex)) {
			view->search_status = SEARCH_CONTINUE;
			s->matched_commit = entry;
			break;
		}

		s->search_commit = entry;
		if (view->searching == SEARCH_FORWARD)
			entry = TAILQ_NEXT(entry, entries);
		else
			entry = TAILQ_PREV(entry, commit_tailhead, entries);
	}

	if (s->matched_commit) {
		int cur = s->selected_commit->idx;
		while (cur < s->matched_commit->idx) {
			if ((rc = tl_input_handler(NULL, view, KEY_DOWN)))
				return rc;
			++cur;
		}
		while (cur > s->matched_commit->idx) {
			if ((rc = tl_input_handler(NULL, view, KEY_UP)))
				return rc;
			--cur;
		}
	}

	s->search_commit = NULL;
	cbreak();

	return rc;
}

static bool
find_commit_match(struct fnc_commit_artifact *commit,
regex_t *regex)
{
	regmatch_t	regmatch;

	if (regexec(regex, commit->user, 1, &regmatch, 0) == 0 ||
	    regexec(regex, (char *)commit->uuid, 1, &regmatch, 0) == 0 ||
	    regexec(regex, commit->comment, 1, &regmatch, 0) == 0 ||
	    (commit->branch && regexec(regex, commit->branch, 1, &regmatch, 0)
	     == 0))
		return true;

	return false;
}

static int
view_close(struct fnc_view *view)
{
        int	rc = 0;

	if (view->child) {
		view_close(view->child);
		view->child = NULL;
	}
	if (view->close)
		rc = view->close(view);
	if (view->panel)
		del_panel(view->panel);
	if (view->window)
		delwin(view->window);
	free(view);

	return rc;
}

static int
close_timeline_view(struct fnc_view *view)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	int				 rc = 0;

	rc = join_tl_thread(s);
	fnc_free_commits(&s->commits);
	regfree(&view->regex);

	return rc;
}

/* static void */
/* sspinner(void) */
/* { */
/* 	int idx; */

/* 	while (1) { */
/* 		for (idx = 0; idx < 4; ++idx) { */
/* 			printf("\b%c", "|/-\\"[idx]); */
/* 			fflush(stdout); */
/* 			ssleep(SPIN_INTERVAL); */
/* 		} */
/* 	} */
/* } */

static int
join_tl_thread(struct fnc_tl_view_state *s)
{
	void	*err;
	fsl_cx	*f = fcli_cx();
	int	 rc = 0;

	if (s->thread_id) {
		s->quit = 1;

		if ((rc = pthread_cond_signal(&s->thread_cx.commit_consumer)))
			return fsl_cx_err_set(f, rc, "pthread_cond_signal");
		if ((rc = pthread_mutex_unlock(&fnc_mutex)))
			return fsl_cx_err_set(f, rc, "mutex unlock fail");
		if ((rc = pthread_join(s->thread_id, &err)) ||
		    err == PTHREAD_CANCELED)
			return fsl_cx_err_set(f, rc ? rc : (intptr_t)err,
			    "pthread_join");
		if ((rc = pthread_mutex_lock(&fnc_mutex)))
			return fsl_cx_err_set(f, rc, "mutex lock fail");

		s->thread_id = 0;
	}

	if ((rc = pthread_cond_destroy(&s->thread_cx.commit_consumer)))
		fsl_cx_err_set(f, rc, "pthread_cond_destroy consumer");

	if ((rc = pthread_cond_destroy(&s->thread_cx.commit_producer)))
		fsl_cx_err_set(f, rc, "pthread_cond_destroy producer");

	return rc;
}

static void
fnc_free_commits(struct commit_queue *commits)
{
	while (!TAILQ_EMPTY(&commits->head)) {
		struct commit_entry	*entry;

		entry = TAILQ_FIRST(&commits->head);
		TAILQ_REMOVE(&commits->head, entry, entries);
		fnc_commit_artifact_close(entry->commit);
		free(entry);
		--commits->ncommits;
	}
}

static void
fnc_commit_artifact_close(struct fnc_commit_artifact *commit)
{
	struct fsl_list_state	st = { FNC_ARTIFACT_OBJ };

	if (commit->branch)
		fsl_free(commit->branch);
	if (commit->comment)
		fsl_free(commit->comment);
	if (commit->timestamp)
		fsl_free(commit->timestamp);
	if (commit->type)
		fsl_free(commit->type);
	if (commit->user)
		fsl_free(commit->user);
	fsl_free(commit->uuid);
	fsl_free(commit->puuid);
	fsl_list_clear(&commit->changeset, fsl_list_object_free, &st);
	fsl_list_reserve(&commit->changeset, 0);
	fsl_free(commit);
}

static int
fsl_list_object_free(void *elem, void *state)
{
	struct fsl_list_state *st = state;

	switch (st->obj) {
	case FNC_ARTIFACT_OBJ: {
		struct fsl_file_artifact *ffa = elem;
		if (ffa->fc)
			fsl_free(ffa->fc);
		if (ffa)
			fsl_free(ffa);
		break;
	}
	case FNC_COLOUR_OBJ: {
		struct fnc_colour *c = elem;
		regfree(&c->regex);
		fsl_free(c);
		break;
	}
	default:
		return fcli_err_set(FSL_RC_MISSING_INFO,
		    "fsl_list_state.obj missing or invalid: %d", st->obj);
	}

	return 0;
}

static int
init_diff_commit(struct fnc_view **new_view, int start_col,
    struct fnc_commit_artifact *commit, struct fnc_view *timeline_view)
{
	struct fnc_view			*diff_view;
	fsl_cx				*f = fcli_cx();
	int				 rc = 0;

	diff_view = view_open(0, 0, 0, start_col, FNC_VIEW_DIFF);
	if (diff_view == NULL)
		return fsl_cx_err_set(f, FSL_RC_OOM, "new_view");

	rc = open_diff_view(diff_view, commit, 5, fnc_init.ws,
	    fnc_init.invert, !fnc_init.quiet, timeline_view);
	if (!rc)
		*new_view = diff_view;

	return rc;
}

static int
open_diff_view(struct fnc_view *view, struct fnc_commit_artifact *commit,
    int context, bool ignore_ws, bool invert, bool verbosity,
    struct fnc_view *timeline_view)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
	int				 rc = 0;

	s->selected_commit = commit;
	s->first_line_onscreen = 1;
	s->last_line_onscreen = view->nlines;
	s->current_line = 1;
	s->f = NULL;
	s->context = context;
	s->sbs = 0;
	verbosity ? s->diff_flags |= FSL_DIFF_VERBOSE : 0;
	ignore_ws ? s->diff_flags |= FSL_DIFF_IGNORE_ALLWS : 0;
	invert ? s->diff_flags |= FSL_DIFF_INVERT : 0;
	s->timeline_view = timeline_view;
	s->colours = fsl_list_empty;
	s->colour = !fnc_init.nocolour;

	if (s->colour && has_colors())
		set_diff_colours(&s->colours);
	if (timeline_view && screen_is_split(view))
		show_timeline_view(timeline_view); /* draw vborder */
	show_diff_status(view);

	s->line_offsets = NULL;
	s->nlines = 0;
	s->ncols = view->ncols;
	rc = create_diff(s);
	if (rc)
		return rc;

	view->show = show_diff;
	view->input = diff_input_handler;
	view->close = close_diff_view;
	view->search_init = diff_search_init;
	view->search_next = diff_search_next;

	return rc;
}

static int
set_diff_colours(fsl_list *s)
{
	struct fnc_colour	*colour;
	fsl_cx			*f = fcli_cx();
	fsl_size_t		 idx;
	const char		*regexp[4] = {
				    "^((checkin|wiki|ticket|technote) [0-9a-f]|"
				    "hash [+-] |\\[[+~>-]] |[+-]{3} )",
				    "^-", "^\\+", "^@@"
				 };
	int			 pairs[4][2] = {
				    {FNC_DIFF_META, COLOR_GREEN},
				    {FNC_DIFF_MINUS, COLOR_MAGENTA},
				    {FNC_DIFF_PLUS, COLOR_CYAN},
				    {FNC_DIFF_CHNK, COLOR_YELLOW}
				 };
	int			 rc = 0;

	for (idx = 0; idx < nitems(regexp); ++idx) {
		colour = fsl_malloc(sizeof(*colour));
		if (colour == NULL)
			return fsl_cx_err_set(f, FSL_RC_OOM, "malloc");
		rc = regcomp(&colour->regex, regexp[idx],
		    REG_EXTENDED | REG_NEWLINE | REG_NOSUB);
		if (rc) {
			static char regerr[512];
			regerror(rc, &colour->regex, regerr, sizeof(regerr));
			free(colour);
			return fsl_cx_err_set(f, fsl_errno_to_rc(rc,
			    FSL_RC_ERROR), "regcomp: %s [%s]", regerr,
			    regexp[idx]);
		}
		colour->scheme = pairs[idx][0];
		init_pair(colour->scheme, pairs[idx][1], -1);
		fsl_list_append(s, colour);
	}

	return rc;
}

static void
show_diff_status(struct fnc_view *view)
{
	mvwaddstr(view->window, 0, 0, "generating diff...");
#ifdef __linux__
	wnoutrefresh(view->window);
#else
	update_panels();
#endif
	doupdate();
}

static int
create_diff(struct fnc_diff_view_state *s)
{
	fsl_cx	*f = fcli_cx();
	FILE	*fout = NULL;
	char	*line, *st0 = NULL, *st = NULL;
	off_t	 lnoff = 0;
	int	 n, rc = 0;

	free(s->line_offsets);
	s->line_offsets = fsl_malloc(sizeof(off_t));
	if (s->line_offsets == NULL)
		return fsl_cx_err_set(f, FSL_RC_OOM, "fsl_malloc");
	s->nlines = 0;

	fout = tmpfile();
	if (fout == NULL) {
		rc = fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_IO),
		    "tmpfile");
		goto end;
	}
	if (s->f && fclose(s->f) == EOF) {
		rc = fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_IO),
		    "fclose");
		goto end;
	}
	s->f = fout;

	/*
	 * We'll diff artifacts of type "ci" (i.e., "checkin") separately, as
	 * it's a different process to diff the others (wiki, technote, etc.).
	 */
	if (!fsl_strcmp(s->selected_commit->type, "checkin")) {
		rc = create_changeset(s->selected_commit);
		if (rc) {
			rc = fsl_cx_err_set(f, FSL_RC_DB, "create_changeset");
			goto end;
		}
	} else
		diff_non_checkin(&s->buf, s->selected_commit, s->diff_flags,
		    s->context, s->sbs);

	rc = add_line_offset(&s->line_offsets, &s->nlines, 0);
	if (rc)
		goto end;

	/*
	 * If we don't have a timeline view, we arrived here via cmd_diff()
	 * (i.e., 'fnc diff' on the CLI), so don't display commit metadata.
	 */
	if (s->timeline_view)
		write_commit_meta(s);

	/*
	 * Diff local changes on disk in the current checkout differently to
	 * checked-in versions: the former compares on disk file content with
	 * file artifacts; the latter compares file artifact blobs only.
	 */
	if (s->selected_commit->rid == 0)
		diff_checkout(&s->buf, s->selected_commit->prid, s->diff_flags,
		    s->context, s->sbs);
	else if (!fsl_strcmp(s->selected_commit->type, "checkin") &&
	    s->selected_commit->puuid != NULL)
		diff_commit(&s->buf, s->selected_commit, s->diff_flags,
		    s->context, s->sbs);

	/*
	 * Parse the diff buffer line-by-line to record byte offsets of each
	 * line for scrolling and searching in diff view.
	 */
	st0 = fsl_strdup(fsl_buffer_str(&s->buf));
	st = st0;
	lnoff = (s->line_offsets)[s->nlines - 1];
	while ((line = fnc_strsep(&st, "\n")) != NULL) {
		n = fsl_fprintf(s->f, "%s\n", line);
		lnoff += n;
		rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff);
		if (rc)
			goto end;
	}

	fputc('\n', s->f);
	++lnoff;
	rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff);
	if (rc)
		goto end;

end:
	free(st0);
	fsl_buffer_clear(&s->buf);
	if (s->f && fflush(s->f) != 0 && rc == 0)
		rc = fsl_cx_err_set(f, FSL_RC_IO, "fflush");
	return rc;
}

static int
create_changeset(struct fnc_commit_artifact *commit)
{
	fsl_cx		*f = fcli_cx();
	fsl_stmt	*st = NULL;
	fsl_db		*db = fsl_cx_db_repo(f);
	fsl_list	 changeset = fsl_list_empty;
	int		 rc = 0;

	rc = fsl_db_prepare_cached(db, &st,
	    "SELECT name, mperm, "
	    "(SELECT uuid FROM blob WHERE rid=mlink.pid), "
	    "(SELECT uuid FROM blob WHERE rid=mlink.fid), "
	    "(SELECT name FROM filename WHERE filename.fnid=mlink.pfnid) "
	    "FROM mlink JOIN filename ON filename.fnid=mlink.fnid "
	    "WHERE mlink.mid=%d AND NOT mlink.isaux "
	    "AND (mlink.fid > 0 "
	    "OR mlink.fnid NOT IN (SELECT pfnid FROM mlink WHERE mid=%d)) "
	    "ORDER BY name", commit->rid, commit->rid);
	if (rc)
		return fsl_cx_err_set(f, FSL_RC_DB, "fsl_db_prepare_cached");

	while ((rc = fsl_stmt_step(st)) == FSL_RC_STEP_ROW) {
		struct fsl_file_artifact *fdiff = NULL;
		const char *path, *oldpath, *olduuid, *uuid;
		/* TODO: Parse file mode to display in commit changeset. */
		/* int perm; */

		path = fsl_stmt_g_text(st, 0, NULL);	/* Current filename. */
		//perm = fsl_stmt_g_int32(st, 1);	/* File permissions. */
		olduuid = fsl_stmt_g_text(st, 2, NULL);	/* UUID before change */
		uuid = fsl_stmt_g_text(st, 3, NULL);	/* UUID after change. */
		oldpath = fsl_stmt_g_text(st, 4, NULL);	/* Old name, if chngd */

		fdiff = fsl_malloc(sizeof(struct fsl_file_artifact));
		fdiff->fc = fsl_malloc(sizeof(fsl_card_F));
		fdiff->fc->name = fsl_strdup(path);
		if (!uuid) {
			fdiff->fc->uuid = fsl_strdup(olduuid);
			fdiff->change = FSL_CKOUT_CHANGE_REMOVED;
		} else if (!olduuid) {
			fdiff->fc->uuid = fsl_strdup(uuid);
			fdiff->change = FSL_CKOUT_CHANGE_ADDED;
		} else if (oldpath) {
			fdiff->fc->uuid = fsl_strdup(uuid);
			fdiff->fc->priorName = fsl_strdup(oldpath);
			fdiff->change = FSL_CKOUT_CHANGE_RENAMED;
		} else {
			fdiff->fc->uuid = fsl_strdup(uuid);
			fdiff->change = FSL_CKOUT_CHANGE_MOD;
		}
		fsl_list_append(&changeset, fdiff);
	}

	commit->changeset = changeset;
	fsl_stmt_cached_yield(st);

	if (rc == FSL_RC_STEP_DONE)
		rc = 0;

	return rc;
}

static int
write_commit_meta(struct fnc_diff_view_state *s)
{
	char		*line = NULL, *st = NULL;
	fsl_size_t	 linelen, idx = 0;
	off_t		 lnoff = 0;
	int		 n, rc = 0;

	if ((n = fsl_fprintf(s->f,"%s %s\n", s->selected_commit->type,
	    s->selected_commit->uuid)) < 0)
		goto end;
	lnoff += n;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	if ((n = fsl_fprintf(s->f,"user: %s\n", s->selected_commit->user)) < 0)
		goto end;
	lnoff += n;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	if ((n = fsl_fprintf(s->f,"tags: %s\n", s->selected_commit->branch ?
	    s->selected_commit->branch : "/dev/null")) < 0)
		goto end;
	lnoff += n;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	if ((n = fsl_fprintf(s->f,"date: %s\n",
	    s->selected_commit->timestamp)) < 0)
		goto end;
	lnoff += n;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	fputc('\n', s->f);
	++lnoff;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	st = fsl_strdup(s->selected_commit->comment);
	if (st == NULL) {
		fcli_err_set(FSL_RC_OOM, "fsl_strdup");
		goto end;
	}
	while ((line = fnc_strsep(&st, "\n")) != NULL) {
		linelen = fsl_strlen(line);
		if (linelen >= s->ncols) {
			rc = wrapline(line, s->ncols, s, &lnoff);
			if (rc)
				goto end;
		}
		else {
			if ((n = fsl_fprintf(s->f, "%s\n", line)) < 0)
				goto end;
			lnoff += n;
			if ((rc = add_line_offset(&s->line_offsets, &s->nlines,
			    lnoff)))
				goto end;

		}
	}

	fputc('\n', s->f);
	++lnoff;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	for (idx = 0; idx < s->selected_commit->changeset.used; ++idx) {
		char				*changeline;
		struct fsl_file_artifact	*file_change;

		file_change = s->selected_commit->changeset.list[idx];

		switch (file_change->change) {
		case FSL_CKOUT_CHANGE_MOD:
			changeline = "[~] ";
			break;
		case FSL_CKOUT_CHANGE_ADDED:
			changeline = "[+] ";
			break;
		case FSL_CKOUT_CHANGE_RENAMED:
			changeline = fsl_mprintf("[>] %s -> ",
			   file_change->fc->priorName);
			break;
		case FSL_CKOUT_CHANGE_REMOVED:
			changeline = "[-] ";
			break;
		default:
			changeline = "[!] ";
			break;
		}
		if ((n = fsl_fprintf(s->f, "%s%s\n", changeline,
		    file_change->fc->name)) < 0)
			goto end;
		lnoff += n;
		if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
			goto end;
	}

end:
	free(st);
	free(line);
	if (rc) {
		free(*&s->line_offsets);
		s->line_offsets = NULL;
		s->nlines = 0;
	}
	return rc;
}

/*
 * Wrap long lines at the terminal's available column width. The caller
 * must ensure the ncols_avail parameter has taken into account whether the
 * screen is currently split, and not mistakenly pass in the curses COLS macro
 * without deducting the parent panel's width. This function doesn't break
 * words, and will wrap at the end of the last word that can wholly fit within
 * the ncols_avail limit.
 */
static int
wrapline(char *line, fsl_size_t ncols_avail, struct fnc_diff_view_state *s,
    off_t *lnoff)
{
	char		*word;
	fsl_size_t	 wordlen, cursor = 0;
	int		 n = 0, rc = 0;

	while ((word = fnc_strsep(&line, " ")) != NULL) {
		wordlen = fsl_strlen(word);
		if ((cursor + wordlen) >= ncols_avail) {
			fputc('\n', s->f);
			++(*lnoff);
			rc = add_line_offset(&s->line_offsets, &s->nlines,
			    *lnoff);
			if (rc)
				return rc;
			cursor = 0;
		}
		if ((n  = fsl_fprintf(s->f, "%s ", word)) < 0)
			return rc;
		*lnoff += n;
		cursor += n;
	}
	fputc('\n', s->f);
	++(*lnoff);
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, *lnoff)))
		return rc;

	return 0;
}

static int
add_line_offset(off_t **line_offsets, size_t *nlines, off_t off)
{
	fsl_cx	*f = fcli_cx();
	off_t	*p;

	p = fsl_realloc(*line_offsets, (*nlines + 1) * sizeof(off_t));
	if (p == NULL)
		return fsl_cx_err_set(f, FSL_RC_OOM, "fsl_realloc");
	*line_offsets = p;
	(*line_offsets)[*nlines] = off;
	(*nlines)++;

	return 0;
}

/*
 * Fill the buffer with the differences between commit->uuid and commit->puuid.
 * commit->rid (to load into deck d2) is the *this* version, and commit->puuid
 * (to be loaded into deck d1) is the version we diff against. Step through the
 * deck of F(ile) cards from both versions to determine: (1) if we have new
 * files added (i.e., no F card counterpart in d1); (2) files deleted (i.e., no
 * F card counterpart in d2); (3) or otherwise the same file (i.e., F card
 * exists in both d1 and d2). In cases (1) and (2), we call diff_file_artifact()
 * to dump the complete content of the added/deleted file if FSL_DIFF_VERBOSE is
 * set, otherwise only diff metatadata will be output. In case (3), if the
 * hash (UUID) of each F card is the same, there are no changes; if different,
 * both artifacts will be passed to diff_file_artifact() to be diffed.
 */
static int
diff_commit(fsl_buffer *buf, struct fnc_commit_artifact *commit, int diff_flags,
    int context, int sbs)
{
	fsl_cx			*f = fcli_cx();
	const fsl_card_F	*fc1 = NULL;
	const fsl_card_F	*fc2 = NULL;
	fsl_deck		 d1 = fsl_deck_empty;
	fsl_deck		 d2 = fsl_deck_empty;
	fsl_id_t		 id1;
	int			 different = 0, rc = 0;

	rc = fsl_deck_load_rid(f, &d2, commit->rid, FSL_SATYPE_CHECKIN);
	if (rc)
		goto end;
	rc = fsl_deck_F_rewind(&d2);
	if (rc)
		goto end;

	/*
	 * If this commit has no parent (initial empty checkin), there's
	 * nothing to diff against. TODO: May be redundant with the
	 * this->puuid != NULL check in the caller.
	 */
	if (d2.P.used == 0)
		goto end;

	rc = fsl_sym_to_rid(f, commit->puuid, FSL_SATYPE_CHECKIN, &id1);
	if (rc)
		goto end;
	rc = fsl_deck_load_rid(f, &d1, id1, FSL_SATYPE_CHECKIN);
	if (rc)
		goto end;
	rc = fsl_deck_F_rewind(&d1);
	if (rc)
		goto end;

	fsl_deck_F_next(&d1, &fc1);
	fsl_deck_F_next(&d2, &fc2);
	while (fc1 || fc2) {
		const fsl_card_F	*a = NULL, *b = NULL;
		const char		*curr;
		fsl_ckout_change_e	 change = FSL_CKOUT_CHANGE_NONE;

		if (fc2)
			curr = fc2->priorName ? fc2->priorName : fc2->name;
		else if (fc1)
			curr = fc1->priorName ? fc1->priorName : fc1->name;
		if (!fc1)	/* File added. */
			different = 1;
		else if (!fc2)	/* File deleted. */
			different = -1;
		else		/* Same filename in both versions. */
			different = fsl_strcmp(fc1->name, curr);

		if (different) {
			if (different > 0) {
				b = fc2;
				change = FSL_CKOUT_CHANGE_ADDED;
				fsl_deck_F_next(&d2, &fc2);
			} else if (different < 0) {
				a = fc1;
				change = FSL_CKOUT_CHANGE_REMOVED;
				fsl_deck_F_next(&d1, &fc1);
			}
			rc = diff_file_artifact(buf, id1, a, commit->rid, b,
			    change, diff_flags, context, sbs);
		} else if (!fsl_uuidcmp(fc1->uuid, fc2->uuid)) { /* No change */
			fsl_deck_F_next(&d1, &fc1);
			fsl_deck_F_next(&d2, &fc2);
		} else {
			change = FSL_CKOUT_CHANGE_MOD;
			rc = diff_file_artifact(buf, id1, fc1, commit->rid, fc2,
			    change, diff_flags, context, sbs);
			fsl_deck_F_next(&d1, &fc1);
			fsl_deck_F_next(&d2, &fc2);
		}
		if (rc == FSL_RC_RANGE || rc == FSL_RC_TYPE) {
			fsl_buffer_append(buf,
			    "\nBinary files cannot be diffed\n", -1);
			rc = 0;
			fsl_cx_err_reset(f);
		} else if (rc)
			goto end;
	}
end:
	fsl_deck_finalize(&d1);
	fsl_deck_finalize(&d2);
	return rc;
}

/*
 * Diff local changes on disk in the current checkout against either a previous
 * commit or, if no version has been supplied, the current checkout.
 *   buf  output buffer in which diff content is appended
 *   vid  repository database record id of the version to diff against
 * diff_flags, context, and sbs are the same parameters as diff_file_artifact()
 * nb. This routine is only called with 'fnc diff [hash]'; that is, one or
 * zero args—not two—supplied to fnc's diff command line interface.
 */
static int
diff_checkout(fsl_buffer *buf, fsl_id_t vid, int diff_flags, int context,
    int sbs)
{
	fsl_cx		*f = fcli_cx();
	fsl_db		*db = fsl_cx_db_ckout(f);
	fsl_stmt	*st = NULL;
	fsl_buffer	 sql, abspath, bminus;
	fsl_uuid_str	 xminus = NULL;
	fsl_id_t	 cid;
	int		 rc = 0;

	abspath = bminus = sql = fsl_buffer_empty;
	fsl_ckout_version_info(f, &cid, NULL);
	/* cid = fsl_config_get_id(f, FSL_CONFDB_CKOUT, 0, "checkout"); */
	/* XXX Already done in cmd_diff(): Load vfile table with local state. */
	/* rc = fsl_vfile_changes_scan(f, cid, */
	/*     FSL_VFILE_CKSIG_ENOTFILE & FSL_VFILE_CKSIG_KEEP_OTHERS); */
	/* if (rc) */
	/*	return fcli_err_set(rc, "fsl_vfile_changes_scan"); */

	/*
	 * If a previous version is supplied, load its vfile state to query
	 * changes. Otherwise query the current checkout state for changes.
	 */
	if (vid != cid) {
		/* Keep vfile ckout state; but unload vid when finished. */
		rc = fsl_vfile_load(f, vid, false, NULL);
		if (rc)
			goto unload;
		fsl_buffer_appendf(&sql, "SELECT v2.pathname, v2.deleted, "
		    "v2.chnged, v2.rid == 0, v1.rid, v1.islink"
		    " FROM vfile v1, vfile v2"
		    " WHERE v1.pathname=v2.pathname AND v1.vid=%d AND v2.vid=%d"
		    " AND (v2.deleted OR v2.chnged OR v1.mrid != v2.rid)"
		    " UNION "
		    "SELECT pathname, 1, 0, 0, 0, islink"
		    " FROM vfile v1"
		    " WHERE v1.vid = %d"
		    " AND NOT EXISTS(SELECT 1 FROM vfile v2"
		    " WHERE v2.vid = %d AND v2.pathname = v1.pathname)"
		    " UNION "
		    "SELECT pathname, 0, 0, 1, 0, islink"
		    " FROM vfile v2"
		    " WHERE v2.vid = %d"
		    " AND NOT EXISTS(SELECT 1 FROM vfile v1"
		    " WHERE v1.vid = %d AND v1.pathname = v2.pathname)"
		    " ORDER BY 1", vid, cid, vid, cid, cid, vid);
	} else {
		fsl_buffer_appendf(&sql, "SELECT pathname, deleted, chnged, "
		    "rid == 0, rid, islink"
		    " FROM vfile"
		    " WHERE vid = %d"
		    " AND (deleted OR chnged OR rid == 0)"
		    " ORDER BY pathname", cid);
	}
	if ((rc = fsl_db_prepare_cached(db, &st, "%b", &sql)))
		goto yield;

	while ((rc = fsl_stmt_step(st)) == FSL_RC_STEP_ROW) {
		const char	*path;
		int		 deleted, changed, added, fid, symlink;
		enum		 fsl_ckout_change_e change;

		path = fsl_stmt_g_text(st, 0, NULL);
		deleted = fsl_stmt_g_int32(st, 1);
		changed = fsl_stmt_g_int32(st, 2);
		added = fsl_stmt_g_int32(st, 3);
		fid = fsl_stmt_g_int32(st, 4);
		symlink = fsl_stmt_g_int32(st, 5);
		rc = fsl_file_canonical_name2(f->ckout.dir, path, &abspath,
		    false);
		if (rc)
			goto yield;

		if (deleted)
			change = FSL_CKOUT_CHANGE_REMOVED;
		else if (fsl_file_access(fsl_buffer_cstr(&abspath), F_OK))
			change = FSL_CKOUT_CHANGE_MISSING;
		else if (added) {
			fid = 0;
			change = FSL_CKOUT_CHANGE_ADDED;
		} else if (changed == 3) {
			fid = 0;
			change = FSL_CKOUT_CHANGE_MERGE_ADD;
		} else if (changed == 5) {
			fid = 0;
			change = FSL_CKOUT_CHANGE_INTEGRATE_ADD;
		} else
			change = FSL_CKOUT_CHANGE_MOD;

		/*
		 * For changed files of which this checkout is already aware,
		 * grab their hash to make comparisons. For removed files, if
		 * diffing against a version other than the current checkout,
		 * load the version's manifest to parse for known versions of
		 * said files. If we don't, we risk diffing stale or bogus
		 * content. Known cases include MISSING, DELETED, and RENAMED
		 * files, which fossil(1) misses in some instances.
		 */
		if (fid > 0)
			xminus = fsl_rid_to_uuid(f, fid);
		else if (vid != cid && !added) {
			fsl_deck		 d = fsl_deck_empty;
			const fsl_card_F	*cf = NULL;

			rc = fsl_deck_load_rid(f, &d, vid, FSL_SATYPE_CHECKIN);
			if (!rc)
				rc = fsl_deck_F_rewind(&d);
			if (rc)
				goto yield;
			do {
				fsl_deck_F_next(&d, &cf);
				if (cf && !fsl_strcmp(cf->name, path)) {
					xminus = fsl_strdup(cf->uuid);
					if (xminus == NULL) {
						fcli_err_set(FSL_RC_OOM,
						    "fsl_strdup");
						goto yield;
					}
					fid = fsl_uuid_to_rid(f, xminus);
					break;
				}
			} while (cf);
		}
		if (!xminus)
			xminus = fsl_strdup(NULL_DEVICE);

		if (!symlink != !fsl_is_symlink(fsl_buffer_cstr(&abspath))) {
			rc = write_diff_meta(buf, path, xminus, path,
			    NULL_DEVICE, diff_flags, change);
			fsl_buffer_append(buf, "\nSymbolic links and regular "
			    "files cannot be diffed\n", -1);
			if (rc)
				goto yield;
			continue;
		}
		if (fid > 0 && change != FSL_CKOUT_CHANGE_ADDED)
			rc = fsl_content_get(f, fid, &bminus);
		else
			fsl_buffer_clear(&bminus);
		if (!rc)
			rc = diff_file(buf, &bminus, path, xminus,
			    fsl_buffer_cstr(&abspath), change, diff_flags,
			    context, sbs);
		fsl_buffer_reuse(&bminus);
		fsl_buffer_reuse(&abspath);
		fsl_free(xminus);
		xminus = NULL;
		if (rc == FSL_RC_RANGE || rc == FSL_RC_TYPE) {
			fsl_buffer_append(buf,
			    "\nBinary files cannot be diffed\n", -1);
			rc = 0;
			fsl_cx_err_reset(f);
		} else if (rc)
			goto yield;
	}

yield:
	fsl_stmt_cached_yield(st);
	fsl_free(xminus);
unload:
	fsl_vfile_unload_except(f, cid);
	fsl_buffer_clear(&abspath);
	fsl_buffer_clear(&bminus);
	fsl_buffer_clear(&sql);
	return rc;
}

/*
 * Write diff index line and file metadata (i.e., file paths and hashes), which
 * signify file addition, removal, or modification.
 *   buf         output buffer in which diff output will be appended
 *   zminus      file name of the file being diffed against
 *   xminus      hex hash of file named zminus
 *   zplus       file name of the file being diffed
 *   xplus       hex hash of the file named zplus
 *   diff_flags  bitwise flags to control the diff
 *   change      enum denoting the versioning change of the file
 */
static int
write_diff_meta(fsl_buffer *buf, const char *zminus, fsl_uuid_str xminus,
    const char *zplus, fsl_uuid_str xplus, int diff_flags,
    enum fsl_ckout_change_e change)
{
	int	rc = 0;
	const char	*index, *plus, *minus;

	index = zminus ? zminus : (zplus ? zplus : NULL_DEVICE);

	switch (change) {
	case FSL_CKOUT_CHANGE_MERGE_ADD:
		/* FALL THROUGH */
	case FSL_CKOUT_CHANGE_INTEGRATE_ADD:
		/* FALL THROUGH */
	case FSL_CKOUT_CHANGE_ADDED:
		minus = NULL_DEVICE;
		plus = xplus;
		zminus = NULL_DEVICE;
		break;
	case FSL_CKOUT_CHANGE_MISSING:
		/* FALL THROUGH */
	case FSL_CKOUT_CHANGE_REMOVED:
		minus = xminus;
		plus = NULL_DEVICE;
		zplus = NULL_DEVICE;
		break;
	case FSL_CKOUT_CHANGE_RENAMED:
		/* FALL THROUGH */
	case FSL_CKOUT_CHANGE_MOD:
		/* FALL THROUGH */
	default:
		minus = xminus;
		plus = xplus;
		break;
	}

	if ((diff_flags & (FSL_DIFF_SIDEBYSIDE | FSL_DIFF_BRIEF)) == 0) {
		rc = fsl_buffer_appendf(buf, "\nIndex: %s\n%.71c\n", index, '=');
		if (!rc)
			rc = fsl_buffer_appendf(buf, "hash - %s\nhash + %s\n",
			    minus, plus);
	}
	if (!rc && (diff_flags & FSL_DIFF_BRIEF) == 0)
		rc = fsl_buffer_appendf(buf, "--- %s\n+++ %s\n", zminus, zplus);

	return rc;
}

/*
 * The diff_file_artifact() counterpart that diffs actual files on disk rather
 * than file artifacts in the Fossil repository's blob table.
 *   buf      output buffer in which diff output will be appended
 *   bminus   blob containing content of the versioned file being diffed against
 *   zminus   filename of bminus
 *   xminus   hex UUID containing the SHA{1,3} hash of the file named zminus
 *   abspath  absolute path to the file on disk being diffed
 *   change   enum denoting the versioning change of the file
 * diff_flags, context, and sbs are the same parameters as diff_file_artifact()
 */
static int
diff_file(fsl_buffer *buf, fsl_buffer *bminus, const char *zminus,
    fsl_uuid_str xminus, const char *abspath, enum fsl_ckout_change_e change,
    int diff_flags, int context, bool sbs)
{
	fsl_cx		*f = fcli_cx();
	fsl_buffer	 bplus = fsl_buffer_empty;
	fsl_buffer	 xplus = fsl_buffer_empty;
	const char	*zplus;
	int		 rc = 0;
	bool		 verbose;

	/*
	 * If it exists, read content of abspath to diff EXCEPT for the content
	 * of 'fossil rm FILE' files because they will either: (1) have the same
	 * content as the versioned file's blob in bminus or (2) have changes.
	 * As a result, the upcoming call to fsl_diff_text_to_buffer() _will_
	 * (1) produce an empty diff or (2) show the differences; neither are
	 * expected behaviour because the SCM has been instructed to remove the
	 * file; therefore, the diff should display the versioned file content
	 * as being entirely removed. With this check, fnc now contrasts the
	 * behaviour of fossil(1), which produces the abovementioned unexpected
	 * output described in (1) and (2).
	 */
	if (fsl_file_size(abspath) < 0)
		zplus = NULL_DEVICE;
	else if (change != FSL_CKOUT_CHANGE_REMOVED) {
		rc = fsl_ckout_file_content(f, abspath, &bplus);
		if (rc)
			goto end;
		/*
		 * To replicate fossil(1)'s behaviour—where a fossil rm'd file
		 * will either show as an unchanged or edited rather than a
		 * removed file with 'fossil diff -v' output—remove the above
		 * 'if (change != FSL_CKOUT_CHANGE_REMOVED)' from the else
		 * condition and uncomment the following three lines of code.
		 */
		/* if (change == FSL_CKOUT_CHANGE_REMOVED && */
		/*     !fsl_buffer_compare(bminus, &bplus)) */
		/*	fsl_buffer_clear(&bplus); */
		zplus = zminus;
	}

	switch (fsl_strlen(xminus)) {
	case FSL_STRLEN_K256:
		rc = fsl_sha3sum_buffer(&bplus, &xplus);
		break;
	case FSL_STRLEN_SHA1:
		rc = fsl_sha1sum_buffer(&bplus, &xplus);
		break;
	case NULL_DEVICELEN:
		switch (fsl_config_get_int32(f, FSL_CONFDB_REPO,
		    FSL_HPOLICY_AUTO, "hash-policy")) {
		case FSL_HPOLICY_SHA1:
			rc = fsl_sha1sum_buffer(&bplus, &xplus);
			break;
		case FSL_HPOLICY_AUTO:
			/* FALL THROUGH */
		case FSL_HPOLICY_SHA3:
			/* FALL THROUGH */
		case FSL_HPOLICY_SHA3_ONLY:
			rc = fsl_sha3sum_buffer(&bplus, &xplus);
			break;
		}
		break;
	default:
		fcli_err_set(FSL_RC_SIZE_MISMATCH,
		    "invalid artifact uuid [%s], xminus");
		goto end;
	}
	if (rc)
		goto end;

	rc = write_diff_meta(buf, zminus, xminus, zplus, fsl_buffer_str(&xplus),
	    diff_flags, change);
	if (rc)
		goto end;

	verbose = (diff_flags & FSL_DIFF_VERBOSE) != 0 ? true : false;
	if (diff_flags & FSL_DIFF_BRIEF) {
		rc = fsl_buffer_compare(bminus, &bplus);
		if (!rc)
			rc = fsl_buffer_appendf(buf, "CHANGED -> %s\n", zminus);
	} else if (verbose || (bminus->used && bplus.used)) {
		rc = fsl_diff_text_to_buffer(bminus, &bplus, buf, context,
		    sbs, diff_flags);
	}

end:
	fsl_buffer_clear(&bplus);
	fsl_buffer_clear(&xplus);

	return rc;
}

/*
 * Parse the deck of non-checkin commits to present a 'fossil ui' equivalent
 * of the corresponding artifact when selected from the timeline.
 * TODO: Rename this horrible function name.
 */
static int
diff_non_checkin(fsl_buffer *buf, struct fnc_commit_artifact *commit,
    int diff_flags, int context, int sbs)
{
	fsl_cx		*f = fcli_cx();
	fsl_buffer	 wiki = fsl_buffer_empty;
	fsl_buffer	 pwiki = fsl_buffer_empty;
	fsl_id_t	 prid = 0;
	fsl_size_t	 idx;
	int		 rc = 0;

	fsl_deck *d = NULL;
	d = fsl_deck_malloc();
	if (d == NULL)
		return fsl_cx_err_set(f, FSL_RC_OOM, "fsl_deck_malloc");

	fsl_deck_init(f, d, FSL_SATYPE_ANY);
	if ((rc = fsl_deck_load_rid(f, d, commit->rid, FSL_SATYPE_ANY)))
		goto end;

	/*
	 * Present ticket commits as a series of field: value tuples as per
	 * the Fossil UI /info/UUID view.
	 */
	if (d->type == FSL_SATYPE_TICKET) {
		for (idx = 0; idx < d->J.used; ++idx) {
			fsl_card_J *ticket = d->J.list[idx];
			bool icom = !fsl_strncmp(ticket->field, "icom", 4);
			fsl_buffer_appendf(buf, "%d. %s:%s%s%c\n", idx + 1,
			    ticket->field, icom ? "\n\n" : " ", ticket->value,
			    icom ? '\n' : ' ');
		}
		goto end;
	}

	if (d->type == FSL_SATYPE_CONTROL) {
		for (idx = 0; idx < d->T.used; ++idx) {
			fsl_card_T *ctl = d->T.list[idx];
			fsl_buffer_appendf(buf, "Tag %d ", idx + 1);
			switch (ctl->type) {
			case FSL_TAGTYPE_CANCEL:
				fsl_buffer_append(buf, "[CANCEL]", -1);
				break;
			case FSL_TAGTYPE_ADD:
				fsl_buffer_append(buf, "[ADD]", -1);
				break;
			case FSL_TAGTYPE_PROPAGATING:
				fsl_buffer_append(buf, "[PROPAGATE]", -1);
				break;
			default:
				break;
			}
			if (ctl->uuid)
				fsl_buffer_appendf(buf, "\ncheckin %s",
				    ctl->uuid);
			fsl_buffer_appendf(buf, "\n%s", ctl->name);
			if (!fsl_strcmp(ctl->name, "branch"))
				commit->branch = fsl_strdup(ctl->value);
			if (ctl->value)
				fsl_buffer_appendf(buf, " -> %s", ctl->value);
			fsl_buffer_append(buf, "\n\n", 2);
		}
		goto end;
	}
	/*
	 * If neither a ticket nor control artifact, we assume it's a wiki, so
	 * check if it has a parent commit to diff against. If not, append the
	 * entire wiki card content.
	 */
	fsl_buffer_append(&wiki, d->W.mem, d->W.used);
	if (commit->puuid == NULL) {
		if (d->P.used > 0)
			commit->puuid = fsl_strdup(d->P.list[0]);
		else {
			fsl_buffer_copy(&wiki, buf);
			goto end;
		}
	}

	/* Diff the artifacts if a parent is found. */
	if ((rc = fsl_sym_to_rid(f, commit->puuid, FSL_SATYPE_ANY, &prid)))
		goto end;
	if ((rc = fsl_deck_load_rid(f, d, prid, FSL_SATYPE_ANY)))
		goto end;
	fsl_buffer_append(&pwiki, d->W.mem, d->W.used);

	rc = fsl_diff_text_to_buffer(&pwiki, &wiki, buf, context, sbs,
	    diff_flags);

	/* If a technote, provide the full content after its diff. */
	if (d->type == FSL_SATYPE_TECHNOTE)
		fsl_buffer_appendf(buf, "\n---\n\n%s", wiki.mem);

end:
	fsl_buffer_clear(&wiki);
	fsl_buffer_clear(&pwiki);
	fsl_deck_finalize(d);
	return rc;
}

/*
 * Compute the differences between two repository file artifacts to produce the
 * set of changes necessary to convert one into the other.
 *   buf         output buffer in which diff output will be appended
 *   vid1        repo record id of the version from which artifact a belongs
 *   a           file artifact being diffed against
 *   vid2        repo record id of the version from which artifact b belongs
 *   b           file artifact being diffed
 *   change      enum denoting the versioning change of the file
 *   diff_flags  bitwise flags to control the diff
 *   context     the number of context lines to surround changes
 *   sbs	 number of columns in which to display each side-by-side diff
 */
static int
diff_file_artifact(fsl_buffer *buf, fsl_id_t vid1, const fsl_card_F *a,
    fsl_id_t vid2, const fsl_card_F *b, enum fsl_ckout_change_e change,
    int diff_flags, int context, int sbs)
{
	fsl_cx		*f = fcli_cx();
	fsl_buffer	 fbuf1 = fsl_buffer_empty;
	fsl_buffer	 fbuf2 = fsl_buffer_empty;
	const char	*zplus = NULL, *zminus = NULL;
	fsl_uuid_str	 xplus = NULL, xminus = NULL;
	int		 rc = 0;
	bool		 verbose;

	assert(vid1 != vid2);
	assert(vid1 > 0 && vid2 > 0 &&
	    "local checkout should be diffed with diff_checkout()");

	fbuf2.used = fbuf1.used = 0;

	if (a) {
		rc = fsl_card_F_content(f, a, &fbuf1);
		if (rc)
			goto end;
		zminus = a->name;
		xminus = a->uuid;
	}
	if (b) {
		rc = fsl_card_F_content(f, b, &fbuf2);
		if (rc)
			goto end;
		zplus = b->name;
		xplus = b->uuid;
	}

	rc = write_diff_meta(buf, zminus, xminus, zplus, xplus, diff_flags,
	    change);
	verbose = (diff_flags & FSL_DIFF_VERBOSE) != 0 ? true : false;
	if (verbose || (a && b))
		rc = fsl_diff_text_to_buffer(&fbuf1, &fbuf2, buf, context, sbs,
		    diff_flags);
	if (rc)
		fcli_err_set(rc, "Error %s: fsl_diff_text_to_buffer\n"
		    " -> %s [%s]\n -> %s [%s]", fsl_rc_cstr(rc),
		    a ? a->name : NULL_DEVICE, a ? a->uuid : NULL_DEVICE,
		    b ? b->name : NULL_DEVICE, b ? b->uuid : NULL_DEVICE);
end:
	fsl_buffer_clear(&fbuf1);
	fsl_buffer_clear(&fbuf2);
	return rc;
}

static int
fsl_ckout_file_content(fsl_cx *f, char const *path, fsl_buffer *dest)
{
	fsl_buffer	fname = fsl_buffer_empty;
	int		rc;

	if (!f || !path || !*path || !dest)
		return FSL_RC_MISUSE;
	else if (!fsl_needs_ckout(f))
		return FSL_RC_NOT_A_CKOUT;

	fname.used = 0;
	rc = fsl_file_canonical_name2(fsl_cx_ckout_dir_name(f, NULL), path,
	    &fname, true);
	if (!rc) {
		assert(fname.used);
		if (fname.mem[fname.used - 1] == '/') {
			rc = fsl_cx_err_set(f, FSL_RC_MISUSE,
			    "Filename may not have a trailing slash.");
		} else {
			dest->used = 0;
			rc = fsl_buffer_fill_from_filename(dest,
			    fsl_buffer_cstr(&fname));
		}
	}
	fsl_buffer_clear(&fname);

	return rc;
}

static int
show_diff(struct fnc_view *view)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
	fsl_cx				*f = fcli_cx();
	char				*headln;

	if ((headln = fsl_mprintf("diff %.40s %.40s",
	    s->selected_commit->puuid ? s->selected_commit->puuid : "/dev/null",
	    s->selected_commit->uuid)) == NULL)
		return fsl_cx_err_set(f, FSL_RC_RANGE, "mprintf");

	return write_diff(view, headln);
}

static int
write_diff(struct fnc_view *view, char *headln)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
	fsl_cx				*f = fcli_cx();
	regmatch_t			*regmatch = &view->regmatch;
	struct fnc_colour		*c = NULL;
	wchar_t				*wcstr;
	char				*line;
	size_t				 linesz = 0;
	ssize_t				 linelen;
	off_t				 line_offset;
	int				 wstrlen;
	int				 max_lines = view->nlines;
	int				 nlines = s->nlines;
	int				 rc = 0, nprintln = 0;
	int				 match = -1;

	line_offset = s->line_offsets[s->first_line_onscreen - 1];
	if (fseeko(s->f, line_offset, SEEK_SET))
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "fseeko");

	werase(view->window);

	if (headln) {
		if ((line = fsl_mprintf("[%d/%d] %s", (s->first_line_onscreen -
		    1 + s->current_line), nlines, headln)) == NULL)
			return fsl_cx_err_set(f, fsl_errno_to_rc(errno,
			    FSL_RC_ERROR), "mprintf");
		rc = formatln(&wcstr, &wstrlen, line, view->ncols, 0);
		fsl_free(line);
		fsl_free(headln);
		if (rc)
			return rc;

		if (screen_is_shared(view))
			wstandout(view->window);
		waddwstr(view->window, wcstr);
		fsl_free(wcstr);
		wcstr = NULL;
		if (screen_is_shared(view))
			wstandend(view->window);
		if (wstrlen <= view->ncols - 1)
			waddch(view->window, '\n');

		if (max_lines <= 1)
			return rc;
		--max_lines;
	}

	s->eof = false;
	line = NULL;
	while (max_lines > 0 && nprintln < max_lines) {
		linelen = getline(&line, &linesz, s->f);
		if (linelen == -1) {
			if (feof(s->f)) {
				s->eof = true;
				break;
			}
			fsl_free(line);
			fsl_cx_err_set(f, fsl_errno_to_rc(ferror(s->f),
			    FSL_RC_IO), "getline");
			return rc;
		}

		if (s->colour && (match = fsl_list_index_of(&s->colours, line,
		    match_line)) != -1)
			c = s->colours.list[match];
		if (c)
			wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
		if (s->first_line_onscreen + nprintln == s->matched_line &&
		    regmatch->rm_so >= 0 && regmatch->rm_so < regmatch->rm_eo) {
			rc = write_matched_line(&wstrlen, line, view->ncols, 0,
			    view->window, regmatch);
			if (rc) {
				fsl_free(line);
				return rc;
			}
		} else {
			rc = formatln(&wcstr, &wstrlen, line, view->ncols, 0);
			if (rc) {
				fsl_free(line);
				return rc;
			}
			waddwstr(view->window, wcstr);
			fsl_free(wcstr);
			wcstr = NULL;
		}
		if (c) {
			wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
			c = NULL;
		}
		if (wstrlen <= view->ncols - 1)
			waddch(view->window, '\n');
		++nprintln;
	}
	fsl_free(line);
	if (nprintln >= 1)
		s->last_line_onscreen = s->first_line_onscreen + (nprintln - 1);
	else
		s->last_line_onscreen = s->first_line_onscreen;

	draw_vborder(view);

	if (s->eof) {
		while (nprintln < view->nlines) {
			waddch(view->window, '\n');
			++nprintln;
		}

		wstandout(view->window);
		waddstr(view->window, "(END)");
		wstandend(view->window);
	}

	return rc;
}

static int
match_line(const void *ln, const void *key)
{
	struct fnc_colour *c = (struct fnc_colour *)key;
	const char *line = ln;

	return regexec(&c->regex, line, 0, NULL, 0);
}

static bool
screen_is_shared(struct fnc_view *view)
{
	if (view_is_parent(view)) {
		if (view->child == NULL || view->child->active ||
		    !screen_is_split(view->child))
			return false;
	} else if (!screen_is_split(view))
		return false;

	return view->active;
}

static int
view_is_parent(struct fnc_view *view)
{
	return view->parent == NULL;
}

static int
screen_is_split(struct fnc_view *view)
{
	return view->start_col > 0;
}

static int
write_matched_line(int *col_pos, const char *line, int ncols_avail,
    int start_column, WINDOW *window, regmatch_t *regmatch)
{
	fsl_cx		*f = fcli_cx();
	wchar_t		*wcstr;
	char		*s;
	int		 wstrlen;
	int		 rc = 0;

	*col_pos = 0;

	/* Copy the line up to the matching substring & write it to screen. */
	s = fsl_strndup(line, regmatch->rm_so);
	if (s == NULL)
		return fsl_cx_err_set(f, FSL_RC_OOM, "fsl_strndup");

	rc = formatln(&wcstr, &wstrlen, s, ncols_avail, start_column);
	if (rc) {
		free(s);
		return rc;
	}
	waddwstr(window, wcstr);
	free(wcstr);
	free(s);
	ncols_avail -= wstrlen;
	*col_pos += wstrlen;

	/* If not EOL, copy matching string & write to screen with highlight. */
	if (ncols_avail > 0) {
		s = fsl_strndup(line + regmatch->rm_so,
		    regmatch->rm_eo - regmatch->rm_so);
		if (s == NULL) {
			rc = fsl_cx_err_set(f, FSL_RC_OOM, "strndup");
			free(s);
			return rc;
		}
		rc = formatln(&wcstr, &wstrlen, s, ncols_avail, start_column);
		if (rc) {
			free(s);
			return rc;
		}
		wattr_on(window, A_STANDOUT, NULL);
		waddwstr(window, wcstr);
		wattr_off(window, A_STANDOUT, NULL);
		free(wcstr);
		free(s);
		ncols_avail -= wstrlen;
		*col_pos += wstrlen;
	}

	/* Write the rest of the line if not yet at EOL. */
	if (ncols_avail > 0 && fsl_strlen(line) > (fsl_size_t)regmatch->rm_eo) {
		rc = formatln(&wcstr, &wstrlen, line + regmatch->rm_eo,
		    ncols_avail, start_column);
		if (rc)
			return rc;
		waddwstr(window, wcstr);
		free(wcstr);
		*col_pos += wstrlen;
	}

	return rc;
}

static void
draw_vborder(struct fnc_view *view)
{
	const struct fnc_view	*view_above;
	char			*codeset = nl_langinfo(CODESET);
	PANEL			*panel;

	if (view->parent)
		draw_vborder(view->parent);

	panel = panel_above(view->panel);
	if (panel == NULL)
		return;

	view_above = panel_userptr(panel);
	mvwvline(view->window, view->start_ln, view_above->start_col - 1,
	    (strcmp(codeset, "UTF-8") == 0) ? ACS_VLINE : '|', view->nlines);
#ifdef __linux__
	wnoutrefresh(view->window);
#endif
}

static int
diff_input_handler(struct fnc_view **new_view, struct fnc_view *view, int ch)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
	struct fnc_tl_view_state	*tlstate;
	struct commit_entry		*previous_selection;
	fsl_cx				*f = fcli_cx();
	char				*line = NULL;
	ssize_t				 linelen;
	size_t				 linesz = 0;
	int				 i, rc = 0;
	bool				 tl_down = false;

	switch (ch) {
	case KEY_DOWN:
	case 'j':
		if (!s->eof)
			++s->first_line_onscreen;
		break;
	case KEY_NPAGE:
	case CTRL('f'):
		if (s->eof)
			break;
		i = 0;
		while (!s->eof && i++ < view->nlines - 1) {
			linelen = getline(&line, &linesz, s->f);
			++s->first_line_onscreen;
			if (linelen == -1) {
				if (feof(s->f))
					s->eof = true;
				else
					rc = fsl_cx_err_set(f, fsl_errno_to_rc
					    (errno, FSL_RC_IO), "ftell");
				break;
			}
		}
		free(line);
		break;
	case KEY_END:
	case 'G':
		if (s->eof)
			break;
		s->first_line_onscreen = (s->nlines - view->nlines) + 2;
		s->eof = true;
		break;
	case KEY_UP:
	case 'k':
		if (s->first_line_onscreen > 1)
			--s->first_line_onscreen;
		break;
	case KEY_PPAGE:
	case CTRL('b'):
		if (s->first_line_onscreen == 1)
			break;
		i = 0;
		while (i++ < view->nlines - 1 && s->first_line_onscreen > 1)
			--s->first_line_onscreen;
		break;
	case 'g': {
		bool home = true;
		halfdelay(10);	/* Only block for 1 second, then return ERR. */
		if (wgetch(view->window) != 'g')
			home = false;
		cbreak();	/* Return to blocking mode on user input. */
		if (!home)
			break;
		/* FALL THROUGH */
	}
	case KEY_HOME:
		s->first_line_onscreen = 1;
		break;
	case 'c':
	case 'i':
	case 'v':
	case 'w':
		if (ch == 'c')
			s->colour = s->colour == false;
		if (ch == 'i')
			s->diff_flags ^= FSL_DIFF_INVERT;
		if (ch == 'v')
			s->diff_flags ^= FSL_DIFF_VERBOSE;
		if (ch == 'w')
			s->diff_flags ^= FSL_DIFF_IGNORE_ALLWS;
		wclear(view->window);
		s->first_line_onscreen = 1;
		s->last_line_onscreen = view->nlines;
		show_diff_status(view);
		rc = create_diff(s);
		break;
	case '-':
	case '_':
		if (s->context > 0) {
			--s->context;
			show_diff_status(view);
			rc = create_diff(s);
			if (s->first_line_onscreen + view->nlines - 1 >
			    (int)s->nlines) {
				s->first_line_onscreen = 1;
				s->last_line_onscreen = view->nlines;
			}
		}
		break;
	case '+':
	case '=':
		if (s->context < DIFF_MAX_CTXT) {
			++s->context;
			show_diff_status(view);
			rc = create_diff(s);
		}
		break;
	case CTRL('j'):
	case '>':
	case '.':
	case 'J':
		tl_down = true;
		/* FALL THROUGH */
	case CTRL('k'):
	case '<':
	case ',':
	case 'K':
		if (s->timeline_view == NULL)
			break;
		tlstate = &s->timeline_view->state.timeline;
		previous_selection = tlstate->selected_commit;

		if ((rc = tl_input_handler(NULL, s->timeline_view,
		    tl_down ? KEY_DOWN : KEY_UP)))
			break;

		if (previous_selection == tlstate->selected_commit)
			break;

		if ((rc = set_selected_commit(s, tlstate->selected_commit)))
			break;

		s->first_line_onscreen = 1;
		s->last_line_onscreen = view->nlines;

		show_diff_status(view);
		rc = create_diff(s);
		break;
	default:
		break;
	}

	return rc;
}

static int
set_selected_commit(struct fnc_diff_view_state *s, struct commit_entry *entry)
{
	s->selected_commit = entry->commit;

	return 0;
}

static int
diff_search_init(struct fnc_view *view)
{
	struct fnc_diff_view_state *s = &view->state.diff;

	s->matched_line = 0;
	return 0;
}

static int
diff_search_next(struct fnc_view *view)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
	fsl_cx				*f = fcli_cx();
	char				*line = NULL;
	ssize_t				 linelen;
	size_t				 linesz = 0;
	int				 start_ln;

	if (view->searching == SEARCH_DONE) {
		view->search_status = SEARCH_CONTINUE;
		return 0;
	}

	if (s->matched_line) {
		if (view->searching == SEARCH_FORWARD)
			start_ln = s->matched_line + 1;
		else
			start_ln = s->matched_line - 1;
	} else {
		if (view->searching == SEARCH_FORWARD)
			start_ln = 1;
		else
			start_ln = s->nlines;
	}

	while (1) {
		off_t offset;

		if (start_ln <= 0 || start_ln > (int)s->nlines) {
			if (s->matched_line == 0) {
				view->search_status = SEARCH_CONTINUE;
				break;
			}

			if (view->searching == SEARCH_FORWARD)
				start_ln = 1;
			else
				start_ln = s->nlines;
		}

		offset = s->line_offsets[start_ln - 1];
		if (fseeko(s->f, offset, SEEK_SET) != 0) {
			free(line);
			return fsl_cx_err_set(f, fsl_errno_to_rc(errno,
			    FSL_RC_IO), "fseeko");
		}
		linelen = getline(&line, &linesz, s->f);
		if (linelen != -1 && regexec(&view->regex, line, 1,
		    &view->regmatch, 0) == 0) {
			view->search_status = SEARCH_CONTINUE;
			s->matched_line = start_ln;
			break;
		}
		if (view->searching == SEARCH_FORWARD)
			++start_ln;
		else
			--start_ln;
	}
	free(line);

	if (s->matched_line) {
		s->first_line_onscreen = s->matched_line;
		s->current_line = 1;
	}

	return 0;
}

static int
close_diff_view(struct fnc_view *view)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
	struct fsl_list_state		st = { FNC_COLOUR_OBJ };
	fsl_cx				*f = fcli_cx();
	int				 rc = 0;

	if (s->f && fclose(s->f) == EOF)
		rc = fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_IO),
		    "fclose");
	free(s->line_offsets);
	fsl_list_clear(&s->colours, fsl_list_object_free, &st);
	s->line_offsets = NULL;
	s->nlines = 0;
	return rc;
}

static void
fnc_resizeterm(void)
{
	struct winsize	size;
	int		cols, lines;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) < 0) {
		cols = 80;
		lines = 24;
	} else {
		cols = size.ws_col;
		lines = size.ws_row;
	}
	resize_term(lines, cols);
}

static int
view_resize(struct fnc_view *view)
{
	fsl_cx	*f = fcli_cx();
	int	 nlines, ncols;

	if (view->lines > LINES)
		nlines = view->nlines - (view->lines - LINES);
	else
		nlines = view->nlines + (LINES - view->lines);

	if (view->cols > COLS)
		ncols = view->ncols - (view->cols - COLS);
	else
		ncols = view->ncols + (COLS - view->cols);

	if (wresize(view->window, nlines, ncols) == ERR)
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "curses wresize -> %s", __func__);
	if (replace_panel(view->panel, view->window) == ERR)
		return fsl_cx_err_set(f, fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "curses replace_panel -> %s", __func__);
	wclear(view->window);

	view->nlines = nlines;
	view->ncols = ncols;
	view->lines = LINES;
	view->cols = COLS;

	if (view->child) {
		view->child->start_col = view_split_start_col(view->start_col);
		if (view->child->start_col == 0) {
			make_fullscreen(view->child);
			if (view->child->active)
				show_panel(view->child->panel);
			else
				show_panel(view->panel);
		} else {
			make_splitscreen(view->child);
			show_panel(view->child->panel);
		}
	}

	return 0;
}

/*
 * Consume repeatable arguments containing artifact type values used in
 * constructing the SQL query to generate commit records of the specified
 * type for the timeline. TODO: Enhance this to generalise processing of
 * various repeatable arguments--paths, usernames, branches, etc.--so we
 * can filter on multiples of these values.
 */
static int
fcli_flag_type_arg_cb(fcli_cliflag const *v)
{
	if (fnc_init.filter_types->nitems)
		fnc_init.filter_types->values =
		    fsl_realloc(fnc_init.filter_types->values,
		    (fnc_init.filter_types->nitems + 1) * sizeof(char *));
	fnc_init.filter_types->values[fnc_init.filter_types->nitems++] =
	    *((const char **)v->flagValue);

	return FCLI_RC_FLAG_AGAIN;
}

static void
sigwinch_handler(int sig)
{
	if (sig == SIGWINCH) {
		struct winsize winsz;

		ioctl(0, TIOCGWINSZ, &winsz);
		rec_sigwinch = 1;
	}
}

static void
sigpipe_handler(int sig)
{
	struct sigaction	sact;
	int			e;

	rec_sigpipe = 1;
	memset(&sact, 0, sizeof(sact));
	sact.sa_handler = SIG_IGN;
	sact.sa_flags = SA_RESTART;
	e = sigaction(SIGPIPE, &sact, NULL);
	if (e)
		err(1, "SIGPIPE");
}

static void
sigcont_handler(int sig)
{
	rec_sigcont = 1;
}

__dead static void
usage(void)
{
	/*
	 * It looks like the fsl_cx f member of the ::fcli singleton has
	 * already been cleaned up by the time this wrapper is called from
	 * fcli_help() after hijacking the process whenever the '--help'
	 * argument is passsed on the command line, so we can't use the
	 * f->output fsl_outputer implementation as we would like.
	 */
	/* fsl_cx *f = fcli_cx(); */
	/* f->output = fsl_outputer_FILE; */
	/* f->output.state.state = (fnc_init.err == true) ? stderr : stdout; */
        FILE *f = fnc_init.err ? stderr : stdout;
	size_t idx = 0;

	endwin();

	/* If a command was passed on the CLI, output its corresponding help. */
	if (fnc_init.cmdarg)
		for (idx = 0; idx < nitems(fnc_init.cmd_args); ++idx) {
			if (!fsl_strcmp(fnc_init.cmdarg,
			    fnc_init.cmd_args[idx].name)) {
				fsl_fprintf(f, "[%s] command:\n\n usage:",
				    fnc_init.cmd_args[idx].name);
				    fnc_init.fnc_usage_cb[idx]();
				fcli_cliflag_help(fnc_init.cmd_args[idx].flags);
				exit(fcli_end_of_main(fnc_init.err));
			}
		}

	/* Otherwise, output help/usage for all commands. */
	fcli_command_help(fnc_init.cmd_args, false);
	fsl_fprintf(f, "[usage]\n\n");
	usage_timeline();
	usage_diff();
	usage_blame();
	fsl_fprintf(f, "  note: %s "
	    "with no args defaults to the timeline command.\n\n",
	    fcli_progname());

	exit(fcli_end_of_main(fnc_init.err));
}

static void
usage_timeline(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " %s timeline [-T tag] [-b branch] [-c sym]"
	    " [-h|--help] [-n n] [-t type] [-u user] [-z|--utc]\n"
	    "  e.g.: %s timeline --type ci -u jimmy\n\n",
	    fcli_progname(), fcli_progname());
}

static void
usage_diff(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " %s diff [-c|--no-colour] [-h|--help] [-i|--invert]"
	    " [-q|--quiet] [-w|--whitespace] [-x|--context n] "
	    "[sym ...]\n  e.g.: %s diff --context 3 d34db33f c0ff33\n\n",
	    fcli_progname(), fcli_progname());
}

static void
usage_blame(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " %s blame [-h|--help] [-c hash] artifact\n"
	    "  e.g.: %s blame -c d34db33f src/foo.c\n\n" ,
	    fcli_progname(), fcli_progname());
}

static int
cmd_diff(fcli_command const *argv)
{
	fsl_cx				*f = fcli_cx();
	struct fnc_view			*view;
	struct fnc_commit_artifact	*commit = NULL;
	const char			*artifact1, *artifact2, *s;
	char				*ptr;
	fsl_id_t			 prid = -1, rid = -1;
	int				 context, rc = 0;

	rc = fcli_process_flags(argv->flags);
	if (rc || (rc = fcli_has_unused_flags(false)))
		return rc;

	/*
	 * If only one artifact is supplied, use the local changes in the
	 * current checkout to diff against it. If no artifacts are supplied,
	 * diff any local changes on disk against the current checkout.
	 */
	if (fcli.argc == 2) {
		artifact1 = fcli_next_arg(true);
		artifact2 = fcli_next_arg(true);
	} else if (fcli.argc < 2) {
		artifact1 = "current";
		rid = 0;
		if (fcli_next_arg(false))
			artifact1 = fcli_next_arg(true);
		if ((rc = fsl_ckout_changes_scan(f)))
			return fcli_err_set(rc, "fsl_ckout_changes_scan");
		if (!fsl_strcmp(artifact1, "current") &&
		    !fsl_ckout_has_changes(f)) {
			fsl_fprintf(stdout, "No local changes.\n");
			return rc;
		}
	} else { /* fcli_* APIs should prevent getting here but just in case. */
		usage_diff();
		return fcli_err_set(FSL_RC_MISUSE, "\ninvalid args");
	}

	/* Find the corresponding rids for the versions we have; checkout = 0 */
	rc = fsl_sym_to_rid(f, artifact1, FSL_SATYPE_ANY, &prid);
	if (rc || prid < 0)
		return fcli_err_set(rc, "invalid artifact [%s]", artifact1);
	if (rid != 0)
		rc = fsl_sym_to_rid(f, artifact2, FSL_SATYPE_ANY, &rid);
	if (rc || rid < 0)
		return fcli_err_set(rc, "invalid artifact [%s]", artifact2);

	commit = calloc(1, sizeof(*commit));
	if (commit == NULL)
		return fcli_err_set(FSL_RC_OOM, "calloc fail");
	if (rid == 0)
		fsl_ckout_version_info(f, NULL, (fsl_uuid_cstr *)commit->uuid);
	else
		commit->uuid = fsl_rid_to_uuid(f, rid);
	commit->puuid = fsl_rid_to_uuid(f, prid);
	commit->prid = prid;
	commit->rid = rid;

	/*
	 * If either of the supplied versions are not checkin artifacts,
	 * let the user know which operands aren't valid.
	 */
	if ((fsl_rid_is_a_checkin(f, rid) || rid == 0) &&
	    fsl_rid_is_a_checkin(f, prid))
		commit->type = fsl_strdup("checkin");
	else
		return fcli_err_set(FSL_RC_TYPE,
		    "artifact(s) [%s] not resolvable to a checkin",
		    (!fsl_rid_is_a_checkin(f, rid) && rid != 0) &&
		    !fsl_rid_is_a_checkin(f, prid) ?
		    fsl_mprintf("%s / %s", artifact1, artifact2) :
		    (!fsl_rid_is_a_checkin(f, rid) && rid != 0) ?
		    artifact2 : artifact1);

	init_curses();

	if (fnc_init.context) {
		s = fnc_init.context;
		context = strtol(s, &ptr, 10);
		if (errno == ERANGE)
			return fsl_cx_err_set(f, FSL_RC_RANGE,
			    "out of range: -x|--context=%s [%s]", s, ptr);
		else if (errno != 0 || errno == EINVAL)
			return fsl_cx_err_set(f, FSL_RC_MISUSE,
			    "not a number: -x|--context=%s [%s]", s, ptr);
		else if (ptr && *ptr != '\0')
			return fsl_cx_err_set(f, FSL_RC_MISUSE,
			    "invalid char: -x|--context=%s [%s]", s, ptr);
		context = MIN(DIFF_MAX_CTXT, context);
	} else
		context = 5;

	view = view_open(0, 0, 0, 0, FNC_VIEW_DIFF);
	if (view == NULL)
		return fcli_err_set(FSL_RC_OOM, "view_open fail");

	rc = open_diff_view(view, commit, context, fnc_init.ws,
	    fnc_init.invert, !fnc_init.quiet, NULL);
	if (!rc)
		rc = view_loop(view);

	fnc_commit_artifact_close(commit);
	return rc;
}

static int
cmd_blame(fcli_command const *argv)
{
	/* Not yet implemened. */
	f_out("%s blame is not yet implemented.\n", fcli_progname());
	return FSL_RC_NYI;
}

static void
fnc_show_version(void)
{
	printf("%s %s\n", fcli_progname(), PRINT_VERSION);
}

static char *
fnc_strsep(char **ptr, const char *sep)
{
	char	*s, *token;

	if ((s = *ptr) == NULL)
		return NULL;

	if (*(token = s + strcspn(s, sep)) != '\0') {
		*token++ = '\0';
		*ptr = token;
	} else
		*ptr = NULL;

	return s;
}

