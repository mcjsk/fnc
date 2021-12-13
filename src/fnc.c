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
#include <sys/stat.h>

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
#include "settings.h"

#define FNC_VERSION	VERSION  /* cf. Makefile */

/* Utility macros. */
#define MIN(_a, _b)	((_a) < (_b) ? (_a) : (_b))
#define MAX(_a, _b)	((_a) > (_b) ? (_a) : (_b))
#define ABS(_n)		((_n) >= 0 ? (_n) : -(_n))
#if !defined(CTRL)
#define CTRL(key)	((key) & 037)	/* CTRL+<key> input. */
#endif
#define nitems(a)	(sizeof((a)) / sizeof((a)[0]))
#define STRINGIFYOUT(s)	#s
#define STRINGIFY(s)	STRINGIFYOUT(s)
#define CONCATOUT(a, b)	a ## b
#define CONCAT(a, b)	CONCATOUT(a, b)
#define FILE_POSITION	__FILE__ ":" STRINGIFY(__LINE__)
#define FLAG_SET(f, b)	((f) |= (b))
#define FLAG_CHK(f, b)	((f) & (b))
#define FLAG_TOG(f, b)	((f) ^= (b))
#define FLAG_CLR(f, b)	((f) &= ~(b))

/* Application macros. */
#define PRINT_VERSION	STRINGIFY(FNC_VERSION)
#define DEF_DIFF_CTX	5		/* Default diff context lines. */
#define MAX_DIFF_CTX	64		/* Max diff context lines. */
#define HSPLIT_SCALE	0.4		/* Default horizontal split scale. */
#define SPIN_INTERVAL	200		/* Status line progress indicator. */
#define SPINNER		"\\|/-\0"
#define NULL_DEVICE	"/dev/null"
#define NULL_DEVICELEN	(sizeof(NULL_DEVICE) - 1)
#define KEY_ESCAPE	27
#if DEBUG
#define RC(r, fmt, ...)	fcli_err_set(r, "%s::%s " fmt,			\
			    __func__, FILE_POSITION, __VA_ARGS__)
#else
#define RC(r, fmt, ...) fcli_err_set(r, fmt, __VA_ARGS__)
#endif /* DEBUG */

/* Portability macros. */
#ifndef __OpenBSD__
#ifndef HAVE_STRTONUM
#  define strtonum(s, min, max, o) strtol(s, (char **)o, 10)
# endif /* HAVE_STRTONUM */
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

/*
 * STAILQ was added to OpenBSD 6.9; fallback to SIMPLEQ for prior versions.
 * XXX This is an ugly hack; replace with a better solution.
 */
#ifdef __OpenBSD__
# ifndef STAILQ_HEAD
#  define STAILQ SIMPLEQ
# endif /* STAILQ_HEAD */
#endif /* OpenBSD */

#if defined __linux__
# ifndef strlcat
#  define strlcat(_d, _s, _sz) fsl_strlcat(_d, _s, _sz)
# endif /* strlcat */
# ifndef strlcpy
#  define strlcpy(_d, _s, _sz) fsl_strlcpy(_d, _s, _sz)
# endif /* strlcpy */
#endif /* __linux__ */

__dead static void	usage(void);
static void		usage_timeline(void);
static void		usage_diff(void);
static void		usage_tree(void);
static void		usage_blame(void);
static void		usage_branch(void);
static void		usage_config(void);
static int		fcli_flag_type_arg_cb(fcli_cliflag const *);
static int		cmd_timeline(fcli_command const *);
static int		cmd_diff(fcli_command const *);
static int		cmd_tree(fcli_command const *);
static int		cmd_blame(fcli_command const *);
static int		cmd_branch(fcli_command const *);
static int		cmd_config(fcli_command const *);

/*
 * Singleton initialising global configuration and state for app startup.
 */
static struct fnc_setup {
	/* Global options. */
	const char	*cmdarg;	/* Retain argv[1] for use/err report. */
	const char	*sym;		/* Open view from this symbolic name. */
	const char	*path;		/* Optional path for timeline & tree. */
	int		 err;		/* Indicate fnc error state. */
	bool		 hflag;		/* Flag if --help is requested. */
	bool		 vflag;		/* Flag if --version is requested. */
	bool		 reverse;	/* Reverse branch sort or blame. */

	/* Timeline options. */
	struct artifact_types {
		const char	**values;
		short		  nitems;
	} filter_types;			/* Only load commits of <type>. */
	union {
		const char	 *zlimit;
		long		  limit;
	} nrecords;			/* Number of commits to load. */
	const char	*filter_tag;	/* Only load commits with <tag>. */
	const char	*filter_branch;	/* Only load commits from <branch>. */
	const char	*filter_user;	/* Only load commits from <user>. */
	const char	*filter_type;	/* Placeholder for repeatable types. */
	const char	*glob;		/* Only load commits containing glob */
	bool		 utc;		/* Display UTC sans user local time. */

	/* Diff options. */
	const char	*context;	/* Number of context lines. */
	bool		 ws;		/* Ignore whitespace-only changes. */
	bool		 nocolour;	/* Disable colour in diff output. */
	bool		 quiet;		/* Disable verbose diff output. */
	bool		 invert;	/* Toggle inverted diff output. */

	/* Branch options. */
	const char	*before;	/* Last branch change before date. */
	const char	*after;		/* Last branch change after date. */
	const char	*sort;		/* Lexicographical, MRU, open/closed. */
	bool		 closed;	/* Show only closed branches. */
	bool		 open;		/* Show only open branches */
	bool		 noprivate;	/* Don't show private branches. */

	/* Config options. */
	bool		 lsconf;	/* List all defined settings. */
	bool		 unset;		/* Unset the specified setting. */

	/* Command line flags and help. */
	fcli_help_info	  fnc_help;			/* Global help. */
	fcli_cliflag	  cliflags_global[3];		/* Global options. */
	fcli_command	  cmd_args[7];			/* App commands. */
	fcli_cliflag	  cliflags_timeline[13];	/* Timeline options. */
	fcli_cliflag	  cliflags_diff[8];		/* Diff options. */
	fcli_cliflag	  cliflags_tree[5];		/* Tree options. */
	fcli_cliflag	  cliflags_blame[7];		/* Blame options. */
	fcli_cliflag	  cliflags_branch[11];		/* Branch options. */
	fcli_cliflag	  cliflags_config[5];		/* Config options. */
} fnc_init = {
	NULL,		/* cmdarg copy of argv[1] to aid usage/error report. */
	NULL,		/* sym(bolic name) of commit to open defaults to tip. */
	NULL,		/* path for tree to open or timeline to find commits. */
	0,		/* err fnc error state. */
	false,		/* hflag if --help is requested. */
	false,		/* vflag if --version is requested. */
	false,		/* reverse branch sort/annotation defaults to off. */
	{NULL, 0},	/* filter_types defaults to indiscriminate. */
	{0},		/* nrecords defaults to all commits. */
	NULL,		/* filter_tag defaults to indiscriminate. */
	NULL,		/* filter_branch defaults to indiscriminate. */
	NULL,		/* filter_user defaults to indiscriminate. */
	NULL,		/* filter_type temp placeholder for filter_types cb. */
	NULL,		/* glob filter defaults to off; all commits are shown */
	false,		/* utc defaults to off (i.e., show user local time). */
	NULL,		/* context defaults to five context lines. */
	false,		/* ws defaults to acknowledge whitespace. */
	false,		/* nocolour defaults to off (i.e., use diff colours). */
	false,		/* quiet defaults to off (i.e., verbose diff is on). */
	false,		/* invert diff defaults to off. */
	NULL,		/* before defaults to any time. */
	NULL,		/* after defaults to any time. */
	NULL,		/* sort by MRU or open/closed (dflt: lexicographical) */
	false,		/* closed only branches is off (defaults to all). */
	false,		/* open only branches is off by (defaults to all). */
	false,		/* noprivate is off (default to show private branch). */
	false,		/* do not list all defined settings by default. */
	false,		/* default to set—not unset—the specified setting. */

	{ /* fnc_help global app help details. */
	    "An ncurses browser for Fossil repositories in the terminal.",
	    NULL, usage
	},

	{ /* cliflags_global global app options. */
	    FCLI_FLAG_BOOL("h", "help", &fnc_init.hflag,
	    "Display program help and usage then exit."),
	    FCLI_FLAG_BOOL("v", "version", &fnc_init.vflag,
	    "Display program version number and exit."),
	    fcli_cliflag_empty_m
	},

	{ /* cmd_args available app commands. */
	    {"timeline", "tl\0time\0ti\0log\0",
	    "Show chronologically descending commit history of the repository.",
	    cmd_timeline, usage_timeline, fnc_init.cliflags_timeline},
	    {"diff", "di\0",
	    "Show changes to versioned files introduced with a given commit.",
	    cmd_diff, usage_diff, fnc_init.cliflags_diff},
	    {"tree", "tr\0dir\0",
	    "Show repository tree corresponding to a given commit",
	    cmd_tree, usage_tree, fnc_init.cliflags_tree},
	    {"blame", "bl\0praise\0pr\0annotate\0an\0",
	    "Show commit attribution history for each line of a file.",
	    cmd_blame, usage_blame, fnc_init.cliflags_blame},
	    {"branch", "br\0tag\0",
	    "Show navigable list of repository branches.",
	    cmd_branch, usage_branch, fnc_init.cliflags_branch},
	    {"config", "conf\0cfg\0settings\0set\0",
	    "Configure or view currently available settings.",
	    cmd_config, usage_config, fnc_init.cliflags_config},
	    {NULL, NULL, NULL, NULL, NULL}	/* Sentinel. */
	},

	{ /* cliflags_timeline timeline command related options. */
	    FCLI_FLAG("b", "branch", "<branch>", &fnc_init.filter_branch,
	    "Only display commits that reside on the given <branch>."),
	    FCLI_FLAG_BOOL("C", "no-colour", &fnc_init.nocolour,
	    "Disable colourised timeline, which is enabled by default on\n    "
	    "supported terminals. Colour can also be toggled with the 'c' "
	    "\n    key binding in timeline view when this option is not used."),
	    FCLI_FLAG("c", "commit", "<commit>", &fnc_init.sym,
	    "Open the timeline from <commit>. Common symbols are:\n"
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
	    FCLI_FLAG_CSTR("f", "filter", "<glob>", &fnc_init.glob,
	    "Populate the timeline with commits containing <glob> in the commit"
	    "\n    comment, user, or branch field."),
	    FCLI_FLAG_BOOL("h", "help", NULL,
	    "Display timeline command help and usage."),
	    FCLI_FLAG("n", "limit", "<n>", &fnc_init.nrecords.zlimit,
	    "Limit display to <n> latest commits; defaults to entire history "
	    "of\n    current checkout. Negative values are a no-op."),
	    FCLI_FLAG_CSTR("R", "repo", "<path>", NULL,
	    "Use the fossil(1) repository located at <path> for this timeline\n"
	    "    invocation."),
	    FCLI_FLAG("T", "tag", "<tag>", &fnc_init.filter_tag,
	    "Only display commits with T cards containing <tag>."),
	    FCLI_FLAG_X("t", "type", "<type>", &fnc_init.filter_type,
	    fcli_flag_type_arg_cb,
	    "Only display <type> commits. Valid types are:\n"
	    "\tci - check-in\n"
	    "\tw  - wiki\n"
	    "\tt  - ticket\n"
	    "\te  - technote\n"
	    "\tf  - forum post\n"
	    "\tg  - tag artifact\n"
	    "    n.b. This is a repeatable flag (e.g., -t ci -t w)."),
	    FCLI_FLAG("u", "username", "<user>", &fnc_init.filter_user,
	    "Only display commits authored by <username>."),
	    FCLI_FLAG_BOOL("z", "utc", &fnc_init.utc,
	    "Use UTC (instead of local) time."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_timeline. */

	{ /* cliflags_diff diff command related options. */
	    FCLI_FLAG_BOOL("C", "no-colour", &fnc_init.nocolour,
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
	    FCLI_FLAG_CSTR("R", "repo", "<path>", NULL,
	    "Use the fossil(1) repository located at <path> for this diff\n    "
	    "invocation."),
	    FCLI_FLAG_BOOL("w", "whitespace", &fnc_init.ws,
	    "Ignore whitespace-only changes when displaying diff. This option "
	    "can\n    also be toggled with the 'w' key binding in diff view."),
	    FCLI_FLAG("x", "context", "<n>", &fnc_init.context,
	    "Show <n> context lines when displaying diff; <n> is capped at 64."
	    "\n    Negative values are a no-op."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_diff. */

	{ /* cliflags_tree tree command related options. */
	    FCLI_FLAG_BOOL("C", "no-colour", &fnc_init.nocolour,
	    "Disable coloured output, which is enabled by default on supported"
	    "\n    terminals. Colour can also be toggled with the 'c' key "
	    "binding when\n    this option is not used."),
	    FCLI_FLAG("c", "commit", "<commit>", &fnc_init.sym,
	    "Display tree that reflects repository state as at <commit>.\n"
	    "    Common symbols are:"
	    "\n\tSHA{1,3} hash\n"
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
	    "Display tree command help and usage."),
	    FCLI_FLAG_CSTR("R", "repo", "<path>", NULL,
	    "Use the fossil(1) repository located at <path> for this tree\n    "
	    "invocation."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_tree. */

	{ /* cliflags_blame blame command related options. */
	    FCLI_FLAG_BOOL("C", "no-colour", &fnc_init.nocolour,
	    "Disable coloured output, which is enabled by default on supported"
	    "\n    terminals. Colour can also be toggled with the 'c' key "
	    "binding when\n    this option is not used."),
	    FCLI_FLAG("c", "commit", "<commit>", &fnc_init.sym,
	    "Start blame of specified file from <commit>. Common symbols are:\n"
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
	    "Display blame command help and usage."),
	    FCLI_FLAG("n", "limit", "<n>", &fnc_init.nrecords.zlimit,
	    "Limit depth of blame history to <n> commits or seconds. Denote the"
	    "\n    latter by postfixing 's' (e.g., 30s). Useful for large files"
	    " with\n    extensive history. Persists for the duration of the "
	    "session."),
	    FCLI_FLAG_CSTR("R", "repo", "<path>", NULL,
	    "Use the fossil(1) repository located at <path> for this blame\n"
	    "    invocation."),
	    FCLI_FLAG_BOOL("r", "reverse", &fnc_init.reverse,
	    "Reverse annotate the file starting from a historical commit. "
	    "Rather\n    than show the most recent change of each line, show "
	    "the first time\n    each line was modified after the specified "
	    "commit. Requires -c|--commit."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_blame. */

	{ /* cliflags_branch branch command related options. */
	    FCLI_FLAG_BOOL("C", "no-colour", &fnc_init.nocolour,
	    "Disable coloured output, which is enabled by default on supported"
	    "\n    terminals. Colour can also be toggled with the 'c' key "
	    "binding when\n    this option is not used."),
	    FCLI_FLAG("a", "after", "<date>", &fnc_init.after,
	    "Show branches with last activity occuring after <date>, which is\n"
	    "    expected to be either an ISO8601 (e.g., 2020-10-10) or "
	    "unambiguous\n    DD/MM/YYYY or MM/DD/YYYY formatted date."),
	    FCLI_FLAG("b", "before", "<date>", &fnc_init.before,
	    "Show branches with last activity occuring before <date>, which is"
	    "\n    expected to be either an ISO8601 (e.g., 2020-10-10) or "
	    "unambiguous\n    DD/MM/YYYY or MM/DD/YYYY formatted date."),
	    FCLI_FLAG_BOOL("c", "closed", &fnc_init.closed,
	    "Show closed branches only. Open and closed branches are listed by "
	    "\n    default."),
	    FCLI_FLAG_BOOL("h", "help", NULL,
	    "Display branch command help and usage."),
	    FCLI_FLAG_BOOL("o", "open", &fnc_init.open,
	    "Show open branches only. Open and closed branches are listed by "
	    "\n    default."),
	    FCLI_FLAG_BOOL("p", "no-private", &fnc_init.noprivate,
	    "Do not show private branches, which are otherwise included in the"
	    "\n    list of displayed branches by default."),
	    FCLI_FLAG_CSTR("R", "repo", "<path>", NULL,
	    "Use the fossil(1) repository located at <path> for this branch\n"
	    "    invocation."),
	    FCLI_FLAG_BOOL("r", "reverse", &fnc_init.reverse,
	    "Reverse the order in which branches are displayed."),
	    FCLI_FLAG("s", "sort", "<order>", &fnc_init.sort,
	    "Sort branches by <order>. Available options are:\n"
	    "\tmru   - most recently used\n"
	    "\tstate - open/closed state\n    "
	    "Branches are sorted in lexicographical order by default."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_blame. */

	{ /* cliflags_config config command related options. */
	    FCLI_FLAG_BOOL("h", "help", NULL,
	    "Display config command help and usage."),
	    FCLI_FLAG_BOOL(NULL, "ls", &fnc_init.lsconf,
	    "Display a list of all currently defined settings."),
	    FCLI_FLAG_CSTR("R", "repo", "<path>", NULL,
	    "Use the fossil(1) repository located at <path> for this config\n"
	    "    invocation."),
	    FCLI_FLAG_BOOL("u", "unset", &fnc_init.unset,
	    "Unset (i.e., remove) the specified repository setting."),
	    fcli_cliflag_empty_m
	}, /* End cliflags_tree. */

};

enum date_string {
	ISO8601_DATE_ONLY = 10,
	ISO8601_DATE_HHMM = 16,
	ISO8601_TIMESTAMP = 20
};

enum fnc_view_id {
	FNC_VIEW_TIMELINE,
	FNC_VIEW_DIFF,
	FNC_VIEW_TREE,
	FNC_VIEW_BLAME,
	FNC_VIEW_BRANCH
};

enum fnc_view_mode {
	VIEW_SPLIT_NONE,
	VIEW_SPLIT_VERT,
	VIEW_SPLIT_HRZN
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

enum fnc_diff_type {
	FNC_DIFF_CKOUT,
	FNC_DIFF_COMMIT,
	FNC_DIFF_BLOB,
	FNC_DIFF_WIKI
};

struct fnc_colour {
	STAILQ_ENTRY(fnc_colour) entries;
	regex_t	regex;
	uint8_t	scheme;
};
STAILQ_HEAD(fnc_colours, fnc_colour);

struct fnc_commit_artifact {
	fsl_buffer		 wiki;
	fsl_buffer		 pwiki;
	fsl_list		 changeset;
	fsl_uuid_str		 uuid;
	fsl_uuid_str		 puuid;
	fsl_id_t		 rid;
	fsl_id_t		 prid;
	char			*user;
	char			*timestamp;
	char			*comment;
	char			*branch;
	char			*type;
	enum fnc_diff_type	 diff_type;
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

/*
 * The following two structs are used to construct the tree of the entire
 * repository; that is, from the root through to all subdirectories and files.
 */
struct fnc_repository_tree {
	struct fnc_repo_tree_node	*head;     /* Head of repository tree */
	struct fnc_repo_tree_node	*tail;     /* Final node in the tree. */
	struct fnc_repo_tree_node	*rootail;  /* Final root level node. */
};

struct fnc_repo_tree_node {
	struct fnc_repo_tree_node	*next;	     /* Next node in tree. */
	struct fnc_repo_tree_node	*prev;	     /* Prev node in tree. */
	struct fnc_repo_tree_node	*parent_dir; /* Dir containing node. */
	struct fnc_repo_tree_node	*sibling;    /* Next node in same dir */
	struct fnc_repo_tree_node	*children;   /* List of node children */
	struct fnc_repo_tree_node	*lastchild;  /* Last child in list. */
	char				*basename;   /* Final path component. */
	char				*path;	     /* Full pathname of node */
	fsl_uuid_str			 uuid;	     /* File artifact hash. */
	mode_t				 mode;	     /* File mode. */
	double				 mtime;	     /* Mod time of file. */
	uint_fast16_t			 pathlen;    /* Length of path. */
	uint_fast16_t			 nparents;   /* Path components sans- */
						     /* -basename. */
};

/*
 * The following two structs represent a given subtree within the repository;
 * for example, the top level tree and all its elements, or the elements of
 * the src/ directory (but not any members of src/ subdirectories).
 */
struct fnc_tree_object {
	struct fnc_tree_entry	*entries;  /* Array of tree entries. */
	int			 nentries; /* Number of tree entries. */
};

struct fnc_tree_entry {
	char			*basename; /* Final component of path. */
	char			*path;	   /* Full pathname of tree entry. */
	fsl_uuid_str		 uuid;	   /* File artifact hash. */
	mode_t			 mode;	   /* File mode. */
	double			 mtime;	   /* Modification time of file. */
	int			 idx;	   /* Index of this tree entry. */
};

/*
 * Each fnc_tree_object that is _not_ the repository root will have a (list of)
 * fnc_parent_tree(s) to be tracked.
 */
struct fnc_parent_tree {
	TAILQ_ENTRY(fnc_parent_tree)	 entry;
	struct fnc_tree_object		*tree;
	struct fnc_tree_entry		*first_entry_onscreen;
	struct fnc_tree_entry		*selected_entry;
	int				 selected_idx;
};

pthread_mutex_t fnc_mutex = PTHREAD_MUTEX_INITIALIZER;

struct fnc_tl_thread_cx {
	struct commit_queue	 *commits;
	struct commit_entry	**first_commit_onscreen;
	struct commit_entry	**selected_commit;
	fsl_db			 *db;
	fsl_stmt		 *q;
	regex_t			 *regex;
	char			 *path;	     /* Match commits involving path. */
	enum fnc_search_state	 *search_status;
	enum fnc_search_mvmnt	 *searching;
	int			  spin_idx;
	int			  ncommits_needed;
	/*
	 * XXX Is there a more elegant solution to retrieving return codes from
	 * thread functions while pinging between, but before we join, threads?
	 */
	int			  rc;
	bool			  endjmp;
	bool			  timeline_end;
	bool			  needs_reset;
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
	struct fnc_colours	 colours;
	const char		*curr_ckout_uuid;
	char			*path;	     /* Match commits involving path. */
	int			 selected_idx;
	int			 nscrolled;
	sig_atomic_t		 quit;
	pthread_t		 thread_id;
	bool			 colour;
};

struct fnc_pathlist_entry {
	TAILQ_ENTRY(fnc_pathlist_entry) entry;
	const char	*path;
	size_t		 pathlen;
	void		*data;  /* XXX May want to save id, mode, etc. */
};
TAILQ_HEAD(fnc_pathlist_head, fnc_pathlist_entry);

struct fnc_diff_view_state {
	struct fnc_view			*timeline_view;
	struct fnc_commit_artifact	*selected_commit;
	struct fnc_pathlist_head	*paths;
	fsl_buffer			 buf;
	struct fnc_colours		 colours;
	FILE				*f;
	fsl_uuid_str			 id1;
	fsl_uuid_str			 id2;
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
	bool				 showmeta;
};

TAILQ_HEAD(fnc_parent_trees, fnc_parent_tree);
struct fnc_tree_view_state {			  /* Parent trees of the- */
	struct fnc_parent_trees		 parents; /* -current subtree. */
	struct fnc_repository_tree	*repo;    /* The repository tree. */
	struct fnc_tree_object		*root;    /* Top level repo tree. */
	struct fnc_tree_object		*tree;    /* Currently displayed tree */
	struct fnc_tree_entry		*first_entry_onscreen;
	struct fnc_tree_entry		*last_entry_onscreen;
	struct fnc_tree_entry		*selected_entry;
	struct fnc_tree_entry		*matched_entry;
	struct fnc_colours		 colours;
	char				*tree_label;  /* Headline string. */
	fsl_uuid_str			 commit_id;
	fsl_id_t			 rid;
	int				 ndisplayed;
	int				 selected_idx;
	bool				 colour;
	bool				 show_id;
	bool				 show_date;
};

struct fnc_blame_line {
	fsl_uuid_str	id;
	bool		annotated;
};

struct fnc_blame_cb_cx {
	struct fnc_view		*view;
	struct fnc_blame_line	*lines;
	fsl_uuid_str		 commit_id;
	fsl_uuid_str		 root_commit;
	int			 nlines;
	bool			*quit;
};

typedef int (*fnc_cancel_cb)(void *);

struct fnc_blame_thread_cx {
	struct fnc_blame_cb_cx	*cb_cx;
	fsl_annotate_opt	 blame_opt;
	fnc_cancel_cb		 cancel_cb;
	const char		*path;
	void			*cancel_cx;
	bool			*complete;
};

struct fnc_blame {
	struct fnc_blame_thread_cx	 thread_cx;
	struct fnc_blame_cb_cx		 cb_cx;
	FILE				*f;	/* Non-annotated copy of file */
	struct fnc_blame_line		*lines;
	off_t				*line_offsets;
	off_t				 filesz;
	fsl_id_t			 origin; /* Tip rid for reverse blame */
	int				 nlines;
	int				 nlimit;    /* Limit depth traversal. */
	pthread_t			 thread_id;
};

CONCAT(STAILQ, _HEAD)(fnc_commit_id_queue, fnc_commit_qid);
struct fnc_commit_qid {
	CONCAT(STAILQ, _ENTRY)(fnc_commit_qid) entry;
	fsl_uuid_str	 id;
};

struct fnc_blame_view_state {
	struct fnc_blame		 blame;
	struct fnc_commit_id_queue	 blamed_commits;
	struct fnc_commit_qid		*blamed_commit;
	struct fnc_commit_artifact	*selected_commit;
	struct fnc_colours		 colours;
	fsl_uuid_str			 commit_id;
	char				*path;
	int				 first_line_onscreen;
	int				 last_line_onscreen;
	int				 selected_line;
	int				 matched_line;
	int				 spin_idx;
	bool				 done;
	bool				 blame_complete;
	bool				 eof;
	bool				 colour;
};

struct fnc_branch {
	char		*name;
	char		*date;
	fsl_uuid_str	 id;
	bool		 private;
	bool		 current;
	bool		 open;
};

struct fnc_branchlist_entry {
	TAILQ_ENTRY(fnc_branchlist_entry) entries;
	struct fnc_branch	*branch;
	int			 idx;
};
TAILQ_HEAD(fnc_branchlist_head, fnc_branchlist_entry);

struct fnc_branch_view_state {
	struct fnc_branchlist_head	 branches;
	struct fnc_branchlist_entry	*first_branch_onscreen;
	struct fnc_branchlist_entry	*last_branch_onscreen;
	struct fnc_branchlist_entry	*matched_branch;
	struct fnc_branchlist_entry	*selected_branch;
	struct fnc_colours		 colours;
	const char			*branch_glob;
	double				 dateline;
	int				 branch_flags;
#define BRANCH_LS_CLOSED_ONLY	0x001  /* Show closed branches only. */
#define BRANCH_LS_OPEN_ONLY	0x002  /* Show open branches only. */
#define BRANCH_LS_OPEN_CLOSED	0x003  /* Show open & closed branches (dflt). */
#define BRANCH_LS_BITMASK	0x003
#define BRANCH_LS_NO_PRIVATE	0x004  /* Show public branches only. */
#define BRANCH_SORT_MTIME	0x008  /* Sort by activity. (default: name) */
#define BRANCH_SORT_STATUS	0x010  /* Sort by open/closed. */
#define BRANCH_SORT_REVERSE	0x020  /* Reverse sort order. */
	int				 nbranches;
	int				 ndisplayed;
	int				 selected;
	int				 when;
	bool				 colour;
	bool				 show_date;
	bool				 show_id;
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
		struct fnc_tree_view_state	tree;
		struct fnc_blame_view_state	blame;
		struct fnc_branch_view_state	branch;
	} state;
	enum fnc_view_id	 vid;
	enum fnc_view_mode	 mode;
	enum fnc_search_state	 search_status;
	enum fnc_search_mvmnt	 searching;
	int			 nlines;	/* Dependent on split height. */
	int			 ncols;		/* Dependent on split width. */
	int			 start_ln;
	int			 start_col;
	int			 lines;		/* Always curses LINES macro */
	int			 cols;		/* Always curses COLS macro. */
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
static int		 open_timeline_view(struct fnc_view *, fsl_id_t,
			    const char *, const char *);
static int		 view_loop(struct fnc_view *);
static int		 show_timeline_view(struct fnc_view *);
static void		*tl_producer_thread(void *);
static int		 block_main_thread_signals(void);
static int		 build_commits(struct fnc_tl_thread_cx *);
static int		 commit_builder(struct fnc_commit_artifact **, fsl_id_t,
			    fsl_stmt *);
static int		 signal_tl_thread(struct fnc_view *, int);
static int		 draw_commits(struct fnc_view *);
static void		 parse_emailaddr_username(char **);
static int		 formatln(wchar_t **, int *, const char *, int, int);
static int		 multibyte_to_wchar(const char *, wchar_t **, size_t *);
static int		 write_commit_line(struct fnc_view *,
			    struct fnc_commit_artifact *, int);
static int		 view_input(struct fnc_view **, int *,
			    struct fnc_view *, struct view_tailhead *);
static int		 cycle_view(struct fnc_view *);
static int		 toggle_fullscreen(struct fnc_view **,
			    struct fnc_view *);
static int		 help(struct fnc_view *);
static int		 padpopup(struct fnc_view *, int, int, FILE *,
			    const char *);
static void		 centerprint(WINDOW *, int, int, int, const char *,
			    chtype);
static int		 tl_input_handler(struct fnc_view **, struct fnc_view *,
			    int);
static int		 move_tl_cursor_down(struct fnc_view *, bool);
static void		 move_tl_cursor_up(struct fnc_view *, bool, bool);
static int		 timeline_scroll_down(struct fnc_view *, int);
static void		 timeline_scroll_up(struct fnc_tl_view_state *, int);
static void		 select_commit(struct fnc_tl_view_state *);
static int		 open_commit_diff_view(struct fnc_view **,
			    struct fnc_view *);
static int		 view_set_split(struct fnc_view *, int *, int *, int *);
static int		 view_split_start_col(int);
static int		 view_split_start_ln(int);
static int		 make_splitscreen(struct fnc_view *);
static int		 make_fullscreen(struct fnc_view *);
static int		 view_search_start(struct fnc_view *);
static int		 tl_search_init(struct fnc_view *);
static int		 tl_search_next(struct fnc_view *);
static bool		 find_commit_match(struct fnc_commit_artifact *,
			    regex_t *);
static int		 init_diff_commit(struct fnc_view **, int, int,
			    struct fnc_commit_artifact *, struct fnc_view *);
static int		 open_diff_view(struct fnc_view *,
			    struct fnc_commit_artifact *, int, bool, bool, bool,
			    struct fnc_view *, bool,
			    struct fnc_pathlist_head *);
static void		 show_diff_status(struct fnc_view *);
static int		 create_diff(struct fnc_diff_view_state *);
static int		 create_changeset(struct fnc_commit_artifact *);
static int		 write_commit_meta(struct fnc_diff_view_state *);
static int		 wrapline(char *, fsl_size_t ncols_avail,
			    struct fnc_diff_view_state *, off_t *);
static int		 add_line_offset(off_t **, size_t *, off_t);
static int		 diff_commit(fsl_buffer *, struct fnc_commit_artifact *,
			    int, int, int, struct fnc_pathlist_head *);
static int		 diff_checkout(fsl_buffer *, fsl_id_t, int, int, int,
			    struct fnc_pathlist_head *);
static int		 write_diff_meta(fsl_buffer *, const char *,
			    fsl_uuid_str, const char *, fsl_uuid_str, int,
			    enum fsl_ckout_change_e);
static int		 diff_file(fsl_buffer *, fsl_buffer *, const char *,
			    fsl_uuid_str, const char *, enum fsl_ckout_change_e,
			    int, int, bool);
static int		 diff_non_checkin(fsl_buffer *,
			    struct fnc_commit_artifact *, int, int, int);
static int		 diff_file_artifact(fsl_buffer *, fsl_id_t,
			    const fsl_card_F *, fsl_id_t, const fsl_card_F *,
			    fsl_ckout_change_e, int, int, int,
			    enum fnc_diff_type);
static int		 show_diff(struct fnc_view *);
static int		 write_diff(struct fnc_view *, char *);
static int		 match_line(const char *, regex_t *, size_t,
			    regmatch_t *);
static int		 write_matched_line(int *, const char *, int, int,
			    WINDOW *, regmatch_t *);
static void		 drawborder(struct fnc_view *);
static int		 diff_input_handler(struct fnc_view **,
			    struct fnc_view *, int);
static int		 request_tl_commits(struct fnc_view *);
static int		 set_selected_commit(struct fnc_diff_view_state *,
			    struct commit_entry *);
static int		 diff_search_init(struct fnc_view *);
static int		 diff_search_next(struct fnc_view *);
static int		 view_close(struct fnc_view *);
static int		 map_repo_path(char **);
static bool		 path_is_child(const char *, const char *, size_t);
static int		 path_skip_common_ancestor(char **, const char *,
			    size_t, const char *, size_t);
static bool		 fnc_path_is_root_dir(const char *);
/* static bool		 fnc_path_is_cwd(const char *); */
static int		 fnc_pathlist_insert(struct fnc_pathlist_entry **,
			    struct fnc_pathlist_head *, const char *, void *);
static int		 fnc_path_cmp(const char *, const char *, size_t,
			    size_t);
static void		 fnc_pathlist_free(struct fnc_pathlist_head *);
static int		 browse_commit_tree(struct fnc_view **, int,
			    struct commit_entry *, const char *);
static int		 open_tree_view(struct fnc_view *, const char *,
			    fsl_id_t);
static int		 walk_tree_path(struct fnc_tree_view_state *,
			    struct fnc_repository_tree *,
			    struct fnc_tree_object **, const char *);
static int		 create_repository_tree(struct fnc_repository_tree **,
			    fsl_uuid_str *, fsl_id_t);
static int		 tree_builder(struct fnc_repository_tree *,
			    struct fnc_tree_object **, const char *);
/* static void		 delete_tree_node(struct fnc_tree_entry **, */
/*			    struct fnc_tree_entry *); */
static int		 link_tree_node(struct fnc_repository_tree *,
			    const char *, const char *, double);
static int		 show_tree_view(struct fnc_view *);
static int		 tree_input_handler(struct fnc_view **,
			    struct fnc_view *, int);
static int		 blame_tree_entry(struct fnc_view **, int,
			    struct fnc_tree_entry *, struct fnc_parent_trees *,
			    fsl_uuid_str);
static int		 tree_search_init(struct fnc_view *);
static int		 tree_search_next(struct fnc_view *);
static int		 tree_entry_path(char **, struct fnc_parent_trees *,
			    struct fnc_tree_entry *);
static int		 draw_tree(struct fnc_view *, const char *);
static int		 timeline_tree_entry(struct fnc_view **, int,
			    struct fnc_tree_view_state *);
static void		 tree_scroll_up(struct fnc_tree_view_state *, int);
static void		 tree_scroll_down(struct fnc_tree_view_state *, int);
static int		 visit_subtree(struct fnc_tree_view_state *,
			    struct fnc_tree_object *);
static int		 tree_entry_get_symlink_target(char **,
			    struct fnc_tree_entry *);
static int		 match_tree_entry(struct fnc_tree_entry *, regex_t *);
static void		 fnc_object_tree_close(struct fnc_tree_object *);
static void		 fnc_close_repository_tree(struct fnc_repository_tree *);
static int		 open_blame_view(struct fnc_view *, char *,
			    fsl_uuid_str, fsl_id_t, int);
static int		 run_blame(struct fnc_view *);
static int		 fnc_dump_buffer_to_file(off_t *, int *, off_t **,
			    FILE *, fsl_buffer *);
static int		 show_blame_view(struct fnc_view *);
static void		*blame_thread(void *);
static int		 blame_cb(void *, fsl_annotate_opt const * const,
			    fsl_annotate_step const * const);
static int		 draw_blame(struct fnc_view *);
static int		 blame_input_handler(struct fnc_view **,
			    struct fnc_view *, int);
static int		 blame_search_init(struct fnc_view *);
static int		 blame_search_next(struct fnc_view *);
static fsl_uuid_cstr	 get_selected_commit_id(struct fnc_blame_line *,
			    int, int, int);
static int		 fnc_commit_qid_alloc(struct fnc_commit_qid **,
			    fsl_uuid_cstr);
static int		 close_blame_view(struct fnc_view *);
static int		 stop_blame(struct fnc_blame *);
static int		 cancel_blame(void *);
static void		 fnc_commit_qid_free(struct fnc_commit_qid *);
static int		 fnc_load_branches(struct fnc_branch_view_state *);
static int		 create_tmp_branchlist_table(void);
static int		 alloc_branch(struct fnc_branch **, const char *,
			    double, bool, bool, bool);
static int		 fnc_branchlist_insert(struct fnc_branchlist_entry **,
			    struct fnc_branchlist_head *, struct fnc_branch *);
static int		 open_branch_view(struct fnc_view *, int, const char *,
			    double, int);
static int		 show_branch_view(struct fnc_view *);
static int		 branch_input_handler(struct fnc_view **,
			    struct fnc_view *, int);
static int		 tl_branch_entry(struct fnc_view **, int,
			    struct fnc_branchlist_entry *);
static int		 browse_branch_tree(struct fnc_view **, int,
			    struct fnc_branchlist_entry *);
static void		 branch_scroll_up(struct fnc_branch_view_state *, int);
static void		 branch_scroll_down(struct fnc_branch_view_state *,
			    int);
static int		 branch_search_next(struct fnc_view *);
static int		 branch_search_init(struct fnc_view *);
static int		 match_branchlist_entry(struct fnc_branchlist_entry *,
			    regex_t *);
static int		 close_branch_view(struct fnc_view *);
static void		 fnc_free_branches(struct fnc_branchlist_head *);
static void		 fnc_branch_close(struct fnc_branch *);
static int		 view_is_parent(struct fnc_view *);
static void		 view_set_child(struct fnc_view *, struct fnc_view *);
static int		 view_close_child(struct fnc_view *);
static int		 close_tree_view(struct fnc_view *);
static int		 close_timeline_view(struct fnc_view *);
static int		 close_diff_view(struct fnc_view *);
static int		 view_resize(struct fnc_view *);
static bool		 screen_is_split(struct fnc_view *);
static bool		 screen_is_shared(struct fnc_view *);
static void		 fnc_resizeterm(void);
static int		 join_tl_thread(struct fnc_tl_view_state *);
static void		 fnc_free_commits(struct commit_queue *);
static void		 fnc_commit_artifact_close(struct fnc_commit_artifact*);
static int		 fsl_file_artifact_free(void *, void *);
static void		 sigwinch_handler(int);
static void		 sigpipe_handler(int);
static void		 sigcont_handler(int);
static int		 strtonumcheck(long *, const char *, const int,
			    const int);
static int		 fnc_date_to_mtime(double *, const char *, int);
static char		*fnc_strsep (char **, const char *);
static bool		 fnc_str_has_upper(const char *);
static int		 fnc_make_sql_glob(char **, char **, const char *, bool);
#ifdef __OpenBSD__
static int		 init_unveil(const char *, const char *, bool);
#endif
static int		 set_colours(struct fnc_colours *, enum fnc_view_id);
static int		 set_colour_scheme(struct fnc_colours *,
			    const int (*)[2], const char **, int);
static int		 init_colour(enum fnc_opt_id);
static int		 default_colour(enum fnc_opt_id);
static void		 free_colours(struct fnc_colours *);
static bool		 fnc_home(struct fnc_view *);
static char		*fnc_conf_getopt(enum fnc_opt_id, bool);
static int		 fnc_conf_setopt(enum fnc_opt_id, const char *, bool);
static int		 fnc_conf_lsopt(bool);
static enum fnc_opt_id	 fnc_conf_str2enum(const char *);
static const char	*fnc_conf_enum2str(enum fnc_opt_id);
static struct fnc_colour	*get_colour(struct fnc_colours *, int);
static struct fnc_colour	*match_colour(struct fnc_colours *,
				    const char *);
static struct fnc_tree_entry	*get_tree_entry(struct fnc_tree_object *,
				    int);
static struct fnc_tree_entry	*find_tree_entry(struct fnc_tree_object *,
				    const char *, size_t);

int
main(int argc, const char **argv)
{
	fcli_command	*cmd = NULL;
	char		*path = NULL;
	int		 rc = 0;

	if (!setlocale(LC_CTYPE, ""))
		fsl_fprintf(stderr, "[!] Warning: Can't set locale.\n");

	fnc_init.cmdarg = argv[1];	/* Which cmd to show usage if needed. */
#if DEBUG
	fcli.clientFlags.verbose = 2;	/* Verbose error reporting. */
#endif
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
		/* NOT REACHED */

#ifdef __OpenBSD__
	/*
	 * See pledge(2). This is the most restrictive set we can operate under.
	 * Look for any adverse impact & revise when implementing new features.
	 * stdio (close, sigaction); rpath (chdir getcwd lstat); wpath (getcwd);
	 * cpath (symlink); flock (open); tty (TIOCGWINSZ); unveil (unveil).
	 */
	if (pledge("stdio rpath wpath cpath flock tty unveil", NULL) == -1) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_ACCESS), "%s", "pledge");
		goto end;
	}
#endif
	rc = fcli_fingerprint_check(true);
	if (rc)
		goto end;

	if (argc == 1)
		cmd = &fnc_init.cmd_args[FNC_VIEW_TIMELINE];
	else {
		rc = fcli_dispatch_commands(fnc_init.cmd_args, false);
		if (rc == FSL_RC_NOT_FOUND && argc == 2) {
			/*
			 * Check if user entered fnc path/in/repo; if valid path
			 * is found, assume fnc timeline path/in/repo was meant.
			 */
			rc = map_repo_path(&path);
			if (rc == FSL_RC_NOT_FOUND || !path) {
				rc = RC(rc,
				    "'%s' is not a valid command or path",
				    argv[1]);
				fnc_init.err = rc;
				usage();
				/* NOT REACHED */
			} else if (rc)
				goto end;
			cmd = &fnc_init.cmd_args[FNC_VIEW_TIMELINE];
			fnc_init.path = path;
			fcli_err_reset(); /* cmd_timeline::fcli_process_flags */
		} else if (rc)
			goto end;
	}

	if ((rc = fcli_has_unused_args(false))) {
		fnc_init.err = rc;
		usage();
		/* NOT REACHED */
	}

	if (!fsl_cx_db_repo(fcli_cx())) {
		rc = RC(FSL_RC_MISUSE, "%s", "repository database required");
		goto end;
	}

	if (cmd != NULL)
		rc = cmd->f(cmd);
end:
	fsl_free(path);
	endwin();
	if (rc) {
		if (rc == FCLI_RC_HELP)
			rc = 0;
		else if (rc == FSL_RC_BREAK) {
			const fsl_cx *const f = fcli_cx();
			const char *errstr;
			fsl_error_get(&f->error, &errstr, NULL);
			fsl_fprintf(stdout, "%s", errstr);
			fcli_err_reset();  /* For fcli_end_of_main() */
			rc = 0;
		}
	}
	putchar('\n');
	return fcli_end_of_main(rc);
}

static int
cmd_timeline(fcli_command const *argv)
{
	struct fnc_view	*v;
	fsl_cx		*const f = fcli_cx();
	char		*glob = NULL, *path = NULL;
	fsl_id_t	 rid = 0;
	int		 rc = 0;

	rc = fcli_process_flags(argv->flags);
	if (rc || (rc = fcli_has_unused_flags(false)))
		return rc;

	if (fnc_init.nrecords.zlimit)
		if ((rc = strtonumcheck(&fnc_init.nrecords.limit,
		    fnc_init.nrecords.zlimit, INT_MIN, INT_MAX)))
			return rc;

	if (fnc_init.sym != NULL) {
		rc = fsl_sym_to_rid(f, fnc_init.sym, FSL_SATYPE_CHECKIN, &rid);
		if (rc || !rid)
			return RC(FSL_RC_TYPE,
			    "artifact [%s] not resolvable to a commit",
			    fnc_init.sym);
	}

	if (fnc_init.glob)
		glob = fsl_strdup(fnc_init.glob);
	if (fnc_init.path)
		path = fsl_strdup(fnc_init.path);
	else {
		rc = map_repo_path(&path);
		if (rc)
			goto end;
	}

	rc = init_curses();
	if (rc)
		goto end;
#ifdef __OpenBSD__
	rc = init_unveil(fsl_cx_db_file_repo(f, NULL),
	    fsl_cx_ckout_dir_name(f, NULL), false);
	if (rc)
		goto end;
#endif

	v = view_open(0, 0, 0, 0, FNC_VIEW_TIMELINE);
	if (v == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "view_open");
		goto end;
	}
	rc = open_timeline_view(v, rid, path, glob);
	if (!rc)
		rc = view_loop(v);
end:
	fsl_free(glob);
	fsl_free(path);
	return rc;
}

/*
 * Look for an in-repository path in **argv. If found, canonicalise it as an
 * absolute path relative to the repository root (e.g., /ckoutdir/found/path),
 * and assign to a dynamically allocated string in *requested_path, which the
 * caller must dispose of with fsl_free or free(3).
 */
static int
map_repo_path(char **requested_path)
{
	fsl_cx		*const f = fcli_cx();
	fsl_buffer	 buf = fsl_buffer_empty;
	char		*canonpath = NULL, *ckoutdir = NULL, *path = NULL;
	const char	*ckoutdir0 = NULL;
	fsl_size_t	 len;
	int		 rc = 0;
	bool		 root;

	*requested_path = NULL;

	/* If no path argument is supplied, default to repository root. */
	if (!fcli_next_arg(false)) {
		*requested_path = fsl_strdup("/");
		if (*requested_path == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		return rc;
	}

	canonpath = fsl_strdup(fcli_next_arg(true));
	if (canonpath == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		goto end;
	}

	/*
	 * If no checkout (e.g., 'fnc timeline -R') copy the path verbatim to
	 * check its validity against a deck of F cards in open_timeline_view().
	 */
	ckoutdir0 = fsl_cx_ckout_dir_name(f, &len);
	if (!ckoutdir0) {
		path = fsl_strdup(canonpath);
		goto end;
	}

	path = realpath(canonpath, NULL);
	if (path == NULL && (errno == ENOENT || errno == ENOTDIR)) {
		/* Path is not on disk, assume it is relative to repo root. */
		rc = fsl_file_canonical_name2(ckoutdir0, canonpath, &buf, NULL);
		if (rc) {
			rc = RC(rc, "%s", "fsl_file_canonical_name2");
			goto end;
		}
		fsl_free(path);
		path = realpath(fsl_buffer_cstr(&buf), NULL);
		if (path) {
			/* Confirmed path is relative to repository root. */
			fsl_free(path);
			path = fsl_strdup(canonpath);
			if (path == NULL)
				rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		} else {
			rc = RC(fsl_errno_to_rc(errno, FSL_RC_NOT_FOUND),
			    "'%s' not found in tree", canonpath);
			*requested_path = fsl_strdup(canonpath);
		}
		goto end;
	}
	fsl_free(path);
	/*
	 * Use the cwd as the virtual root to canonicalise the supplied path if
	 * it is either: (a) relative; or (b) the root of the current checkout.
	 * Otherwise, use the root of the current checkout.
	 */
	rc = fsl_cx_getcwd(f, &buf);
	if (rc)
		goto end;
	ckoutdir = fsl_mprintf("%.*s", len - 1, ckoutdir0);
	root = fsl_strcmp(ckoutdir, fsl_buffer_cstr(&buf)) == 0;
	fsl_buffer_reuse(&buf);
	rc = fsl_ckout_filename_check(f, (canonpath[0] == '.' || !root) ?
	    true : false, canonpath, &buf);
	if (rc)
		goto end;
	fsl_free(canonpath);
	canonpath = fsl_strdup(fsl_buffer_str(&buf));

	if (canonpath[0] == '\0') {
		path = fsl_strdup(canonpath);
		if (path == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
		}
	} else {
		fsl_buffer_reuse(&buf);
		rc = fsl_file_canonical_name2(f->ckout.dir, canonpath, &buf,
		    false);
		if (rc)
			goto end;
		path = fsl_strdup(fsl_buffer_str(&buf));
		if (path == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
		}
		if (access(path, F_OK) != 0) {
			rc = RC(fsl_errno_to_rc(errno, FSL_RC_ACCESS),
			    "path does not exist or inaccessible [%s]", path);
			goto end;
		}
		/*
		 * Now we have an absolute path, check again if it's the ckout
		 * dir; if so, clear it to signal an open_timeline_view() check.
		 */
		len = fsl_strlen(path);
		if (!fsl_strcmp(path, f->ckout.dir)) {
			fsl_free(path);
			path = fsl_strdup("");
			if (path == NULL) {
				rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
				goto end;
			}
		} else if (len > f->ckout.dirLen && path_is_child(path,
		    f->ckout.dir, f->ckout.dirLen)) {
			char *child;
			/*
			 * Matched on-disk path within the repository; strip
			 * common prefix with repository root path.
			 */
			rc = path_skip_common_ancestor(&child, f->ckout.dir,
			    f->ckout.dirLen, path, len);
			if (rc)
				goto end;
			fsl_free(path);
			path = child;
		} else {
			/*
			 * Matched on-disk path outside the repository; treat
			 * as relative to repo root. (Though this should fail.)
			 */
			fsl_free(path);
			path = canonpath;
			canonpath = NULL;
		}
	}

	/* Trim trailing slash if it exists. */
	if (path[fsl_strlen(path) - 1] == '/')
		path[fsl_strlen(path) - 1] = '\0';

end:
	fsl_buffer_clear(&buf);
	fsl_free(canonpath);
	fsl_free(ckoutdir);
	if (rc)
		fsl_free(path);
	else {
		/* Make path absolute from repository root. */
		if (path[0] != '/' && (path[0] != '.' && path[1] != '/')) {
			char *abspath;
			if ((abspath = fsl_mprintf("/%s", path)) == NULL) {
				rc = RC(FSL_RC_ERROR, "%s", "fsl_mprintf");
				goto end;
			}
			fsl_free(path);
			path = abspath;
		}

		*requested_path = path;
	}
	return rc;
}

static bool
path_is_child(const char *child, const char *parent, size_t parentlen)
{
	if (parentlen == 0 || fnc_path_is_root_dir(parent))
		return true;

	if (fsl_strncmp(parent, child, parentlen) != 0)
		return false;
	if (child[parentlen - 1 /* Trailing slash */] != '/')
		return false;

	return true;
}

/*
 * As a special case, due to fsl_ckout_filename_check() resolving the current
 * checkout directory to ".", this function returns true for ".". For this
 * reason, when path is intended to be the current working directory for any
 * directory other than the repository root, callers must ensure path is either
 * absolute or relative to the respository root--not ".".
 */
static bool
fnc_path_is_root_dir(const char *path)
{
	while (*path == '/' || *path == '.')
		++path;
	return (*path == '\0');
}

static int
path_skip_common_ancestor(char **child, const char *parent_abspath,
    size_t parentlen, const char *abspath, size_t len)
{
	size_t	bufsz;
	int	rc = 0;

	*child = NULL;

	if (parentlen >= len)
		return RC(FSL_RC_RANGE, "invalid path [%s]", abspath);
	if (fsl_strncmp(parent_abspath, abspath, parentlen) != 0)
		return RC(FSL_RC_TYPE, "invalid path [%s]", abspath);
	if (!fnc_path_is_root_dir(parent_abspath) &&
	    abspath[parentlen - 1 /* Trailing slash */] != '/')
		return RC(FSL_RC_TYPE, "invalid path [%s]", abspath);
	while (abspath[parentlen] == '/')
		++abspath;
	bufsz = len - parentlen + 1;
	*child = fsl_malloc(bufsz);
	if (*child == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_malloc");
	if (strlcpy(*child, abspath + parentlen, bufsz) >= bufsz) {
		rc = RC(FSL_RC_RANGE, "%s", "strlcpy");
		fsl_free(*child);
		*child = NULL;
	}
	return rc;
}

#if 0
static bool
fnc_path_is_cwd(const char *path)
{
	return (path[0] == '.' && path[1] == '\0');
}
#endif

static int
init_curses(void)
{
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	curs_set(0);
	set_escdelay(0);  /* ESC should return immediately. */
#ifndef __linux__
	typeahead(-1);	/* Don't disrupt screen update operations. */
#endif

	if (!fnc_init.nocolour && has_colors()) {
		start_color();
		use_default_colors();
	}

	if (sigaction(SIGPIPE, &(struct sigaction){{sigpipe_handler}}, NULL)
	    == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "%s", "sigaction(SIGPIPE)");
	if (sigaction(SIGWINCH, &(struct sigaction){{sigwinch_handler}}, NULL)
	    == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "%s", "sigaction(SIGWINCH)");
	if (sigaction(SIGCONT, &(struct sigaction){{sigcont_handler}}, NULL)
	    == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "%s", "sigaction(SIGCONT)");

	return 0;
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
open_timeline_view(struct fnc_view *view, fsl_id_t rid, const char *path,
    const char *glob)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	fsl_cx				*const f = fcli_cx();
	fsl_db				*db = fsl_cx_db_repo(f);
	fsl_buffer			 sql = fsl_buffer_empty;
	char				*startdate = NULL;
	char				*op = NULL, *str = NULL;
	fsl_id_t			 idtag = 0;
	int				 idx, rc = 0;

	f->clientState.state = &s->thread_cx;

	if (path != s->path) {
		fsl_free(s->path);
		s->path = fsl_strdup(path);
		if (s->path == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
	}

	/*
	 * TODO: See about opening this API.
	 * If a path has been supplied, create a table of all path's
	 * ancestors and add "AND blob.rid IN fsl_computed_ancestors" to query.
	 */
	/* if (path[1]) { */
	/*	rc = fsl_compute_ancestors(db, rid, 0, 0); */
	/*	if (rc) */
	/*		return RC(FSL_RC_DB, "%s", "fsl_compute_ancestors"); */
	/* } */
	s->thread_cx.q = NULL;
	/* s->selected_idx = 0; */	/* Unnecessary? */

	TAILQ_INIT(&s->commits.head);
	s->commits.ncommits = 0;

	if (rid)
		startdate = fsl_mprintf("(SELECT mtime FROM event "
		    "WHERE objid=%d)", rid);
	else
		fsl_ckout_version_info(f, NULL, &s->curr_ckout_uuid);

	/*
	 * In 'fnc timeline -R repo.fossil path' case, check that path is a
	 * valid repository path in the repository tree as at either the
	 * latest check-in or the specified commit.
	 */
	if (s->curr_ckout_uuid == NULL && path[1]) {
		fsl_deck d = fsl_deck_empty;
		fsl_uuid_str id = NULL;
		bool ispath = false;
		if (rid)
			id = fsl_rid_to_uuid(f, rid);
		rc = fsl_deck_load_sym(f, &d, fnc_init.sym ? fnc_init.sym :
		    id ? id : "tip", FSL_SATYPE_CHECKIN);
		fsl_deck_F_rewind(&d);
		if (fsl_deck_F_search(&d, path + 1 /* Slash */) == NULL) {
			const fsl_card_F *cf;
			fsl_deck_F_next(&d, &cf);
			do {
				fsl_deck_F_next(&d, &cf);
				if (cf && !fsl_strncmp(path + 1 /* Slash */,
				    cf->name, fsl_strlen(path) - 1)) {
					ispath = true;
					break;
				}
			} while (cf);
		} else
			ispath = true;
		fsl_deck_finalize(&d);
		fsl_free(id);
		if (!ispath)
			return RC(FSL_RC_NOT_FOUND, "'%s' invalid path in [%s]",
			    path + 1, fnc_init.sym ? fnc_init.sym : "tip");
	}

	if ((rc = pthread_cond_init(&s->thread_cx.commit_consumer, NULL))) {
		RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_cond_init");
		goto end;
	}
	if ((rc = pthread_cond_init(&s->thread_cx.commit_producer, NULL))) {
		RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_cond_init");
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

	if (fnc_init.filter_types.nitems) {
		fsl_buffer_appendf(&sql, " AND (");
		for (idx = 0; idx < fnc_init.filter_types.nitems; ++idx)
			fsl_buffer_appendf(&sql, " eventtype=%Q%s",
			    fnc_init.filter_types.values[idx], (idx + 1) <
			    fnc_init.filter_types.nitems ? " OR " : ")");
	}

	if (fnc_init.filter_branch) {
		rc = fnc_make_sql_glob(&op, &str, fnc_init.filter_branch,
		    !fnc_str_has_upper(fnc_init.filter_branch));
		if (rc)
			goto end;
		idtag = fsl_db_g_id(db, 0,
		    "SELECT tagid FROM tag WHERE tagname %q 'sym-%q'"
		    " ORDER BY tagid DESC", op, str);
		if (idtag) {
			rc = fsl_buffer_appendf(&sql,
			    " AND EXISTS(SELECT 1 FROM tagxref"
			    " WHERE tagid=%"FSL_ID_T_PFMT
			    " AND tagtype > 0 AND rid=blob.rid)", idtag);
			if (rc)
				goto end;
		} else {
			rc = RC(FSL_RC_NOT_FOUND, "branch not found: %s",
			    fnc_init.filter_branch);
			goto end;
		}
	}

	if (fnc_init.filter_tag) {
		/* Lookup non-branch tag first; if not found, lookup branch. */
		rc = fnc_make_sql_glob(&op, &str, fnc_init.filter_tag,
		    !fnc_str_has_upper(fnc_init.filter_tag));
		if (rc)
			goto end;
		idtag = fsl_db_g_id(db, 0,
		    "SELECT tagid FROM tag WHERE tagname %q '%q'"
		    " ORDER BY tagid DESC", op, str);
		if (idtag == 0)
			idtag = fsl_db_g_id(db, 0,
			    "SELECT tagid FROM tag WHERE tagname %q 'sym-%q'"
			    " ORDER BY tagid DESC", op, str);
		if (idtag) {
			rc = fsl_buffer_appendf(&sql,
			    " AND EXISTS(SELECT 1 FROM tagxref"
			    " WHERE tagid=%"FSL_ID_T_PFMT
			    " AND tagtype > 0 AND rid=blob.rid)", idtag);
			if (rc)
				goto end;
		} else {
			rc = RC(FSL_RC_NOT_FOUND, "tag not found: %s",
			    fnc_init.filter_tag);
			goto end;
		}
	}

	if (fnc_init.filter_user) {
		rc = fnc_make_sql_glob(&op, &str, fnc_init.filter_user,
		    !fnc_str_has_upper(fnc_init.filter_user));
		if (rc)
			goto end;
		rc = fsl_buffer_appendf(&sql,
		    " AND coalesce(euser, user) %q '%q'", op, str);
		if (rc)
			goto end;
	}

	if (glob) {
		/* Filter commits on comment, user, and branch name. */
		rc = fnc_make_sql_glob(&op, &str, glob,
		    !fnc_str_has_upper(glob));
		if (rc)
			goto end;
		idtag = fsl_db_g_id(db, 0,
		    "SELECT tagid FROM tag WHERE tagname %q 'sym-%q'"
		    " ORDER BY tagid DESC", op, str);
		rc = fsl_buffer_appendf(&sql,
		    " AND (coalesce(ecomment, comment) %q %Q"
		    " OR coalesce(euser, user) %q %Q%c",
		    op, str, op, str, idtag ? ' ' : ')');
		if (!rc && idtag > 0)
			rc = fsl_buffer_appendf(&sql,
			    " OR EXISTS(SELECT 1 FROM tagxref"
			    " WHERE tagid=%"FSL_ID_T_PFMT
			    " AND tagtype > 0 AND rid=blob.rid))", idtag);
		if (rc)
			goto end;
	}

	if (startdate) {
		fsl_buffer_appendf(&sql, " AND event.mtime <= %s", startdate);
		fsl_free(startdate);
	}

	/*
	 * If path is not root ("/"), a versioned path in the repository has
	 * been requested, only retrieve commits involving path.
	 */
	if (path[1]) {
		fsl_buffer_appendf(&sql,
		    " AND EXISTS(SELECT 1 FROM mlink"
		    " WHERE mlink.mid = event.objid"
		    " AND mlink.fnid IN ");
		if (fsl_cx_is_case_sensitive(f)) {
			fsl_buffer_appendf(&sql,
			    "(SELECT fnid FROM filename"
			    " WHERE name = %Q OR name GLOB '%q/*')",
			    path + 1, path + 1);  /* Skip prepended slash. */
		} else {
			fsl_buffer_appendf(&sql,
			    "(SELECT fnid FROM filename"
			    " WHERE name = %Q COLLATE nocase"
			    " OR lower(name) GLOB lower('%q/*'))",
			    path + 1, path + 1);  /* Skip prepended slash. */
		}
		fsl_buffer_append(&sql, ")", 1);
	}

	fsl_buffer_appendf(&sql, " ORDER BY event.mtime DESC");

	if (fnc_init.nrecords.limit > 0)
		fsl_buffer_appendf(&sql, " LIMIT %d", fnc_init.nrecords.limit);

	view->show = show_timeline_view;
	view->input = tl_input_handler;
	view->close = close_timeline_view;
	view->search_init = tl_search_init;
	view->search_next = tl_search_next;

	s->thread_cx.q = fsl_stmt_malloc();
	rc = fsl_db_prepare(db, s->thread_cx.q, "%b", &sql);
	if (rc) {
		rc = RC(rc, "%s", "fsl_db_prepare");
		goto end;
	}
	rc = fsl_stmt_step(s->thread_cx.q);
	switch (rc) {
	case FSL_RC_STEP_ROW:
		rc = 0;
		break;
	case FSL_RC_STEP_ERROR:
		rc = RC(rc, "%s", "fsl_stmt_step");
		goto end;
	case FSL_RC_STEP_DONE:
		rc = RC(FSL_RC_BREAK, "%s", "no matching records");
		goto end;
	}

	s->colour = !fnc_init.nocolour && has_colors();
	s->thread_cx.rc = 0;
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
	s->thread_cx.path = s->path;
	s->thread_cx.needs_reset = false;

	if (s->colour) {
		STAILQ_INIT(&s->colours);
		rc = set_colours(&s->colours, FNC_VIEW_TIMELINE);
	}
end:
	fsl_buffer_clear(&sql);
	fsl_free(op);
	fsl_free(str);
	if (rc) {
		if (view->close)
			view_close(view);
		else
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
	int			 done = 0, err = 0, rc = 0;

	if ((rc = pthread_mutex_lock(&fnc_mutex)))
		return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_lock");

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
				/* Restore fullscreen line height. */
				view->parent->nlines = view->parent->lines;
				rc = view_resize(view->parent);
				if (rc)
					goto end;
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

	if ((err = pthread_mutex_unlock(&fnc_mutex)) && !rc)
		rc = RC(fsl_errno_to_rc(err, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_unlock");

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
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_create");
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
			if (rc) {
				cx->rc = rc;
				return (void *)(intptr_t)rc;
			}
			else if (cx->ncommits_needed > 0)
				cx->ncommits_needed--;
			break;
		}

		if ((rc = pthread_mutex_lock(&fnc_mutex))) {
			rc = RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_lock");
			break;
		} else if (*cx->first_commit_onscreen == NULL) {
			*cx->first_commit_onscreen =
			    TAILQ_FIRST(&cx->commits->head);
			*cx->selected_commit = *cx->first_commit_onscreen;
		} else if (*cx->quit)
			done = true;

		if ((rc = pthread_cond_signal(&cx->commit_producer))) {
			rc = RC(fsl_errno_to_rc(rc, FSL_RC_MISUSE),
			    "%s", "pthread_cond_signal");
			pthread_mutex_unlock(&fnc_mutex);
			break;
		}

		if (done)
			cx->ncommits_needed = 0;
		else if (cx->ncommits_needed == 0) {
			if ((rc = pthread_cond_wait(&cx->commit_consumer,
			    &fnc_mutex)))
				rc = RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
				    "%s", "pthread_cond_wait");
			if (*cx->quit)
				done = true;
		}

		if ((rc = pthread_mutex_unlock(&fnc_mutex)))
			rc = RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_unlock");
	}

	cx->timeline_end = true;
	return (void *)(intptr_t)rc;
}

static int
block_main_thread_signals(void)
{
	sigset_t	 set;

	if (sigemptyset(&set) == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_MISUSE), "%s",
		    "sigemptyset");

	/* Bespoke signal handlers for SIGWINCH and SIGCONT. */
	if (sigaddset(&set, SIGWINCH) == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_MISUSE), "%s",
		    "sigaddset");
	if (sigaddset(&set, SIGCONT) == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_MISUSE), "%s",
		    "sigaddset");

	/* ncurses handles SIGTSTP. */
	if (sigaddset(&set, SIGTSTP) == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_MISUSE), "%s",
		    "sigaddset");

	if (pthread_sigmask(SIG_BLOCK, &set, NULL))
		return RC(fsl_errno_to_rc(errno, FSL_RC_MISUSE), "%s",
		    "pthread_sigmask");

	return 0;
}

static int
build_commits(struct fnc_tl_thread_cx *cx)
{
	int	rc = 0;

	if (cx->needs_reset) {
		/*
		 * XXX If a {tree,branch} view has been opened with the '{t,b}'
		 * key binding, there may be cached statements that necessitate
		 * the commit builder statement being reset otherwise one of the
		 * SQLite3 APIs down the fsl_stmt_step() call stack fails. This
		 * is irrespective of whether fsl_db_prepare_cached() was used.
		 */
		fsl_size_t loaded = cx->commits->ncommits + 1;
		cx->needs_reset = false;
		rc = fsl_stmt_reset(cx->q);
		if (rc)
			return RC(rc, "%s", "fsl_stmt_reset");
		while (loaded--)
			if ((rc = fsl_stmt_step(cx->q)) != FSL_RC_STEP_ROW)
				return RC(rc, "%s", "fsl_stmt_step");
	}
	/*
	 * Step through the given SQL query, passing each row to the commit
	 * builder to build commits for the timeline.
	 */
	do {
		struct fnc_commit_artifact	*commit = NULL;
		struct commit_entry		*dup_entry, *entry;

		rc = commit_builder(&commit, 0, cx->q);
		if (rc)
			return RC(rc, "%s", "commit_builder");
		/*
		 * TODO: Find out why, without this, fnc reads and displays
		 * the first (i.e., latest) commit twice. This hack checks to
		 * see if the current row returned a UUID matching the last
		 * commit added to the list to avoid adding a duplicate entry.
		 */
		dup_entry = TAILQ_FIRST(&cx->commits->head);
		if (cx->commits->ncommits == 1 &&
		    !fsl_strcmp(dup_entry->commit->uuid, commit->uuid)) {
			fnc_commit_artifact_close(commit);
			cx->ncommits_needed++;
			continue;
		}

		entry = fsl_malloc(sizeof(*entry));
		if (entry == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_malloc");

		entry->commit = commit;

		rc = pthread_mutex_lock(&fnc_mutex);
		if (rc)
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_lock");

		entry->idx = cx->commits->ncommits;
		TAILQ_INSERT_TAIL(&cx->commits->head, entry, entries);
		cx->commits->ncommits++;

		if (!cx->endjmp && *cx->searching == SEARCH_FORWARD &&
		    *cx->search_status == SEARCH_WAITING) {
			if (find_commit_match(commit, cx->regex))
				*cx->search_status = SEARCH_CONTINUE;
		}

		rc = pthread_mutex_unlock(&fnc_mutex);
		if (rc)
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_unlock");

	} while ((rc = fsl_stmt_step(cx->q)) == FSL_RC_STEP_ROW
	    && *cx->searching == SEARCH_FORWARD
	    && *cx->search_status == SEARCH_WAITING);

	return rc;
}

/*
 * Given prepared SQL statement q _XOR_ record ID rid, allocate and build the
 * corresponding commit artifact from the result set. The commit must
 * eventually be disposed of with fnc_commit_artifact_close().
 */
static int
commit_builder(struct fnc_commit_artifact **ptr, fsl_id_t rid, fsl_stmt *q)
{
	fsl_cx				*const f = fcli_cx();
	fsl_db				*db = fsl_needs_repo(f);
	struct fnc_commit_artifact	*commit = NULL;
	fsl_buffer			 buf = fsl_buffer_empty;
	const char			*comment, *prefix, *type;
	int				 rc = 0;
	enum fnc_diff_type		 diff_type = FNC_DIFF_WIKI;

	if (rid) {
		rc = fsl_db_prepare(db, q, "SELECT "
		    /* 0 */"uuid, "
		    /* 1 */"datetime(event.mtime%s), "
		    /* 2 */"coalesce(euser, user), "
		    /* 3 */"rid AS rid, "
		    /* 4 */"event.type AS eventtype, "
		    /* 5 */"(SELECT group_concat(substr(tagname,5), ',') "
		    "FROM tag, tagxref WHERE tagname GLOB 'sym-*' "
		    "AND tag.tagid=tagxref.tagid AND tagxref.rid=blob.rid "
		    "AND tagxref.tagtype > 0) as tags, "
		    /*6*/"coalesce(ecomment, comment) AS comment "
		    "FROM event JOIN blob WHERE blob.rid=%d AND event.objid=%d",
		    fnc_init.utc ? "" : ", 'localtime'", rid, rid);
		if (rc)
			return RC(FSL_RC_DB, "%s", "fsl_db_prepare");
		fsl_stmt_step(q);
	}

	type = fsl_stmt_g_text(q, 4, NULL);
	comment = fsl_stmt_g_text(q, 6, NULL);
	prefix = NULL;

	switch (*type) {
	case 'c':
		type = "checkin";
		diff_type = FNC_DIFF_COMMIT;
		break;
	case 'w':
		type = "wiki";
		if (comment) {
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
			if (prefix)
				rc = fsl_buffer_append(&buf, prefix, -1);
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
	if (!rc && comment)
		rc = fsl_buffer_append(&buf, comment, -1);
	if (rc) {
		rc = RC(rc, "%s", "fsl_buffer_append");
		goto end;
	}

	commit = calloc(1, sizeof(*commit));
	if (commit == NULL) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_ERROR), "%s", "calloc");
		goto end;
	}

	if (!rid && (rc = fsl_stmt_get_id(q, 3, &rid))) {
		rc = RC(rc, "%s", "fsl_stmt_get_id");
		goto end;
	}
	/* Is there a more efficient way to get the parent? */
	commit->puuid = fsl_db_g_text(db, NULL,
	    "SELECT uuid FROM plink, blob WHERE plink.cid=%d "
	    "AND blob.rid=plink.pid AND plink.isprim", rid);
	commit->prid = fsl_uuid_to_rid(f, commit->puuid);
	commit->uuid = fsl_strdup(fsl_stmt_g_text(q, 0, NULL));
	commit->rid = rid;
	commit->type = fsl_strdup(type);
	commit->diff_type = diff_type;
	commit->timestamp = fsl_strdup(fsl_stmt_g_text(q, 1, NULL));
	commit->user = fsl_strdup(fsl_stmt_g_text(q, 2, NULL));
	commit->branch = fsl_strdup(fsl_stmt_g_text(q, 5, NULL));
	commit->comment = fsl_strdup(comment ? fsl_buffer_str(&buf) : "");
	fsl_buffer_clear(&buf);

	*ptr = commit;
end:
	return rc;
}

static int
signal_tl_thread(struct fnc_view *view, int wait)
{
	struct fnc_tl_thread_cx	*cx = &view->state.timeline.thread_cx;
	int			 rc = 0;

	while (cx->ncommits_needed > 0) {
		if (cx->timeline_end)
			break;

		/* Wake timeline thread. */
		if ((rc = pthread_cond_signal(&cx->commit_consumer)))
			return RC(fsl_errno_to_rc(rc, FSL_RC_MISUSE),
			    "%s", "pthread_cond_signal");

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
		if ((rc = pthread_cond_wait(&cx->commit_producer, &fnc_mutex)))
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_cond_wait");

		/* Show status update in timeline view. */
		show_timeline_view(view);
		update_panels();
		doupdate();
	}

	return cx->rc;
}

static int
draw_commits(struct fnc_view *view)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct fnc_tl_thread_cx		*tcx = &s->thread_cx;
	struct commit_entry		*entry = s->selected_commit;
	struct fnc_colour		*c = NULL;
	const char			*search_str = NULL;
	char				*headln = NULL, *idxstr = NULL;
	char				*branch = NULL, *type = NULL;
	char				*uuid = NULL;
	wchar_t				*wcstr;
	int				 ncommits = 0, rc = 0, wstrlen = 0;
	int				 ncols_needed, max_usrlen = -1;

	if (s->selected_commit && !(view->searching != SEARCH_DONE &&
	    view->search_status == SEARCH_WAITING)) {
		uuid = fsl_strdup(s->selected_commit->commit->uuid);
		branch = fsl_strdup(s->selected_commit->commit->branch);
		type = fsl_strdup(s->selected_commit->commit->type);
	}

	if (tcx->ncommits_needed > 0 && !tcx->timeline_end) {
		if ((idxstr = fsl_mprintf(" [%d/%d] %s",
		    entry ? entry->idx + 1 : 0, s->commits.ncommits,
		    (view->searching && !view->search_status) ?
		    "searching..." : "loading...")) == NULL) {
			rc = RC(FSL_RC_RANGE, "%s", "fsl_mprintf");
			goto end;
		}
	} else {
		if (view->searching) {
			if (view->search_status == SEARCH_COMPLETE)
				search_str = "no more matches";
			else if (view->search_status == SEARCH_NO_MATCH)
				search_str = "no matches found";
			else if (view->search_status == SEARCH_WAITING)
				search_str = "searching...";
		}

		if ((idxstr = fsl_mprintf("%s [%d/%d] %s",
		    !fsl_strcmp(uuid, s->curr_ckout_uuid) ? " [current]" : "",
		    entry ? entry->idx + 1 : 0, s->commits.ncommits,
		    search_str ? search_str : (branch ? branch : "")))
		    == NULL) {
			rc = RC(FSL_RC_RANGE, "%s", "fsl_mprintf");
			goto end;
		}
	}
	/*
	 * Compute cols needed to fit all components of the headline to truncate
	 * the hash component if needed. wiki, tag, and ticket artifacts don't
	 * have a branch component, checkins and some technotes do, so add a col
	 * for the space separator. Same applies if search_str is being shown.
	 */
	ncols_needed = fsl_strlen(type) + fsl_strlen(idxstr) + FSL_STRLEN_K256
	    + (!search_str && (!fsl_strcmp(type, "wiki") ||
	    !fsl_strcmp(type, "tag")  || !fsl_strcmp(type, "ticket") ||
	    (!branch && !fsl_strcmp(type, "technote"))) ? 0 : 1);
	/* If a path has been requested, display it in the headline. */
	if (s->path[1]) {
		if ((headln = fsl_mprintf("%s%c%.*s %s%s", type ? type : "",
		    type ? ' ' : SPINNER[tcx->spin_idx], view->ncols <
		    ncols_needed ? view->ncols - (ncols_needed -
		    FSL_STRLEN_K256) : FSL_STRLEN_K256, uuid ? uuid :
		    "........................................",
		    s->path, idxstr)) == NULL) {
			rc = RC(FSL_RC_RANGE, "%s", "fsl_mprintf");
			headln = NULL;
			goto end;
		}
	} else if ((headln = fsl_mprintf("%s%c%.*s%s", type ? type : "", type ?
	    ' ' : SPINNER[tcx->spin_idx], view->ncols < ncols_needed ?
	    view->ncols - (ncols_needed - FSL_STRLEN_K256) : FSL_STRLEN_K256,
	    uuid ? uuid : "........................................", idxstr))
	    == NULL) {
		rc = RC(FSL_RC_RANGE, "%s", "fsl_mprintf");
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
		wattron(view->window, A_REVERSE);
	if (s->colour)
		c = get_colour(&s->colours, FNC_COLOUR_COMMIT);
	if (c)
		wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
	waddwstr(view->window, wcstr);
	while (wstrlen < view->ncols) {
		waddch(view->window, ' ');
		++wstrlen;
	}
	if (c)
		wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
	if (screen_is_shared(view))
		wattroff(view->window, A_REVERSE);
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
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
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
		if (ncommits >= (view->mode == VIEW_SPLIT_HRZN ?
		    view->nlines - 1 : view->lines - 1))
			break;
		if (ncommits == s->selected_idx)
			wattr_on(view->window, A_REVERSE, NULL);
		rc = write_commit_line(view, entry->commit, max_usrlen);
		if (ncommits == s->selected_idx)
			wattr_off(view->window, A_REVERSE, NULL);
		++ncommits;
		s->last_commit_onscreen = entry;
		entry = TAILQ_NEXT(entry, entries);
	}
	drawborder(view);

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
				    ((cols + start_column) % TABSIZE);
			} else {
				width = 1;
				wline[i] = L'.';
			}
			if (cols + width > column_limit)
				break;
			cols += width;
			i++;
		} else {
			rc = RC(FSL_RC_RANGE, "%s", "wcwidth");
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
	int	rc = 0;

	/*
	 * mbstowcs POSIX extension specifies that the number of wchar that
	 * would be written are returned when first arg is a null pointer:
	 * https://en.cppreference.com/w/cpp/string/multibyte/mbstowcs
	 */
	*dstlen = mbstowcs(NULL, src, 0);
	if (*dstlen == (size_t)-1) {
		if (errno == EILSEQ)
			return RC(FSL_RC_RANGE,
			    "invalid multibyte character [%s]", src);
		return RC(FSL_RC_MISUSE, "mbstowcs(%s)", src);
	}


	*dst = NULL;
	*dst = fsl_malloc(sizeof(wchar_t) * (*dstlen + 1));
	if (*dst == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "malloc");
		goto end;
	}

	if (mbstowcs(*dst, src, *dstlen) != *dstlen)
		rc = RC(FSL_RC_SIZE_MISMATCH, "mbstowcs(%s)", src);

end:
	if (rc) {
		fsl_free(*dst);
		*dst = NULL;
		*dstlen = 0;
	}

	return rc;
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
 * When < 110 columns, the (abbreviated 9-character) UUID will be elided.
 */
static int
write_commit_line(struct fnc_view *view, struct fnc_commit_artifact *commit,
    int max_usrlen)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct fnc_colour		*c = NULL;
	wchar_t				*usr_wcstr = NULL, *wcomment = NULL;
	char				*comment0 = NULL, *comment = NULL;
	char				*date = NULL;
	char				*eol = NULL, *pad = NULL, *user = NULL;
	size_t				 i = 0;
	int				 col_pos, ncols_avail, usrlen;
	int				 commentlen, rc = 0;

	/* Trim time component from timestamp for the date field. */
	date = fsl_strdup(commit->timestamp);
	while (!fsl_isspace(date[i++])) {}
	date[i] = '\0';
	col_pos = MIN(view->ncols, ISO8601_DATE_ONLY + 1);
	if (s->colour)
		c = get_colour(&s->colours, FNC_COLOUR_DATE);
	if (c)
		wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
	waddnstr(view->window, date, col_pos);
	if (c)
		wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
	if (col_pos > view->ncols)
		goto end;

	/* If enough columns, write abbreviated commit hash. */
	if (view->ncols >= 110) {
		if (s->colour)
			c = get_colour(&s->colours, FNC_COLOUR_COMMIT);
		if (c)
			wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
		wprintw(view->window, "%.9s ", commit->uuid);
		if (c)
			wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
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
	if (s->colour)
		c = get_colour(&s->colours, FNC_COLOUR_USER);
	if (c)
		wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
	waddwstr(view->window, usr_wcstr);
	pad = fsl_mprintf("%*c",  max_usrlen - usrlen + 2, ' ');
	waddstr(view->window, pad);
	if (c)
		wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
	col_pos += (max_usrlen + 2);
	if (col_pos > view->ncols)
		goto end;

	/* Only show comment up to the first newline character. */
	comment0 = fsl_strdup(commit->comment);
	comment = comment0;
	if (comment == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
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
	int		 ch, rc = 0;

	*new = NULL;

	/* Clear search indicator string. */
	if (view->search_status == SEARCH_COMPLETE ||
	    view->search_status == SEARCH_NO_MATCH)
		view->search_status = SEARCH_CONTINUE;

	if (view->searching && view->search_status == SEARCH_WAITING) {
		if ((rc = pthread_mutex_unlock(&fnc_mutex)))
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_unlock");
		sched_yield();
		if ((rc = pthread_mutex_lock(&fnc_mutex)))
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_lock");
		rc = view->search_next(view);
		return rc;
	}

	nodelay(stdscr, FALSE);
	/* Allow thread to make progress while waiting for input. */
	if ((rc = pthread_mutex_unlock(&fnc_mutex)))
		return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_unlock");
	ch = wgetch(view->window);
	if ((rc = pthread_mutex_lock(&fnc_mutex)))
		return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_lock");

	if (rec_sigwinch || rec_sigcont) {
		fnc_resizeterm();
		rec_sigwinch = 0;
		rec_sigcont = 0;
		TAILQ_FOREACH(v, views, entries) {
			rc = view_resize(v);
			if (rc)
				return rc;
			rc = v->input(new, v, KEY_RESIZE);
			if (rc)
				return rc;
			if (v->child) {
				rc = view_resize(v->child);
				if (rc)
					return rc;
				rc = v->child->input(new, v->child, KEY_RESIZE);
				if (rc)
					return rc;
			}
		}
	}

	switch (ch) {
	case '\t':
		rc = cycle_view(view);
		break;
	case KEY_F(1):
	case 'H':
	case '?':
		help(view);
		break;
	case 'q':
		rc = view->input(new, view, ch);
		view->egress = true;
		break;
	case 'f':
		rc = toggle_fullscreen(new, view);
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
			rc = view->search_next(view);
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
cycle_view(struct fnc_view *view)
{
	int rc = FSL_RC_OK;

	if (view->child) {
		view->active = false;
		view->child->active = true;
		view->focus_child = true;
	} else if (view->parent) {
		view->active = false;
		view->parent->active = true;
		view->parent->focus_child = false;
		if (view->mode == VIEW_SPLIT_HRZN && !screen_is_split(view)) {
			if (view->parent->vid == FNC_VIEW_TIMELINE)
				rc = request_tl_commits(view->parent);
			rc = make_fullscreen(view->parent);
		}
	}

	return rc;
}

static int
toggle_fullscreen(struct fnc_view **new, struct fnc_view *view)
{
	int	rc = FSL_RC_OK;

	if (view_is_parent(view)) {
		if (view->child == NULL)
			return rc;
		if (screen_is_split(view->child)) {
			view->active = false;
			view->child->active = true;
			rc = make_fullscreen(view->child);
		} else
			rc = make_splitscreen(view->child);
		if (rc)
			return rc;
		rc = view->child->input(new, view->child, KEY_RESIZE);
	} else {
		if (screen_is_split(view)) {
			view->parent->active = false;
			view->active = true;
			rc = make_fullscreen(view);
		} else
			rc = make_splitscreen(view);
		if (rc)
			return rc;
		rc = view->input(new, view, KEY_RESIZE);
	}

	return rc;
}

static int
help(struct fnc_view *view)
{
	FILE			*help = NULL;
	char			*title = NULL;
	static const char	*keys[][2] = {
	    {""},
	    {""}, /* Global */
	    {"  H,?,F1           ", "  ❬H❭❬?❭❬F1❭      "},
	    {"  k,<Up>           ", "  ❬↑❭❬k❭          "},
	    {"  j,<Down>         ", "  ❬↓❭❬j❭          "},
	    {"  C-b,PgUp         ", "  ❬C-b❭❬PgUp❭     "},
	    {"  C-f,PgDn         ", "  ❬C-f❭❬PgDn❭     "},
	    {"  gg,Home          ", "  ❬gg❭❬Home❭      "},
	    {"  G,End            ", "  ❬G❭❬End❭        "},
	    {"  Tab              ", "  ❬TAB❭           "},
	    {"  c                ", "  ❬c❭             "},
	    {"  f                ", "  ❬f❭             "},
	    {"  /                ", "  ❬/❭             "},
	    {"  n                ", "  ❬n❭             "},
	    {"  N                ", "  ❬N❭             "},
	    {"  q                ", "  ❬q❭             "},
	    {"  Q                ", "  ❬Q❭             "},
	    {""},
	    {""}, /* Timeline */
	    {"  <,,              ", "  ❬<❭❬,❭          "},
	    {"  >,.              ", "  ❬>❭❬.❭          "},
	    {"  Enter,Space      ", "  ❬Enter❭❬Space❭  "},
	    {"  b                ", "  ❬b❭             "},
	    {"  F                ", "  ❬F❭             "},
	    {"  t                ", "  ❬t❭             "},
	    {""},
	    {""}, /* Diff */
	    {"  Space            ", "  ❬Space❭         "},
	    {"  b                ", "  ❬b❭             "},
	    {"  i                ", "  ❬i❭             "},
	    {"  v                ", "  ❬v❭             "},
	    {"  w                ", "  ❬w❭             "},
	    {"  -,_              ", "  ❬-❭❬_❭          "},
	    {"  +,=              ", "  ❬+❭❬=❭          "},
	    {"  C-k,K,<,,        ", "  ❬C-k❭❬K❭❬<❭❬,❭  "},
	    {"  C-j,J,>,.        ", "  ❬C-j❭❬J❭❬>❭❬.❭  "},
	    {""},
	    {""}, /* Tree */
	    {"  l,Enter,<Right>  ", "  ❬→❭❬l❭❬Enter❭   "},
	    {"  h,<BS>,<Left>    ", "  ❬←❭❬h❭❬⌫❭       "},
	    {"  b                ", "  ❬b❭             "},
	    {"  d                ", "  ❬d❭             "},
	    {"  i                ", "  ❬i❭             "},
	    {"  t                ", "  ❬t❭             "},
	    {""},
	    {""}, /* Blame */
	    {"  Space            ", "  ❬Space❭         "},
	    {"  Enter            ", "  ❬Enter❭         "},
	    {"  b                ", "  ❬b❭             "},
	    {"  p                ", "  ❬p❭             "},
	    {"  B                ", "  ❬B❭             "},
	    {"  T                ", "  ❬T❭             "},
	    {""},
	    {""}, /* Branch */
	    {"  Enter,Space      ", "  ❬Enter❭❬Space❭  "},
	    {"  d                ", "  ❬d❭             "},
	    {"  i                ", "  ❬i❭             "},
	    {"  s                ", "  ❬s❭             "},
	    {"  t                ", "  ❬t❭             "},
	    {"  R,<C-l>          ", "  ❬R❭❬C-l❭        "},
	    {""},
	    {""},
	    {0}
	};
	static const char *desc[] = {
	    "",
	    "Global",
	    "Open in-app help",
	    "Move selection cursor or page up one line",
	    "Move selection cursor or page down one line",
	    "Scroll up one page",
	    "Scroll down one page",
	    "Jump to first line or start of the view",
	    "Jump to last line or end of the view",
	    "Switch focus between open views",
	    "Toggle coloured output",
	    "Toggle fullscreen",
	    "Open prompt to enter search term (not available in this view)",
	    "Find next line or token matching the current search term",
	    "Find previous line or token matching the current search term",
	    "Quit the active view",
	    "Quit the program",
	    "",
	    "Timeline",
	    "Move selection cursor up one commit",
	    "Move selection cursor down one commit",
	    "Open diff view of the selected commit",
	    "Open and populate branch view with all repository branches",
	    "Open prompt to enter term with which to filter new timeline view",
	    "Display a tree reflecting the state of the selected commit",
	    "",
	    "Diff",
	    "Scroll down one page of diff output",
	    "Open and populate branch view with all repository branches",
	    "Toggle inversion of diff output",
	    "Toggle verbosity of diff output",
	    "Toggle ignore whitespace-only changes in diff",
	    "Decrease the number of context lines",
	    "Increase the number of context lines",
	    "Display diff of next (newer) commit in the timeline",
	    "Display diff of previous (older) commit in the timeline",
	    "",
	    "Tree",
	    "Move into the selected directory",
	    "Return to the parent directory",
	    "Open and populate branch view with all repository branches",
	    "Toggle ISO8601 modified timestamp display for each tree entry",
	    "Toggle display of file artifact SHA hash ID",
	    "Display timeline of all commits modifying the selected entry",
	    "",
	    "Blame",
	    "Scroll down one page",
	    "Display the diff of the commit corresponding to the selected line",
	    "Blame the version of the file found in the selected line's commit",
	    "Blame the version of the file found in the selected line's parent "
	    "commit",
	    "Reload the previous blamed version of the file",
	    "Open and populate branch view with all repository branches",
	    "",
	    "Branch",
	    "Display the timeline of the currently selected branch",
	    "Toggle display of the date when the branch last received changes",
	    "Toggle display of the SHA hash that identifies the branch",
	    "Toggle branch sort order (lexicographical -> mru -> state)",
	    "Open a tree view of the currently selected branch",
	    "Reload view with all repository branches and no filters applied",
	    "",
	    "  See fnc(1) for complete list of options and key bindings."
	};
	int	cs, ln, width = 0, rc = 0;

	cs = (strcmp(nl_langinfo(CODESET), "UTF-8") == 0) ? 1 : 0;

	title = fsl_mprintf("%s %s Help\n", fcli_progname(), PRINT_VERSION);
	if (title == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_mprintf");

	help = tmpfile();
	if (help == NULL)
		return RC(FSL_RC_IO, "%s", "tmpfile");

	/*
	 * Format help text, and compute longest line and total number of
	 * lines in text to be displayed to determine pad dimensions.
	 */
	width = fsl_strlen(title);
	for (ln = 0; keys[ln][0]; ++ln) {
		if (keys[ln][1]) {
			width = MAX((fsl_size_t)width,
			    fsl_strlen(keys[ln][cs]) + fsl_strlen(desc[ln]));
		}
		fsl_fprintf(help, "%s%s%c", keys[ln][cs], desc[ln],
		    keys[ln + 1] ? '\n' : 0);
	}
	rewind(help);

	rc = padpopup(view, width, ln, help, title);
	if (fclose(help) == EOF)
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_IO), "%s", "fclose");

	fsl_free(title);
	return rc;
}

/*
 * Create popup pad in which to write the supplied txt string and optional
 * title. The pad is contained within a window that is offset four columns in
 * and two lines down from the parent window.
 */
static int
padpopup(struct fnc_view *view, int width, int height, FILE *txt,
    const char *title)
{
	WINDOW		*win, *content;
	char		*line = NULL;
	ssize_t		 linelen;
	size_t		 linesz;
	int		 ch, cury, end, wy, wx, x0, y0;

	x0 = 4;		/* Number of columns to border window. */
	y0 = 2;		/* Number of lines to border window. */
	cury = 0;
	wx = getmaxx(view->window) - ((x0 + 1) * 2); /* Width of window. */
	wy = getmaxy(view->window) - ((y0 + 1) * 2); /* Height of window */
	ch = ERR;

	if ((win = newwin(wy, wx, y0, x0)) == 0)
		return RC(FSL_RC_ERROR, "%s", "newwin");
	if ((content = newpad(height + 1, width + 1)) == 0) {
		delwin(win);
		return RC(FSL_RC_ERROR, "%s", "newpad");
	}

	doupdate();
	keypad(content, TRUE);

	/* Write text content to pad. */
	if (title)
		centerprint(content, 0, 0, width, title, 0);
	while ((linelen = getline(&line, &linesz, txt)) != -1)
		waddstr(content, line);
	fsl_free(line);

	end = (getcury(content) - (wy - 3));  /* No. lines past end of pad. */
	do {
		switch (ch) {
			case KEY_UP:
			case 'k':
				if (cury > 0)
					--cury;
				break;
			case KEY_DOWN:
			case 'j':
				if (cury < end)
					++cury;
				break;
			case KEY_PPAGE:
			case CTRL('b'):
				if (cury > 0) {
					cury -= wy / 2;
					if (cury < 0)
						cury = 0;
				}
				break;
			case KEY_NPAGE:
			case CTRL('f'):
			case ' ':
				if (cury < end) {
					cury += wy / 2;
					if (cury > end)
						cury = end;
				}
				break;
			case 'g':
				if (!fnc_home(view))
					break;
				/* FALL THROUGH */
			case KEY_HOME:
				cury = 0;
				break;
			case KEY_END:
			case 'G':
				cury = end;
				break;
			case ERR:
			default:
				break;
		}
		werase(win);
		box(win, 0, 0);
		wnoutrefresh(win);
		pnoutrefresh(content, cury, 0, y0 + 1, x0 + 1, wy, wx);
		doupdate();
	} while ((ch = wgetch(content)) != 'q' && ch != KEY_ESCAPE
	    && ch != ERR);

	/* Destroy window. */
	werase(win);
	wrefresh(win);
	delwin(win);
	delwin(content);

	/* Restore fnc window content. */
	touchwin(view->window);
	wnoutrefresh(view->window);
	doupdate();

	return 0;
}

static void
centerprint(WINDOW *win, int starty, int startx, int cols, const char *str,
    chtype colour)
{
	int	x, y;

	if (win == NULL)
		win = stdscr;

	getyx(win, y, x);
	x = startx ? startx : x;
	y = starty ? starty : y;
	if (!cols)
		cols = getmaxx(win);

	x = startx + (cols - fsl_strlen(str)) / 2;
	wattron(win, colour ? colour : A_UNDERLINE);
	mvwprintw(win, y, x, "%s", str);
	wattroff(win, colour ? colour : A_UNDERLINE);
	refresh();
}

static int
tl_input_handler(struct fnc_view **new_view, struct fnc_view *view, int ch)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct fnc_view			*branch_view = NULL;
	struct fnc_view			*tree_view = NULL;
	int				 rc = 0, start_col = 0;

	switch (ch) {
	case KEY_DOWN:
	case 'j':
	case '.':
	case '>':
		rc = move_tl_cursor_down(view, false);
		break;
	case KEY_NPAGE:
	case CTRL('f'): {
		rc = move_tl_cursor_down(view, true);
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
		move_tl_cursor_up(view, false, false);
		break;
	case KEY_PPAGE:
	case CTRL('b'):
		move_tl_cursor_up(view, true, false);
		break;
	case 'g':
		if (!fnc_home(view))
			break;
		/* FALL THROUGH */
	case KEY_HOME:
		move_tl_cursor_up(view, false, true);
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
		rc = open_commit_diff_view(new_view, view);
		break;
	case 'b':
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		branch_view = view_open(view->nlines, view->ncols,
		    view->start_ln, start_col, FNC_VIEW_BRANCH);
		if (branch_view == NULL)
			return RC(FSL_RC_ERROR, "%s", "view_open");
		rc = open_branch_view(branch_view, BRANCH_LS_OPEN_CLOSED, NULL,
		    0, 0);
		if (rc) {
			view_close(branch_view);
			return rc;
		}
		s->thread_cx.needs_reset = true;
		view->active = false;
		branch_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, branch_view);
			view->focus_child = true;
		} else
			*new_view = branch_view;
		break;
	case 'c':
		s->colour = !s->colour;
		break;
	case 'F': {
		struct fnc_view *new;
		char glob[BUFSIZ];
		int retval;
		mvwaddstr(view->window, view->start_ln + view->nlines - 1, 0,
		    "/");
		wclrtoeol(view->window);
		nocbreak();
		echo();
		retval = wgetnstr(view->window, glob, sizeof(glob));
		cbreak();
		noecho();
		if (retval == ERR)
			return rc;
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		new = view_open(view->nlines, view->ncols, view->start_ln,
		    start_col, FNC_VIEW_TIMELINE);
		if (new == NULL)
			return RC(FSL_RC_ERROR, "%s", "view_open");
		rc = open_timeline_view(new, 0, "/", glob);
		if (rc) {
			if (rc != FSL_RC_BREAK)
				return rc;
			wattr_on(view->window, A_BOLD, NULL);
			mvwaddstr(view->window,
			    view->start_ln + view->nlines - 1, 0,
			    "-- no matching commits --");
			wclrtoeol(view->window);
			wattr_off(view->window, A_BOLD, NULL);
			fcli_err_reset();
			rc = 0;
			update_panels();
			doupdate();
			sleep(1);
			break;
		}
		view->active = false;
		new->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, new);
			view->focus_child = true;
		} else
			*new_view = new;
		break;
	}
	case 't':
		if (s->selected_commit == NULL)
			break;
		if (!fsl_rid_is_a_checkin(fcli_cx(),
		    s->selected_commit->commit->rid)) {
			wattr_on(view->window, A_BOLD, NULL);
			mvwaddstr(view->window,
			    view->start_ln + view->nlines - 1, 0,
			    "-- tree requires check-in artifact --");
			wclrtoeol(view->window);
			wattr_off(view->window, A_BOLD, NULL);
			fcli_err_reset();
			rc = 0;
			update_panels();
			doupdate();
			sleep(1);
			break;
		}
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		rc = browse_commit_tree(&tree_view, start_col,
		    s->selected_commit, s->path);
		if (rc)
			break;
		s->thread_cx.needs_reset = true;
		view->active = false;
		tree_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, tree_view);
			view->focus_child = true;
		} else
			*new_view = tree_view;
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
move_tl_cursor_down(struct fnc_view *view, bool page)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct commit_entry		*first;
	int				 rc = FSL_RC_OK;

	first = s->first_commit_onscreen;
	if (first == NULL)
		return rc;

	if (s->thread_cx.timeline_end &&
	    s->selected_commit->idx >= s->commits.ncommits - 1)
		return rc;

	if (!page &&
	    s->selected_idx < MIN(view->nlines - 2, s->commits.ncommits - 1))
		++s->selected_idx;
	else {
		rc = timeline_scroll_down(view, !page ? 1 :
		    s->thread_cx.timeline_end ?
		    MIN(s->commits.ncommits - s->selected_commit->idx - 1,
		    view->nlines - 1) : view->nlines - 1);
		if (rc)
			return rc;
		if (first == s->first_commit_onscreen && s->selected_idx <
		    MIN(view->nlines - 2, s->commits.ncommits - 1)) {
			/* End of timeline, no more commits; move cursor down */
			int end = MIN(s->selected_idx +=
			    s->commits.ncommits - s->selected_commit->idx - 1,
			    MIN(view->nlines - 2, s->commits.ncommits - 1));
			s->selected_idx = end;
		}
	}

	if (!rc)
		select_commit(s);
	return rc;
}

static void
move_tl_cursor_up(struct fnc_view *view, bool page, bool end)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;

	if (s->first_commit_onscreen == NULL)
		return;

	if ((page && TAILQ_FIRST(&s->commits.head) == s->first_commit_onscreen)
	    || end)
		s->selected_idx = 0;

	if (!page && !end && s->selected_idx > 0)
		--s->selected_idx;
	else
		timeline_scroll_up(s, end ? s->commits.ncommits : page ?
		    view->nlines - 1 : 1);

	select_commit(s);
	return;
}

static int
open_commit_diff_view(struct fnc_view **new_view, struct fnc_view *view)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct fnc_view			*diff_view;
	int				 x = 0, y = 0, rc = FSL_RC_OK;

	if (s->selected_commit == NULL)
		return rc;

	if (view_is_parent(view)) {
		rc = view_set_split(view, &s->selected_idx, &x, &y);
		if (rc)
			return rc;
	}

	rc = init_diff_commit(&diff_view, x, y, s->selected_commit->commit,
	    view);
	if (rc)
		return rc;

	view->active = false;
	diff_view->active = true;
	diff_view->mode = view->mode;
	diff_view->nlines = view->lines - y;

	if (view_is_parent(view)) {
		if (view->child != NULL) {
			rc = view_close(view->child);
			view->child = NULL;
			if (rc)
				return rc;
		}
		view->child = diff_view;
		diff_view->parent = view;
		view->focus_child = true;
	} else
		*new_view = diff_view;

	return rc;
}

/*
 * Set dimensions for splitscreen view. If FNC_VIEW_SPLIT_MODE is either unset
 * or set to auto, determine vertical or horizontal split depending on screen
 * estate. If set to 'v' or 'h', assign start column or start line of the split
 * view to *start_col and *start_ln, respectively. If the line of the selected
 * commit is in the bottom horizontal split section, scroll the timeline to
 * offset the displacement of moving the selected commit into the top split and
 * deduct the offset from *selected.
 */
static int
view_set_split(struct fnc_view *view, int *selected, int *start_col,
    int *start_ln)
{
	char	*mode;
	int	 offset, rc = FSL_RC_OK;

	mode = fnc_conf_getopt(FNC_VIEW_SPLIT_MODE, false);

	if (!mode || mode[0] != 'h')
		*start_col = view_split_start_col(view->start_col);

	if (!*start_col && (!mode || mode[0] != 'v')) {
		*start_ln = view_split_start_ln(view->lines);
		view->mode = VIEW_SPLIT_HRZN;
		view->nlines = *start_ln;
		rc = view_resize(view);
		if (rc)
			goto end;
		view->nlines = *start_ln - 1;
		if (*selected >= view->nlines - 1) {
			offset = ABS(view->nlines - *selected - 2);
			rc = timeline_scroll_down(view, offset);
			*selected -= offset;
		}
	}
end:
	fsl_free(mode);
	return rc;
}

static int
timeline_scroll_down(struct fnc_view *view, int maxscroll)
{
	struct fnc_tl_view_state	*s = &view->state.timeline;
	struct commit_entry		*pentry;
	int				 rc = 0, nscrolled = 0, ncommits_needed;

	if (s->last_commit_onscreen == NULL || !maxscroll)
		return rc;

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
		if (pentry == NULL && view->mode != VIEW_SPLIT_HRZN)
			break;

		s->last_commit_onscreen = pentry ?
		    pentry : s->last_commit_onscreen;

		pentry = TAILQ_NEXT(s->first_commit_onscreen, entries);
		if (pentry == NULL)
			break;
		s->first_commit_onscreen = pentry;
	} while (++nscrolled < maxscroll);

	s->nscrolled += view->mode == VIEW_SPLIT_HRZN ? nscrolled : 0;
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
	int	 rc = FSL_RC_OK;

	view->start_ln = view->mode == VIEW_SPLIT_HRZN ?
	    view_split_start_ln(view->nlines) : 0;
	view->start_col = view->mode != VIEW_SPLIT_HRZN ?
	    view_split_start_col(0) : 0;
	view->nlines = LINES - view->start_ln;
	view->ncols = COLS - view->start_col;
	view->lines = LINES;
	view->cols = COLS;

	rc = view_resize(view);
	if (rc)
		return rc;

	if (view->parent && view->mode == VIEW_SPLIT_HRZN)
		view->parent->nlines = view->lines - view->nlines - 1;

	if (mvwin(view->window, view->start_ln, view->start_col) == ERR)
		return RC(FSL_RC_ERROR, "%s", "mvwin");

	return rc;
}

static int
make_fullscreen(struct fnc_view *view)
{
	int	 rc = FSL_RC_OK;

	view->start_col = 0;
	view->start_ln = 0;
	view->nlines = LINES;
	view->ncols = COLS;
	view->lines = LINES;
	view->cols = COLS;

	rc = view_resize(view);
	if (rc)
		return rc;

	if (mvwin(view->window, view->start_ln, view->start_col) == ERR)
		return RC(FSL_RC_ERROR, "%s", "mvwin");

	return rc;
}

/*
 * Find start column for vertical split. If terminal width is < 120 columns,
 * return 0 (i.e., do not split; open new view in the existing one). If >= 120,
 * return the largest of 80 columns or 50% of the current view width subtracted
 * from COLS so that the child view will be no smaller than 80 columns wide.
 */
static int
view_split_start_col(int start_col)
{
	if (start_col > 0 || COLS < 120)
		return 0;
	return (COLS - MAX(COLS / 2, 80));
}

/*
 * Find start line for horizontal split. If FNC_VIEW_SPLIT_HEIGHT is set as
 * either an absolute line value or % equalling less than lines - 2, subtract
 * from lines and return. If invalid or not set, return HSPLIT_SCALE(lines).
 */
static int
view_split_start_ln(int lines)
{
	char		*height = NULL;
	long		 n = 0;
	int		 rc = FSL_RC_OK;

	height = fnc_conf_getopt(FNC_VIEW_SPLIT_HEIGHT, false);

	if (height && height[fsl_strlen(height) - 1] == '%') {
		n = strtol(height, NULL, 10);
		if (n > INT_MAX || (errno == ERANGE && n == LONG_MAX))
			rc = RC(fsl_errno_to_rc(errno, FSL_RC_RANGE),
			    "%s", "strtol");
		if (n < INT_MIN || (errno == ERANGE && n == LONG_MIN))
			rc = RC(fsl_errno_to_rc(errno, FSL_RC_RANGE),
			    "%s", "strtol");
		if (!rc)
			n = lines * ((float)n / 100);
	} else if (height)
		rc = strtonumcheck(&n, height, 0, lines);

	fsl_free(height);
	return !rc && n && n < (lines - 2) ? lines - n : lines * HSPLIT_SCALE;
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
		rc = view->search_next(view);

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
		rc = view->search_next(view);
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
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_unlock");
		ch = wgetch(view->window);
		if ((rc = pthread_mutex_lock(&fnc_mutex)))
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_lock");
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
	fsl_stmt_finalize(s->thread_cx.q);
	fnc_free_commits(&s->commits);
	free_colours(&s->colours);
	regfree(&view->regex);
	fsl_free(s->path);
	s->path = NULL;

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
	int	 rc = 0;

	if (s->thread_id) {
		s->quit = 1;

		if ((rc = pthread_cond_signal(&s->thread_cx.commit_consumer)))
			return RC(fsl_errno_to_rc(rc, FSL_RC_MISUSE),
			    "%s", "pthread_cond_signal");
		if ((rc = pthread_mutex_unlock(&fnc_mutex)))
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_unlock");
		if ((rc = pthread_join(s->thread_id, &err)) ||
		    err == PTHREAD_CANCELED)
			return RC(fsl_errno_to_rc(rc, FSL_RC_MISUSE),
			    "%s", "pthread_join");
		if ((rc = pthread_mutex_lock(&fnc_mutex)))
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_lock");

		s->thread_id = 0;
	}

	if ((rc = pthread_cond_destroy(&s->thread_cx.commit_consumer)))
		RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_cond_destroy");

	if ((rc = pthread_cond_destroy(&s->thread_cx.commit_producer)))
		RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_cond_destroy");

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
	fsl_list_clear(&commit->changeset, fsl_file_artifact_free, NULL);
	fsl_list_reserve(&commit->changeset, 0);
	fsl_free(commit);
}

static int
fsl_file_artifact_free(void *elem, void *state)
{
	struct fsl_file_artifact *ffa = elem;
	fsl_free(ffa->fc->name);
	fsl_free(ffa->fc->uuid);
	fsl_free(ffa->fc->priorName);
	fsl_free(ffa->fc);
	fsl_free(ffa);

	return 0;
}

static int
init_diff_commit(struct fnc_view **new_view, int start_col, int start_ln,
    struct fnc_commit_artifact *commit, struct fnc_view *timeline_view)
{
	struct fnc_view			*diff_view;
	int				 rc = 0;

	diff_view = view_open(0, 0, start_ln, start_col, FNC_VIEW_DIFF);
	if (diff_view == NULL)
		return RC(FSL_RC_ERROR, "%s", "view_open");

	rc = open_diff_view(diff_view, commit, DEF_DIFF_CTX, fnc_init.ws,
	    fnc_init.invert, !fnc_init.quiet, timeline_view, true, NULL);
	if (!rc)
		*new_view = diff_view;

	return rc;
}

static int
open_diff_view(struct fnc_view *view, struct fnc_commit_artifact *commit,
    int context, bool ignore_ws, bool invert, bool verbosity,
    struct fnc_view *timeline_view, bool showmeta,
    struct fnc_pathlist_head *paths)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
	int				 rc = 0;

	s->paths = paths;
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
	s->colour = !fnc_init.nocolour && has_colors();
	s->showmeta = showmeta;

	if (s->colour) {
		STAILQ_INIT(&s->colours);
		rc = set_colours(&s->colours, FNC_VIEW_DIFF);
		if (rc)
			return rc;
	}

	if (timeline_view && screen_is_split(view))
		show_timeline_view(timeline_view); /* draw vborder */
	show_diff_status(view);

	s->line_offsets = NULL;
	s->nlines = 0;
	s->ncols = view->ncols;
	rc = create_diff(s);
	if (rc) {
		if (s->colour)
			free_colours(&s->colours);
		return rc;
	}

	view->show = show_diff;
	view->input = diff_input_handler;
	view->close = close_diff_view;
	view->search_init = diff_search_init;
	view->search_next = diff_search_next;

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
	FILE	*fout = NULL;
	char	*line, *st0 = NULL, *st = NULL;
	off_t	 lnoff = 0;
	int	 n, rc = 0;

	free(s->line_offsets);
	s->line_offsets = fsl_malloc(sizeof(off_t));
	if (s->line_offsets == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_malloc");
	s->nlines = 0;

	fout = tmpfile();
	if (fout == NULL) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_IO), "%s", "tmpfile");
		goto end;
	}
	if (s->f && fclose(s->f) == EOF) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_IO), "%s", "fclose");
		goto end;
	}
	s->f = fout;

	/*
	 * We'll diff artifacts of type "ci" (i.e., "checkin") separately, as
	 * it's a different process to diff the others (wiki, technote, etc.).
	 */
	if (s->selected_commit->diff_type == FNC_DIFF_COMMIT)
		rc = create_changeset(s->selected_commit);
	else if (s->selected_commit->diff_type == FNC_DIFF_BLOB)
		rc = diff_file_artifact(&s->buf, s->selected_commit->prid, NULL,
		    s->selected_commit->rid, NULL, FSL_CKOUT_CHANGE_MOD,
		    s->diff_flags, s->context, s->sbs,
		    s->selected_commit->diff_type);
	else if (s->selected_commit->diff_type == FNC_DIFF_WIKI)
		rc = diff_non_checkin(&s->buf, s->selected_commit,
		    s->diff_flags, s->context, s->sbs);
	if (rc)
		goto end;

	/*
	 * Delay assigning diff headline labels (i.e., diff id1 id2) till now
	 * because wiki parent commits are obtained in diff_non_checkin().
	 */
	if (s->selected_commit->puuid) {
		s->id1 = fsl_strdup(s->selected_commit->puuid);
		if (s->id1 == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
		}
	} else
		s->id1 = NULL;	/* Initial commit, tag, technote, etc. */
	if (s->selected_commit->uuid) {
		s->id2 = fsl_strdup(s->selected_commit->uuid);
		if (s->id2 == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
		}
	} else
		s->id2 = NULL;	/* Local work tree. */

	rc = add_line_offset(&s->line_offsets, &s->nlines, 0);
	if (rc)
		goto end;

	if (s->showmeta)
		write_commit_meta(s);

	/*
	 * Diff local changes on disk in the current checkout differently to
	 * checked-in versions: the former compares on disk file content with
	 * file artifacts; the latter compares file artifact blobs only.
	 */
	if (s->selected_commit->diff_type == FNC_DIFF_COMMIT)
		diff_commit(&s->buf, s->selected_commit, s->diff_flags,
		    s->context, s->sbs, s->paths);
	else if (s->selected_commit->diff_type == FNC_DIFF_CKOUT)
		diff_checkout(&s->buf, s->selected_commit->prid, s->diff_flags,
		    s->context, s->sbs, s->paths);

	/*
	 * Parse the diff buffer line-by-line to record byte offsets of each
	 * line for scrolling and searching in diff view.
	 */
	st0 = fsl_strdup(fsl_buffer_str(&s->buf));
	st = st0;
	lnoff = (s->line_offsets)[s->nlines - 1];
	while ((line = fnc_strsep(&st, "\n")) != NULL) {
		n = fprintf(s->f, "%s\n", line);
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
		rc = RC(FSL_RC_IO, "%s", "fflush");
	return rc;
}

static int
create_changeset(struct fnc_commit_artifact *commit)
{
	fsl_cx		*const f = fcli_cx();
	fsl_stmt	*st = NULL;
	fsl_list	 changeset = fsl_list_empty;
	int		 rc = 0;

	st = fsl_stmt_malloc();
	rc = fsl_cx_prepare(f, st,
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
		return RC(FSL_RC_DB, "%s", "fsl_cx_prepare");

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
		*fdiff->fc = fsl_card_F_empty;
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
	fsl_stmt_finalize(st);

	if (rc == FSL_RC_STEP_DONE)
		rc = 0;

	return rc;
}

static int
write_commit_meta(struct fnc_diff_view_state *s)
{
	char		*line = NULL, *st0 = NULL, *st = NULL;
	fsl_size_t	 linelen, idx = 0;
	off_t		 lnoff = 0;
	int		 n, rc = 0;

	if ((n = fprintf(s->f,"%s %s\n", s->selected_commit->type,
	    s->selected_commit->uuid)) < 0)
		goto end;
	lnoff += n;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	if ((n = fprintf(s->f,"user: %s\n", s->selected_commit->user)) < 0)
		goto end;
	lnoff += n;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	if ((n = fprintf(s->f,"tags: %s\n", s->selected_commit->branch ?
	    s->selected_commit->branch : "/dev/null")) < 0)
		goto end;
	lnoff += n;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	if ((n = fprintf(s->f,"date: %s\n",
	    s->selected_commit->timestamp)) < 0)
		goto end;
	lnoff += n;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	fputc('\n', s->f);
	++lnoff;
	if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
		goto end;

	st0 = fsl_strdup(s->selected_commit->comment);
	st = st0;
	if (st == NULL) {
		RC(FSL_RC_ERROR, "%s", "fsl_strdup");
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
			if ((n = fprintf(s->f, "%s\n", line)) < 0)
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
		if ((n = fprintf(s->f, "%s%s\n", changeline,
		    file_change->fc->name)) < 0)
			goto end;
		lnoff += n;
		if ((rc = add_line_offset(&s->line_offsets, &s->nlines, lnoff)))
			goto end;
	}

end:
	free(st0);
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
		if ((n  = fprintf(s->f, "%s ", word)) < 0)
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
	off_t	*p;

	p = fsl_realloc(*line_offsets, (*nlines + 1) * sizeof(off_t));
	if (p == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_realloc");
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
    int context, int sbs, struct fnc_pathlist_head *paths)
{
	fsl_cx			*const f = fcli_cx();
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
	 * For the one-and-only special case of repositories, such as the
	 * canonical fnc, that do not have an "initial empty check-in", we
	 * proceed with no parent version to diff against.
	 */
	if (commit->puuid) {
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
	}

	fsl_deck_F_next(&d2, &fc2);
	while (fc1 || fc2) {
		const fsl_card_F	*a = NULL, *b = NULL;
		fsl_ckout_change_e	 change = FSL_CKOUT_CHANGE_NONE;
		bool			 diff = true;

		if (paths != NULL && !TAILQ_EMPTY(paths)) {
			struct fnc_pathlist_entry *pe;
			diff = false;
			TAILQ_FOREACH(pe, paths, entry)
				if (!fsl_strcmp(pe->path, fc1->name) ||
				    !fsl_strcmp(pe->path, fc2->name) ||
				    !fsl_strncmp(pe->path, fc1->name,
				    pe->pathlen) || !fsl_strncmp(pe->path,
				    fc2->name, pe->pathlen)) {
					diff = true;
					break;
				}
		}

		if (!fc1)	/* File added. */
			different = 1;
		else if (!fc2)	/* File deleted. */
			different = -1;
		else		/* Same filename in both versions. */
			different = fsl_strcmp(fc1->name, fc2->name);

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
			if (diff)
				rc = diff_file_artifact(buf, id1, a,
				    commit->rid, b, change, diff_flags,
				    context, sbs, commit->diff_type);
		} else if (!fsl_uuidcmp(fc1->uuid, fc2->uuid)) { /* No change */
			fsl_deck_F_next(&d1, &fc1);
			fsl_deck_F_next(&d2, &fc2);
		} else {
			change = FSL_CKOUT_CHANGE_MOD;
			if (diff)
				rc = diff_file_artifact(buf, id1, fc1,
				    commit->rid, fc2, change, diff_flags,
				    context, sbs, commit->diff_type);
			fsl_deck_F_next(&d1, &fc1);
			fsl_deck_F_next(&d2, &fc2);
		}
		if (rc == FSL_RC_RANGE) {
			fsl_buffer_append(buf,
			    "\nDiff has too many changes\n", -1);
			rc = 0;
			fsl_cx_err_reset(f);
		} else if (rc == FSL_RC_DIFF_BINARY) {
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
    int sbs, struct fnc_pathlist_head *paths)
{
	fsl_cx		*const f = fcli_cx();
	fsl_stmt	*st = NULL;
	fsl_buffer	 sql, abspath, bminus;
	fsl_uuid_str	 xminus = NULL;
	fsl_id_t	 cid;
	int		 rc = 0;
	bool		 allow_symlinks;

	abspath = bminus = sql = fsl_buffer_empty;
	fsl_ckout_version_info(f, &cid, NULL);
	/* cid = fsl_config_get_id(f, FSL_CONFDB_CKOUT, 0, "checkout"); */
	/* XXX Already done in cmd_diff(): Load vfile table with local state. */
	/* rc = fsl_vfile_changes_scan(f, cid, */
	/*     FSL_VFILE_CKSIG_ENOTFILE & FSL_VFILE_CKSIG_KEEP_OTHERS); */
	/* if (rc) */
	/*	return RC(rc, "%s", "fsl_vfile_changes_scan"); */

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
	st = fsl_stmt_malloc();
	rc = fsl_cx_prepare(f, st, "%b", &sql);
	if (rc) {
		rc = RC(rc, "%s", "fsl_cx_prepare");
		goto yield;
	}

	while ((rc = fsl_stmt_step(st)) == FSL_RC_STEP_ROW) {
		const char	*path;
		int		 deleted, changed, added, fid, symlink;
		enum		 fsl_ckout_change_e change;
		bool		 diff = true;

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
						RC(FSL_RC_ERROR, "%s",
						    "fsl_strdup");
						goto yield;
					}
					fid = fsl_uuid_to_rid(f, xminus);
					break;
				}
			} while (cf);
			fsl_deck_finalize(&d);
		}
		if (!xminus)
			xminus = fsl_strdup(NULL_DEVICE);
		allow_symlinks = fsl_config_get_bool(f, FSL_CONFDB_REPO, false,
		    "allow-symlinks");
		if (!symlink != !(fsl_is_symlink(fsl_buffer_cstr(&abspath)) &&
		    allow_symlinks)) {
			rc = write_diff_meta(buf, path, xminus, path,
			    NULL_DEVICE, diff_flags, change);
			fsl_buffer_append(buf, "\nSymbolic links and regular "
			    "files cannot be diffed\n", -1);
			if (rc)
				goto yield;
			continue;
		}
		if (fid > 0 && change != FSL_CKOUT_CHANGE_ADDED) {
			rc = fsl_content_get(f, fid, &bminus);
			if (rc)
				goto yield;
		} else
			fsl_buffer_clear(&bminus);
		if (paths != NULL && !TAILQ_EMPTY(paths)) {
			struct fnc_pathlist_entry *pe;
			diff = false;
			TAILQ_FOREACH(pe, paths, entry)
				if (!fsl_strncmp(pe->path, path, pe->pathlen)
				    || !fsl_strcmp(pe->path, path)) {
					diff = true;
					break;
				}
		}
		if (diff)
			rc = diff_file(buf, &bminus, path, xminus,
			    fsl_buffer_cstr(&abspath), change, diff_flags,
			    context, sbs);
		fsl_buffer_reuse(&bminus);
		fsl_buffer_reuse(&abspath);
		fsl_free(xminus);
		xminus = NULL;
		if (rc == FSL_RC_RANGE) {
			fsl_buffer_append(buf,
			    "\nDiff has too many changes\n", -1);
			rc = 0;
			fsl_cx_err_reset(f);
		} else if (rc == FSL_RC_DIFF_BINARY) {
			fsl_buffer_append(buf,
			    "\nBinary files cannot be diffed\n", -1);
			rc = 0;
			fsl_cx_err_reset(f);
		} else if (rc)
			goto yield;
	}

yield:
	fsl_stmt_finalize(st);
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

	index = zplus ? zplus : (zminus ? zminus : NULL_DEVICE);

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

	if (diff_flags & FSL_DIFF_INVERT) {
		const char *tmp = minus;
		minus = plus;
		plus = tmp;
		tmp = zminus;
		zminus = zplus;
		zplus = tmp;
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
	fsl_cx		*const f = fcli_cx();
	fsl_buffer	 bplus = fsl_buffer_empty;
	fsl_buffer	 xplus = fsl_buffer_empty;
	const char	*zplus = NULL;
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
	if (change != FSL_CKOUT_CHANGE_REMOVED) {
		rc = fsl_ckout_file_content(f, false, abspath, &bplus);
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
		RC(FSL_RC_SIZE_MISMATCH, "invalid artifact uuid [%s]", xminus);
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
	fsl_cx		*const f = fcli_cx();
	fsl_buffer	 wiki = fsl_buffer_empty;
	fsl_buffer	 pwiki = fsl_buffer_empty;
	fsl_id_t	 prid = 0;
	fsl_size_t	 idx;
	int		 rc = 0;

	fsl_deck *d = NULL;
	d = fsl_deck_malloc();
	if (d == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_deck_malloc");

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
			fsl_buffer_copy(buf, &wiki);
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
    int diff_flags, int context, int sbs, enum fnc_diff_type diff_type)
{
	fsl_cx		*const f = fcli_cx();
	fsl_stmt	 stmt = fsl_stmt_empty;
	fsl_buffer	 fbuf1 = fsl_buffer_empty;
	fsl_buffer	 fbuf2 = fsl_buffer_empty;
	char		*zminus0 = NULL, *zplus0 = NULL;
	const char	*zplus = NULL, *zminus = NULL;
	fsl_uuid_str	 xplus0 = NULL, xminus0 = NULL;
	fsl_uuid_str	 xplus = NULL, xminus = NULL;
	int		 rc = 0;
	bool		 verbose;

	assert(vid1 != vid2);
	assert(vid2 > 0 &&
	    "local checkout should be diffed with diff_checkout()");

	fbuf2.used = fbuf1.used = 0;

	if (a) {
		rc = fsl_card_F_content(f, a, &fbuf1);
		if (rc)
			goto end;
		zminus = a->name;
		xminus = a->uuid;
	} else if (diff_type == FNC_DIFF_BLOB) {
		rc = fsl_cx_prepare(f, &stmt,
		    "SELECT name FROM filename, mlink "
		    "WHERE filename.fnid=mlink.fnid AND mlink.fid = %d", vid1);
		if (rc) {
			rc = RC(FSL_RC_DB, "%s %d", "fsl_cx_prepare", vid1);
			goto end;
		}
		rc = fsl_stmt_step(&stmt);
		if (rc == FSL_RC_STEP_ROW) {
			rc = 0;
			zminus0 = fsl_strdup(fsl_stmt_g_text(&stmt, 0, NULL));
			zminus = zminus0;
		} else if (rc == FSL_RC_STEP_DONE)
			rc = 0;
		else if (rc) {
			rc = RC(rc, "%s", "fsl_stmt_step");
			goto end;
		}
		xminus0 = fsl_rid_to_uuid(f, vid1);
		xminus = xminus0;
		fsl_stmt_finalize(&stmt);
		fsl_content_get(f, vid1, &fbuf1);
	}
	if (b) {
		rc = fsl_card_F_content(f, b, &fbuf2);
		if (rc)
			goto end;
		zplus = b->name;
		xplus = b->uuid;
	} else if (diff_type == FNC_DIFF_BLOB) {
		rc = fsl_cx_prepare(f, &stmt,
		    "SELECT name FROM filename, mlink "
		    "WHERE filename.fnid=mlink.fnid AND mlink.fid = %d", vid2);
		if (rc) {
			rc = RC(FSL_RC_DB, "%s %d", "fsl_cx_prepare", vid2);
			goto end;
		}
		rc = fsl_stmt_step(&stmt);
		if (rc == FSL_RC_STEP_ROW) {
			rc = 0;
			zplus0 = fsl_strdup(fsl_stmt_g_text(&stmt, 0, NULL));
			zplus = zplus0;
		} else if (rc == FSL_RC_STEP_DONE)
			rc = 0;
		else if (rc) {
			rc = RC(rc, "%s", "fsl_stmt_step");
			goto end;
		}
		xplus0 = fsl_rid_to_uuid(f, vid2);
		xplus = xplus0;
		fsl_stmt_finalize(&stmt);
		fsl_content_get(f, vid2, &fbuf2);
	}

	rc = write_diff_meta(buf, zminus, xminus, zplus, xplus, diff_flags,
	    change);
	verbose = (diff_flags & FSL_DIFF_VERBOSE) != 0 ? true : false;
	if (verbose || (a && b))
		rc = fsl_diff_text_to_buffer(&fbuf1, &fbuf2, buf, context, sbs,
		    diff_flags);
	if (rc)
		RC(rc, "%s: fsl_diff_text_to_buffer\n"
		    " -> %s [%s]\n -> %s [%s]", fsl_rc_cstr(rc),
		    a ? a->name : NULL_DEVICE, a ? a->uuid : NULL_DEVICE,
		    b ? b->name : NULL_DEVICE, b ? b->uuid : NULL_DEVICE);
end:
	fsl_free(zminus0);
	fsl_free(zplus0);
	fsl_free(xminus0);
	fsl_free(xplus0);
	fsl_buffer_clear(&fbuf1);
	fsl_buffer_clear(&fbuf2);
	return rc;
}

static int
show_diff(struct fnc_view *view)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
	char				*headln, *id2, *id1 = NULL;

	/* Some diffs (e.g., technote, tag) have no parent hash to display. */
	id1 = fsl_strdup(s->id1 ? s->id1 : "/dev/null");
	if (id1 == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_strdup");

	/*
	 * If diffing the work tree, we have no hash to display for it.
	 * XXX Display "work tree" or "checkout" or "/dev/null" for clarity?
	 */
	id2 = fsl_strdup(s->id2 ? s->id2 : "");
	if (id2 == NULL) {
		fsl_free(id1);
		return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
	}

	if ((headln = fsl_mprintf("diff %.40s %.40s", id1, id2)) == NULL) {
		fsl_free(id1);
		fsl_free(id2);
		return RC(FSL_RC_RANGE, "%s", "fsl_mprintf");
	}

	fsl_free(id1);
	fsl_free(id2);
	return write_diff(view, headln);
}

static int
write_diff(struct fnc_view *view, char *headln)
{
	struct fnc_diff_view_state	*s = &view->state.diff;
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

	line_offset = s->line_offsets[s->first_line_onscreen - 1];
	if (fseeko(s->f, line_offset, SEEK_SET))
		return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR), "%s", "fseeko");

	werase(view->window);

	if (headln) {
		if ((line = fsl_mprintf("[%d/%d] %s", (s->first_line_onscreen -
		    1 + s->current_line), nlines, headln)) == NULL)
			return RC(FSL_RC_RANGE, "%s", "fsl_mprintf");
		rc = formatln(&wcstr, &wstrlen, line, view->ncols, 0);
		fsl_free(line);
		fsl_free(headln);
		if (rc)
			return rc;

		if (screen_is_shared(view))
			wattron(view->window, A_REVERSE);
		waddwstr(view->window, wcstr);
		fsl_free(wcstr);
		wcstr = NULL;
		while (wstrlen < view->ncols) {
			waddch(view->window, ' ');
			++wstrlen;
		}
		if (screen_is_shared(view))
			wattroff(view->window, A_REVERSE);

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
			RC(ferror(s->f) ? fsl_errno_to_rc(errno, FSL_RC_IO) :
			    FSL_RC_IO, "%s", "getline");
			return rc;
		}

		if (s->colour)
			c = match_colour(&s->colours, line);
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
		if (c)
			wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
		if (wstrlen <= view->ncols - 1)
			waddch(view->window, '\n');
		++nprintln;
	}
	fsl_free(line);
	if (nprintln >= 1)
		s->last_line_onscreen = s->first_line_onscreen + (nprintln - 1);
	else
		s->last_line_onscreen = s->first_line_onscreen;

	drawborder(view);

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

static bool
screen_is_split(struct fnc_view *view)
{
	return view->start_col > 0 || view->start_ln > 0;
}

static int
write_matched_line(int *col_pos, const char *line, int ncols_avail,
    int start_column, WINDOW *window, regmatch_t *regmatch)
{
	wchar_t		*wcstr;
	char		*s;
	int		 wstrlen;
	int		 rc = 0;

	*col_pos = 0;

	/* Copy the line up to the matching substring & write it to screen. */
	s = fsl_strndup(line, regmatch->rm_so);
	if (s == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_strndup");

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
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strndup");
			free(s);
			return rc;
		}
		rc = formatln(&wcstr, &wstrlen, s, ncols_avail, start_column);
		if (rc) {
			free(s);
			return rc;
		}
		wattr_on(window, A_REVERSE, NULL);
		waddwstr(window, wcstr);
		wattr_off(window, A_REVERSE, NULL);
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
drawborder(struct fnc_view *view)
{
	const struct fnc_view	*view_above;
	char			*codeset = nl_langinfo(CODESET);
	PANEL			*panel;

	if (view->parent)
		drawborder(view->parent);

	panel = panel_above(view->panel);
	if (panel == NULL)
		return;

	view_above = panel_userptr(panel);
	if (view->mode == VIEW_SPLIT_HRZN)
		mvwhline(view->window, view_above->start_ln - 1,
		    view->start_col, (strcmp(codeset, "UTF-8") == 0) ?
		    ACS_HLINE : '-', view->ncols);
	else
		mvwvline(view->window, view->start_ln,
		    view_above->start_col - 1, (strcmp(codeset, "UTF-8") == 0) ?
		    ACS_VLINE : '|', view->nlines);
#ifdef __linux__
	wnoutrefresh(view->window);
#endif
}

static int
diff_input_handler(struct fnc_view **new_view, struct fnc_view *view, int ch)
{
	struct fnc_view			*branch_view;
	struct fnc_diff_view_state	*s = &view->state.diff;
	struct fnc_tl_view_state	*tlstate;
	struct commit_entry		*previous_selection;
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
	case ' ':
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
					RC(ferror(s->f) ?
					    fsl_errno_to_rc(errno, FSL_RC_IO) :
					    FSL_RC_IO, "%s", "getline");
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
	case 'g':
		if (!fnc_home(view))
			break;
		/* FALL THROUGH */
	case KEY_HOME:
		s->first_line_onscreen = 1;
		break;
	case 'b': {
		int start_col = 0;
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		branch_view = view_open(view->nlines, view->ncols,
		    view->start_ln, start_col, FNC_VIEW_BRANCH);
		if (branch_view == NULL)
			return RC(FSL_RC_ERROR, "%s", "view_open");
		rc = open_branch_view(branch_view, BRANCH_LS_OPEN_CLOSED, NULL,
		    0, 0);
		if (rc) {
			view_close(branch_view);
			return rc;
		}
		view->active = false;
		branch_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, branch_view);
			view->focus_child = true;
		} else
			*new_view = branch_view;
		break;
	}
	case 'c':
	case 'i':
	case 'v':
	case 'w':
		if (ch == 'c')
			s->colour = !s->colour;
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
		if (s->context < MAX_DIFF_CTX) {
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
	case 'q':
		if (s->timeline_view && view->mode == VIEW_SPLIT_HRZN) {
			/* May need more commits to fill fullscreen. */
			rc = request_tl_commits(s->timeline_view);
			s->timeline_view->mode = VIEW_SPLIT_NONE;
		}
		/* FALL THROUGH */
	default:
		break;
	}

	return rc;
}

static int
request_tl_commits(struct fnc_view *view)
{
	struct fnc_tl_view_state	*state = &view->state.timeline;
	int				 rc = FSL_RC_OK;

	state->thread_cx.ncommits_needed = state->nscrolled;
	rc = signal_tl_thread(view, 1);
	state->nscrolled = 0;

	return rc;
}

static int
set_selected_commit(struct fnc_diff_view_state *s, struct commit_entry *entry)
{
	fsl_free(s->id2);
	s->id2 = fsl_strdup(entry->commit->uuid);
	if (s->id2 == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
	fsl_free(s->id1);
	s->id1 = entry->commit->puuid ? fsl_strdup(entry->commit->puuid) : NULL;
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
			return RC(fsl_errno_to_rc(errno, FSL_RC_IO),
			    "%s", "fseeko");
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
	int				 rc = 0;

	if (s->f && fclose(s->f) == EOF)
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_IO), "%s", "fclose");
	fsl_free(s->id1);
	s->id1 = NULL;
	fsl_free(s->id2);
	s->id2 = NULL;
	fsl_free(s->line_offsets);
	free_colours(&s->colours);
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
	int	 nlines, ncols, rc = FSL_RC_OK;

	if (view->lines > LINES)
		nlines = view->nlines - (view->lines - LINES);
	else
		nlines = view->nlines + (LINES - view->lines);

	if (view->cols > COLS)
		ncols = view->ncols - (view->cols - COLS);
	else
		ncols = view->ncols + (COLS - view->cols);

	if (wresize(view->window, nlines, ncols) == ERR)
		return RC(FSL_RC_ERROR, "%s", "wresize");
	if (replace_panel(view->panel, view->window) == ERR)
		return RC(FSL_RC_ERROR, "%s", "replace_panel");
	wclear(view->window);

	view->nlines = nlines;
	view->ncols = ncols;
	view->lines = LINES;
	view->cols = COLS;

	if (view->child) {
		view->child->start_col = view_split_start_col(view->start_col);
		if (view->mode == VIEW_SPLIT_HRZN || !view->child->start_col) {
			rc = make_fullscreen(view->child);
			if (view->child->active)
				show_panel(view->child->panel);
			else
				show_panel(view->panel);
		} else {
			rc = make_splitscreen(view->child);
			show_panel(view->child->panel);
		}
	}

	return rc;
}

/*
 * Consume repeatable arguments containing artifact type values used in
 * constructing the SQL query to generate commit records of the specified type
 * for the timeline. n.b. filter_types->values is owned by fcli—do not free.
 * TODO: Enhance to generalise processing of various repeatable args--paths,
 * usernames, branches, etc.--so we can filter on multiples of these values.
 */
static int
fcli_flag_type_arg_cb(fcli_cliflag const *v)
{
	struct artifact_types	*ft = &fnc_init.filter_types;
	const char		*t = *((const char **)v->flagValue);

	/* Valid types: ci, e, f, g, t, w */
	if (t[2] || (t[1] && (*t != 'c' || t[1] != 'i')) || (!t[1] &&
	    (*t != 'e' && *t != 'f' && *t != 'g' && *t != 't' && *t != 'w'))) {
		fnc_init.err = RC(FSL_RC_TYPE, "invalid type: %s", t);
		usage();
		/* NOT REACHED */
	}

	ft->values = fsl_realloc(ft->values, (ft->nitems + 1) * sizeof(char *));
	ft->values[ft->nitems++] = t;

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
			fcli_command cmd = fnc_init.cmd_args[idx];
			if (!fsl_strcmp(fnc_init.cmdarg, cmd.name) ||
			    fcli_cmd_aliascmp(&cmd, fnc_init.cmdarg)) {
				fcli_command_help(&cmd, true, true);
				exit(fcli_end_of_main(fnc_init.err));
			}
		}

	/* Otherwise, output help/usage for all commands. */
	fcli_command_help(fnc_init.cmd_args, true, false);
	fsl_fprintf(f, "  note: %s "
	    "with no args defaults to the timeline command.\n\n",
	    fcli_progname());

	exit(fcli_end_of_main(fnc_init.err));
}

static void
usage_timeline(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " usage: %s timeline [-C|--no-colour] [-R path] [-T tag] "
	    "[-b branch] [-c commit] [-f glob] [-h|--help] [-n n] [-t type] "
	    "[-u user] [-z|--utc] [path]\n"
	    "  e.g.: %s timeline --type ci -u jimmy src/frobnitz.c\n\n",
	    fcli_progname(), fcli_progname());
}

static void
usage_diff(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " usage: %s diff [-C|--no-colour] [-R path] [-h|--help] "
	    "[-i|--invert] [-q|--quiet] [-w|--whitespace] [-x|--context n] "
	    "[artifact1 [artifact2]] [path ...]\n  "
	    "e.g.: %s diff --context 3 d34db33f c0ff33 src/*.c\n\n",
	    fcli_progname(), fcli_progname());
}

static void
usage_tree(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " usage: %s tree [-C|--no-colour] [-R path] [-c commit] [-h|--help]"
	    " [path]\n"
	    "  e.g.: %s tree -c d34dc0d3\n\n" ,
	    fcli_progname(), fcli_progname());
}

static void
usage_blame(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " usage: %s blame [-C|--no-colour] [-R path] [-c commit [-r]] "
	    "[-h|--help] [-n n] path\n"
	    "  e.g.: %s blame -c d34db33f src/foo.c\n\n" ,
	    fcli_progname(), fcli_progname());
}

static void
usage_branch(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " usage: %s branch [-C|--no-colour] [-R path] [-a|--after date] "
	    "[-b|--before date] [-c|--closed] [-h|--help] [-o|--open] "
	    "[-p|--no-private] [-r|--reverse] [-s|--sort order] [glob]\n"
	    "  e.g.: %s branch -b 2020-10-10\n\n" ,
	    fcli_progname(), fcli_progname());
}

static void
usage_config(void)
{
	fsl_fprintf(fnc_init.err ? stderr : stdout,
	    " usage: %s config [-R path] [-h|--help] [--ls] "
	    "[setting [value|--unset]]\n"
	    "  e.g.: %s config FNC_DIFF_COMMIT blue\n\n" ,
	    fcli_progname(), fcli_progname());
}

static int
cmd_diff(fcli_command const *argv)
{
	fsl_cx				*const f = fcli_cx();
	struct fnc_view			*view;
	struct fnc_commit_artifact	*commit = NULL;
	struct fnc_pathlist_head	 paths;
	struct fnc_pathlist_entry	*pe;
	fsl_deck			 d = fsl_deck_empty;
	fsl_stmt			*q = NULL;
	const char			*artifact1 = NULL, *artifact2 = NULL;
	char				*path0 = NULL;
	fsl_id_t			 prid = -1, rid = -1;
	long				 context = DEF_DIFF_CTX;
	int				 rc = FSL_RC_OK;
	unsigned short			 blob = 0;
	enum fnc_diff_type		 diff_type = FNC_DIFF_CKOUT;
	bool				 showmeta = false;

	rc = fcli_process_flags(argv->flags);
	if (rc || (rc = fcli_has_unused_flags(false)))
		return rc;

	TAILQ_INIT(&paths);

	/*
	 * To provide an intuitive UI, use some magic. First, if there's an arg
	 * and it's a symbolic checkin name, take as a checkin artifact. Repeat
	 * for the next arg. If just one is a checkin, diff changes on disk
	 * against it. If neither are checkins, diff changes on disk against the
	 * current checkout. If both are checkins, diff against eachother. Treat
	 * any non-symbol args as paths and try map to a valid repo path or F
	 * card in the checkin(s) deck(s). It's tricky, but provides a smart UI:
	 * fnc diff f1 f2 ... -> diff f{1,2,...} on disk against current ckout
	 * fnc diff sym3 f1 -> diff f1 on disk against f1 found in checkin sym3
	 * fnc diff sym1 sym2 f1 f2 -> diff f{1,2} between checkins sym1 & sym2
	 */
	if (!fsl_sym_to_rid(f, fcli_next_arg(false), FSL_SATYPE_ANY, &prid)) {
		artifact1 = fcli_next_arg(true);
		if (!fsl_rid_is_a_checkin(f, prid))
			++blob;
		if (!fsl_sym_to_rid(f, fcli_next_arg(false), FSL_SATYPE_ANY,
		    &rid)) {
			artifact2 = fcli_next_arg(true);
			diff_type = FNC_DIFF_COMMIT;
			if (!fsl_rid_is_a_checkin(f, rid))
				++blob;
		}
	}
	if (fcli_error()->code == FSL_RC_NOT_FOUND) {
		fcli_err_reset();  /* If args aren't symbols, treat as paths. */
		rc = 0;
	}
	if (blob == 2)
		diff_type = FNC_DIFF_BLOB;
	if (!artifact1 && diff_type != FNC_DIFF_BLOB) {
		artifact1 = "current";
		rc = fsl_sym_to_rid(f, artifact1, FSL_SATYPE_CHECKIN, &prid);
		if (rc || prid < 0) {
			rc = RC(rc, "%s", "fsl_sym_to_rid");
			goto end;
		}
	}
	if (!artifact2 && diff_type != FNC_DIFF_BLOB) {
		fsl_ckout_version_info(f, &rid, NULL);
		if ((rc = fsl_ckout_changes_scan(f)))
			return RC(rc, "%s", "fsl_ckout_changes_scan");
		if (!fsl_strcmp(artifact1, "current") &&
		    !fsl_ckout_has_changes(f)) {
			fsl_fprintf(stdout, "No local changes.\n");
			return rc;
		}
	}
	while (fcli_next_arg(false) && diff_type != FNC_DIFF_BLOB) {
		struct fnc_pathlist_entry *ins;
		char *path, *path_to_diff;
		rc = map_repo_path(&path0);
		path = path0;
		while (path[0] == '/')
			++path;
		if (rc) {
			if (rc != FSL_RC_NOT_FOUND ||
			    (!fsl_strcmp(artifact1, "current") && !artifact2)) {
				rc = RC(rc, "invalid artifact hash: %s", path);
				goto end;
			}
			rc = 0;
			fcli_err_reset();
			/* Path may be valid in tree of specified commit(s). */
			const fsl_card_F *cf = NULL;
			rc = fsl_deck_load_sym(f, &d, artifact1,
			    FSL_SATYPE_CHECKIN);
			if (rc)
				goto end;
			cf = fsl_deck_F_search(&d, path);
			if (cf == NULL) {
				if (!artifact2) {
					rc = RC(FSL_RC_NOT_FOUND,
					    "'%s' not found in tree [%s]", path,
					    artifact1);
					goto end;
				}
				fsl_deck_finalize(&d);
				rc = fsl_deck_load_sym(f, &d, artifact2,
				    FSL_SATYPE_CHECKIN);
				if (rc)
					goto end;
				cf = fsl_deck_F_search(&d, path);
				if (cf == NULL) {
					rc = RC(FSL_RC_NOT_FOUND,
					    "'%s' not found in trees [%s] [%s]",
					    path, artifact1, artifact2);
					goto end;
				}
			}
		}
		path_to_diff = fsl_strdup(path);
		if (path_to_diff == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
		}
		rc = fnc_pathlist_insert(&ins, &paths, path_to_diff, NULL);
		if (rc || ins == NULL /* Duplicate path. */)
			fsl_free(path_to_diff);
		if (rc)
			goto end;
	}

	if (diff_type != FNC_DIFF_BLOB && diff_type != FNC_DIFF_CKOUT) {
		q = fsl_stmt_malloc();
		rc = commit_builder(&commit, rid, q);
		if (rc)
			goto end;
		if (commit->prid == prid)
			showmeta = true;
		else {
			fsl_free(commit->puuid);
			commit->prid = prid;
			commit->puuid = fsl_rid_to_uuid(f, prid);
		}
	} else {
		commit = calloc(1, sizeof(*commit));
		if (commit == NULL) {
			rc = RC(fsl_errno_to_rc(errno, FSL_RC_ERROR),
			    "%s", "calloc");
			goto end;
		}
		commit->prid = prid;
		commit->rid = rid;
		commit->puuid = fsl_rid_to_uuid(f, prid);
		commit->uuid = fsl_rid_to_uuid(f, rid);
		commit->type = fsl_strdup("blob");
		commit->diff_type = diff_type;
	}

	rc = init_curses();
	if (rc)
		goto end;
#ifdef __OpenBSD__
	rc = init_unveil(fsl_cx_db_file_repo(f, NULL),
	    fsl_cx_ckout_dir_name(f, NULL), false);
	if (rc)
		goto end;
#endif

	if (fnc_init.context) {
		if ((rc = strtonumcheck(&context, fnc_init.context, INT_MIN,
		    INT_MAX)))
			goto end;
		context = MIN(MAX_DIFF_CTX, context);
	}

	view = view_open(0, 0, 0, 0, FNC_VIEW_DIFF);
	if (view == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "view_open");
		goto end;
	}

	rc = open_diff_view(view, commit, context, fnc_init.ws,
	    fnc_init.invert, !fnc_init.quiet, NULL, showmeta, &paths);
	if (!rc)
		rc = view_loop(view);
end:
	fsl_free(path0);
	fsl_deck_finalize(&d);
	fsl_stmt_finalize(q);
	if (commit)
		fnc_commit_artifact_close(commit);
	TAILQ_FOREACH(pe, &paths, entry)
		free((char *)pe->path);
	fnc_pathlist_free(&paths);
	return rc;
}

static int
browse_commit_tree(struct fnc_view **new_view, int start_col,
    struct commit_entry *entry, const char *path)
{
	struct fnc_view	*tree_view;
	int		 rc = 0;

	tree_view = view_open(0, 0, 0, start_col, FNC_VIEW_TREE);
	if (tree_view == NULL)
		return RC(FSL_RC_ERROR, "%s", "view_open");

	rc = open_tree_view(tree_view, path, entry->commit->rid);
	if (rc)
		return rc;

	*new_view = tree_view;

	return rc;
}

static int
cmd_tree(fcli_command const *argv)
{
	fsl_cx		*const f = fcli_cx();
	struct fnc_view	*view;
	char		*path = NULL;
	fsl_id_t	 rid;
	int		 rc = 0;

	rc = fcli_process_flags(argv->flags);
	if (rc || (rc = fcli_has_unused_flags(false)))
		goto end;

	rc = map_repo_path(&path);
	if (rc)
		goto end;
	if (fnc_init.sym)
		rc = fsl_sym_to_rid(f, fnc_init.sym, FSL_SATYPE_ANY, &rid);
	else
		fsl_ckout_version_info(f, &rid, NULL);

	if (rc) {
		switch (rc) {
		case FSL_RC_AMBIGUOUS:
			RC(rc, "prefix too ambiguous [%s]",
			    fnc_init.sym);
			goto end;
		case FSL_RC_NOT_A_REPO:
			RC(rc, "%s tree needs a local checkout",
			    fcli_progname());
			goto end;
		case FSL_RC_NOT_FOUND:
			RC(rc, "invalid symbolic checkin name [%s]",
			    fnc_init.sym);
			goto end;
		case FSL_RC_MISUSE:
			/* FALL THROUGH */
		default:
			goto end;
		}
	}

	/* In 'fnc tree -R repo.db [path]' case, use the latest checkin. */
	if (rid == 0) {
		rc = fsl_sym_to_rid(f, "tip", FSL_SATYPE_CHECKIN, &rid);
		if (rc)
			goto end;
	} else if (!fsl_rid_is_a_checkin(f, rid)) {
		rc = RC(FSL_RC_TYPE, "%s tree requires check-in artifact",
		    fcli_progname());
		goto end;
	}

	rc = init_curses();
	if (rc)
		goto end;
#ifdef __OpenBSD__
	rc = init_unveil(fsl_cx_db_file_repo(f, NULL),
	    fsl_cx_ckout_dir_name(f, NULL), false);
	if (rc)
		goto end;
#endif

	view = view_open(0, 0, 0, 0, FNC_VIEW_TREE);
	if (view == NULL) {
		RC(FSL_RC_ERROR, "%s", "view_open");
		goto end;
	}

	rc = open_tree_view(view, path, rid);
	if (!rc)
		rc = view_loop(view);
end:
	fsl_free(path);
	return rc;
}

static int
open_tree_view(struct fnc_view *view, const char *path, fsl_id_t rid)
{
	fsl_cx				*const f = fcli_cx();
	struct fnc_tree_view_state	*s = &view->state.tree;
	int				 rc = 0;

	TAILQ_INIT(&s->parents);
	s->show_id = false;
	s->colour = !fnc_init.nocolour && has_colors();
	s->rid = rid;
	s->commit_id = fsl_rid_to_uuid(f, rid);
	if (s->commit_id == NULL)
		return RC(FSL_RC_AMBIGUOUS, "%s", "fsl_rid_to_uuid");

	/*
	 * Construct tree of entire repository from which all (sub)tress will
	 * be derived. This object will be released when this view closes.
	 */
	rc = create_repository_tree(&s->repo, &s->commit_id, s->rid);
	if (rc)
		goto end;

	/*
	 * Open the initial root level of the repository tree now. Subtrees
	 * opened during traversal are built and destroyed on demand.
	 */
	rc = tree_builder(s->repo, &s->root, "/");
	if (rc)
		goto end;
	s->tree = s->root;
	/*
	 * If user has supplied a path arg (i.e., fnc tree path/in/repo), or
	 * has selected a commit from an 'fnc timeline path/in/repo' command,
	 * walk the path and open corresponding (sub)tree objects now.
	 */
	if (!fnc_path_is_root_dir(path)) {
		rc = walk_tree_path(s, s->repo, &s->root, path);
		if (rc)
			goto end;
	}


	if ((s->tree_label = fsl_mprintf("checkin %s", s->commit_id)) == NULL) {
		rc = RC(FSL_RC_RANGE, "%s", "fsl_mprintf");
		goto end;
	}

	s->first_entry_onscreen = &s->tree->entries[0];
	s->selected_entry = &s->tree->entries[0];

	if (s->colour) {
		STAILQ_INIT(&s->colours);
		rc = set_colours(&s->colours, FNC_VIEW_TREE);
		if (rc)
			goto end;
	}

	view->show = show_tree_view;
	view->input = tree_input_handler;
	view->close = close_tree_view;
	view->search_init = tree_search_init;
	view->search_next = tree_search_next;
end:
	if (rc)
		close_tree_view(view);
	return rc;
}

/*
 * Decompose the supplied path into its constituent components, then build,
 * open and visit each subtree segment on the way to the requested entry.
 */
static int
walk_tree_path(struct fnc_tree_view_state *s, struct fnc_repository_tree *repo,
    struct fnc_tree_object **root, const char *path)
{
	struct fnc_tree_object	*tree = NULL;
	const char		*p;
	char			*slash, *subpath = NULL;
	int			 rc = 0;

	/* Find each slash and open preceding directory segment as a tree. */
	p = path;
	while (*p) {
		struct fnc_tree_entry	*te;
		char			*te_name;

		while (p[0] == '/')
			p++;

		slash = strchr(p, '/');
		if (slash == NULL)
			te_name = fsl_strdup(p);
		else
			te_name = fsl_strndup(p, slash - p);
		if (te_name == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			break;
		}

		te = find_tree_entry(s->tree, te_name, fsl_strlen(te_name));
		if (te == NULL) {
			rc = RC(FSL_RC_NOT_FOUND, "find_tree_entry(%s)",
			    te_name);
			fsl_free(te_name);
			break;
		}
		fsl_free(te_name);

		s->first_entry_onscreen = s->selected_entry = te;
		if (!S_ISDIR(s->selected_entry->mode))
			break;	/* If a file, jump to this entry. */

		slash = strchr(p, '/');
		if (slash)
			subpath = fsl_strndup(path, slash - path);
		else
			subpath = fsl_strdup(path);
		if (subpath == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			break;
		}

		rc = tree_builder(repo, &tree, subpath + 1 /* Leading slash */);
		if (rc)
			break;
		rc = visit_subtree(s, tree);
		if (rc) {
			fnc_object_tree_close(tree);
			break;
		}

		if (slash == NULL)
			break;
		fsl_free(subpath);
		subpath = NULL;
		p = slash;
	}

	fsl_free(subpath);
	return rc;
}

/*
 * This routine constructs the repository tree, repo, which is a DLL; from this
 * tree, all displayed (sub)trees are derived. File paths are extracted from F
 * cards of the checkin identified by id referenced in the repo database by rid.
 */
static int
create_repository_tree(struct fnc_repository_tree **repo, fsl_uuid_str *id,
    fsl_id_t rid)
{
	fsl_cx				*const f = fcli_cx();
	struct fnc_repository_tree	*ptr;
	fsl_deck			 d = fsl_deck_empty;
	const fsl_card_F		*cf = NULL;
	int				 rc = 0;

	ptr = fsl_malloc(sizeof(struct fnc_repository_tree));
	if (ptr == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_malloc");
	memset(ptr, 0, sizeof(struct fnc_repository_tree));

	rc = fsl_deck_load_rid(f, &d, rid, FSL_SATYPE_CHECKIN);
	if (rc)
		return RC(rc, "fsl_deck_load_rid(%d) [%s]", rid, id);
	rc = fsl_deck_F_rewind(&d);
	if (rc)
		goto end;
	rc = fsl_deck_F_next(&d, &cf);
	if (rc)
		goto end;

	while (cf) {
		char		*filename = NULL, *uuid = NULL;
		fsl_time_t	 mtime;

		filename = fsl_strdup(cf->name);
		if (filename == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
		}
		uuid = fsl_strdup(cf->uuid);
		if (uuid == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
		}
		rc = fsl_mtime_of_F_card(f, rid, cf, &mtime);
		if (!rc)
			rc = link_tree_node(ptr, filename, uuid,
			    fsl_unix_to_julian(mtime));
		fsl_free(filename);
		fsl_free(uuid);
		if (!rc)
			rc = fsl_deck_F_next(&d, &cf);
		if (rc)
			goto end;
	}
end:
	fsl_deck_finalize(&d);
	*repo = ptr;
	return rc;
}

/*
 * This routine constructs the (sub)trees that are displayed. The directory dir
 * and its contents form a subtree, which is an array of tree entries copied
 * from DLL nodes in repo and stored in tree. This routine is called for each
 * directory that is displayed as a tree.
 */
static int
tree_builder(struct fnc_repository_tree *repo, struct fnc_tree_object **tree,
    const char *dir)
{
	struct fnc_tree_entry		*te = NULL;
	struct fnc_repo_tree_node	*tn = NULL;
	int				 i = 0;

	*tree = NULL;
	*tree = fsl_malloc(sizeof(**tree));
	if (*tree == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_malloc");
	memset(*tree, 0, sizeof(**tree));

	/* Count how many elements will comprise the tree to be allocated. */
	for(tn = repo->head; tn; tn = tn->next) {
		if ((!tn->parent_dir && fsl_strcmp(dir, "/")) ||
		    (tn->parent_dir && fsl_strcmp(dir, tn->parent_dir->path)))
			continue;
		++i;
	}
	(*tree)->entries = calloc(i, sizeof(struct fnc_tree_entry));
	if ((*tree)->entries == NULL)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR), "%s", "calloc");
	/* Construct the tree to be displayed. */
	for(tn = repo->head, i = 0; tn; tn = tn->next) {
		if ((!tn->parent_dir && fsl_strcmp(dir, "/")) ||
		    (tn->parent_dir && fsl_strcmp(dir, tn->parent_dir->path)))
			continue;
		te = &(*tree)->entries[i];
		te->mode = tn->mode;
		te->mtime = tn->mtime;
		te->basename = fsl_strdup(tn->basename);
		if (te->basename == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		te->path = fsl_strdup(tn->path);
		if (te->path == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		te->uuid = fsl_strdup(tn->uuid);
		if (te->uuid == NULL && !S_ISDIR(te->mode))
			return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		te->idx = i++;
	}
	(*tree)->nentries = i;

	return 0;
}

#if 0
static void
delete_tree_node(struct fnc_tree_entry **head, struct fnc_tree_entry *del)
{
	struct fnc_tree_entry *temp = *head, *prev;

	if (temp == del) {
		*head = temp->next;
		fsl_free(temp);
		return;
	}

	while (temp != NULL && temp != del) {
		prev = temp;
		temp = temp->next;
	}

	if (temp == NULL)
		return;

	prev->next = temp->next;

	fsl_free(temp);
}
#endif

/*
 * This routine inserts nodes into the doubly-linked repository tree. Each
 * path component of path (i.e., tokens delimited by '/') becomes a node in
 * tree. The final path component of each segment is the node's .basename, and
 * its full repository relative path its .path. All files in a given directory
 * will comprise the directory node's .children list, and each file node's
 * .sibling list; said directory will be each file node's .parent_dir. The
 * elements of each requested tree will be identified by the node's .parent_dir;
 * that is, each node with the same parent_dir will be an entry in the same tree
 *   tree    The repository tree into which nodes are inserted
 *   path    The repository relative pathname of the versioned file
 *   uuid    The SHA hash of the file
 *   mtime   Modification time of the file
 * Returns 0 on success, non-zero on error.
 */
static int
link_tree_node(struct fnc_repository_tree *tree, const char *path,
    const char *uuid, double mtime)
{
	struct fnc_repo_tree_node	*parent_dir;
	fsl_buffer			 buf = fsl_buffer_empty;
	struct stat			 s;
	int				 i, rc = 0;

	parent_dir = tree->tail;
	while (parent_dir != 0 &&
	    (strncmp(parent_dir->path, path, parent_dir->pathlen) != 0 ||
	    path[parent_dir->pathlen] != '/'))
		parent_dir = parent_dir->parent_dir;

	i = parent_dir ? parent_dir->pathlen + 1 : 0;

	while (path[i]) {
		struct fnc_repo_tree_node	*tn;
		int				 nodesz, slash = i;

		/* Find slash to demarcate each path component. */
		while (path[i] && path[i] != '/')
			i++;
		nodesz = sizeof(*tn) + i + 1;

		/*
		 * If not at end of path string, node is a directory so don't
		 * allocate space for the hash.
		 */
		if (uuid != 0 && path[i] == '\0')
			nodesz += FSL_STRLEN_K256 + 1; /* NUL */
		tn = fsl_malloc(nodesz);
		if (tn == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_malloc");
		memset(tn, 0, sizeof(*tn));

		tn->path = (char *)&tn[1];
		memcpy(tn->path, path, i);
		tn->path[i] = '\0';
		tn->pathlen = i;

		if (uuid != 0 && path[i] == '\0') {
			tn->uuid = tn->path + i + 1;
			memcpy(tn->uuid, uuid, fsl_strlen(uuid) + 1);
		}

		tn->basename = tn->path + slash;

		/* Insert node into DLL or make it the head if first. */
		if (tree->tail) {
			tree->tail->next = tn;
			tn->prev = tree->tail;
		} else
			tree->head = tn;

		tree->tail = tn;
		tn->parent_dir = parent_dir;
		if (parent_dir) {
			if (parent_dir->children)
				parent_dir->lastchild->sibling = tn;
			else
				parent_dir->children = tn;
			tn->nparents = parent_dir->nparents + 1;
			parent_dir->lastchild = tn;
		} else {
			if (tree->rootail)
				tree->rootail->sibling = tn;
			tree->rootail = tn;
		}

		tn->mtime = mtime;
		while (path[i] == '/')	/* Consume slashes. */
			++i;
		parent_dir = tn;

		/* Stat path for tree display features. */
		rc = fsl_file_canonical_name2(fcli_cx()->ckout.dir, tn->path,
		    &buf, false);
		if (rc)
			goto end;
		if (lstat(fsl_buffer_cstr(&buf), &s) == -1) {
			if (errno == ENOENT)
				tn->mode = (!fsl_strcmp(tn->path, path) &&
				    tn->uuid) ? S_IFREG : S_IFDIR;
			else {
				rc = RC(fsl_errno_to_rc(errno, FSL_RC_ACCESS),
				    "lstat(%s)", fsl_buffer_cstr(&buf));
				goto end;
			}
		} else
			tn->mode = s.st_mode;
		fsl_buffer_reuse(&buf);
	}

	while (parent_dir && parent_dir->parent_dir) {
		if (parent_dir->parent_dir->mtime < parent_dir->mtime)
			parent_dir->parent_dir->mtime = parent_dir->mtime;
		parent_dir = parent_dir->parent_dir;
	}
end:
	fsl_buffer_clear(&buf);
	return rc;
}

static int
show_tree_view(struct fnc_view *view)
{
	struct fnc_tree_view_state	*s = &view->state.tree;
	char				*treepath;
	int				 rc = 0;

	rc = tree_entry_path(&treepath, &s->parents, NULL);
	if (rc)
		return rc;

	rc = draw_tree(view, treepath);
	fsl_free(treepath);
	drawborder(view);

	return rc;
}

/*
 * Construct absolute repository path of the currently selected tree entry to
 * display in the tree view header, or pass to open_timeline_view() to construct
 * a timeline of all commits modifying path.
 */
static int
tree_entry_path(char **path, struct fnc_parent_trees *parents,
    struct fnc_tree_entry *te)
{
	struct fnc_parent_tree	*pt;
	size_t			 len = 2;  /* Leading slash and NUL. */
	int			 rc = 0;

	TAILQ_FOREACH(pt, parents, entry)
		len += strlen(pt->selected_entry->basename) + 1 /* slash */;
	if (te)
		len += strlen(te->basename);

	*path = calloc(1, len);
	if (path == NULL)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR), "%s", "calloc");

	(*path)[0] = '/';  /* Make it absolute from the repository root. */
	pt = TAILQ_LAST(parents, fnc_parent_trees);
	while (pt) {
		const char *name = pt->selected_entry->basename;
		if (strlcat(*path, name, len) >= len) {
			rc = RC(FSL_RC_RANGE, "strlcat(%s, %s, %d)",
			    *path, name, len);
			goto end;
		}
		if (strlcat(*path, "/", len) >= len) {
			rc = RC(FSL_RC_RANGE, "strlcat(%s, \"/\", %d)",
			    *path, len);
			goto end;
		}
		pt = TAILQ_PREV(pt, fnc_parent_trees, entry);
	}
	if (te) {
		if (strlcat(*path, te->basename, len) >= len) {
			rc = RC(FSL_RC_RANGE, "strlcat(%s, %s, %d)",
			    *path, te->basename, len);
			goto end;
		}
	}
end:
	if (rc) {
		fsl_free(*path);
		*path = NULL;
	}
	return rc;
}

/*
 * Draw the currently visited tree. Headline view with the checkin's SHA hash,
 * and write subheader comprised of the tree path. Lexicographically order nodes
 * (cf. ls(1)) and postfix with identifier corresponding to the file mode as
 * returned by lstat(2) such that the tree takes the following form:
 *
 *  checkin COMMIT-HASH
 *  /absolute/repository/tree/path/
 *
 *   ..
 *   dir/
 *   executable*
 *   regularfile
 *   symlink@ -> /path/to/source/file
 *
 * If the 'i' key binding is entered, prefix each versioned file with its
 * SHA{1,3} hash. Directories, however, have no such hash UUID to display.
 */
static int
draw_tree(struct fnc_view *view, const char *treepath)
{
	struct fnc_tree_view_state	*s = &view->state.tree;
	struct fnc_tree_entry		*te;
	struct fnc_colour		*c = NULL;
	wchar_t				*wcstr;
	int				 rc = 0;
	int				 wstrlen, n, idx, nentries;
	int				 limit = view->nlines;
	uint_fast8_t			 hashlen = FSL_UUID_STRLEN_MIN;

	s->ndisplayed = 0;
	werase(view->window);
	if (limit == 0)
		return rc;

	/* Write (highlighted) headline (if view is active in splitscreen). */
	rc = formatln(&wcstr, &wstrlen, s->tree_label, view->ncols, 0);
	if (rc)
		return rc;
	if (screen_is_shared(view))
		wattron(view->window, A_REVERSE);
	if (s->colour)
		c = get_colour(&s->colours, FNC_COLOUR_COMMIT);
	if (c)
		wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
	waddwstr(view->window, wcstr);
	while (wstrlen < view->ncols) {
		waddch(view->window, ' ');
		++wstrlen;
	}
	if (c)
		wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
	if (screen_is_shared(view))
		wattroff(view->window, A_REVERSE);
	fsl_free(wcstr);
	wcstr = NULL;
	if (--limit <= 0)
		return rc;

	/* Write this (sub)tree's absolute repository path subheader. */
	rc = formatln(&wcstr, &wstrlen, treepath, view->ncols, 0);
	if (rc)
		return rc;
	waddwstr(view->window, wcstr);
	fsl_free(wcstr);
	wcstr = NULL;
	if (wstrlen < view->ncols - 1)
		waddch(view->window, '\n');
	if (--limit <= 0)
		return rc;
	waddch(view->window, '\n');
	if (--limit <= 0)
		return rc;

	/* Write parent dir entry (i.e., "..") if top of the tree is in view. */
	if (s->first_entry_onscreen == NULL) {
		te = &s->tree->entries[0];
		if (s->selected_idx == 0) {
			if (view->active)
				wattr_on(view->window, A_REVERSE, NULL);
			s->selected_entry = NULL;
		}
		waddstr(view->window, "  ..\n");
		if (s->selected_idx == 0 && view->active)
			wattr_off(view->window, A_REVERSE, NULL);
		++s->ndisplayed;
		if (--limit <= 0)
			return rc;
		n = 1;
	} else {
		n = 0;
		te = s->first_entry_onscreen;
	}

	nentries = s->tree->nentries;
	for (idx = 0; idx < nentries; ++idx)	/* Find max hash length. */
		if (hashlen < fsl_strlen(s->tree->entries[idx].uuid))
			hashlen = fsl_strlen(s->tree->entries[idx].uuid);
	/* Iterate and write tree nodes postfixed with path type identifier. */
	for (idx = te->idx; idx < nentries; ++idx) {
		char		*line = NULL, *idstr = NULL, *targetlnk = NULL;
		char		 iso8601[ISO8601_TIMESTAMP] = {0};
		const char	*modestr = "";
		mode_t		 mode;

		if (idx < 0 || idx >= s->tree->nentries)
			return 0;
		te = &s->tree->entries[idx];
		mode = te->mode;

		if (s->show_id) {
			idstr = fsl_strdup(te->uuid);
			/* Directories don't have UUIDs; pad with "..." dots. */
			if (idstr == NULL && !S_ISDIR(mode))
				return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			/* If needed, pad SHA1 hash to align w/ SHA3 hashes. */
			if (idstr == NULL || fsl_strlen(idstr) < hashlen) {
				char buf[hashlen], pad = '.';
				buf[hashlen] = '\0';
				idstr = fsl_mprintf("%s%s", idstr ? idstr : "",
				    (char *)memset(buf, pad,
				    hashlen - fsl_strlen(idstr)));
				if (idstr == NULL)
					return RC(FSL_RC_RANGE,
					    "%s", "fsl_mprintf");
				idstr[hashlen] = '\0';
				/* idstr = fsl_mprintf("%*c", hashlen, ' '); */
			}
		}
		if (S_ISLNK(mode)) {
			fsl_size_t	ch;

			rc = tree_entry_get_symlink_target(&targetlnk, te);
			if (rc) {
				fsl_free(idstr);
				return rc;
			}
			for (ch = 0; ch < fsl_strlen(targetlnk); ++ch) {
				if (!isprint((unsigned char)targetlnk[ch]))
					targetlnk[ch] = '?';
			}
			modestr = "@";
		}
		else if (S_ISDIR(mode))
			modestr = "/";
		else if (mode & S_IXUSR)
			modestr = "*";
		if (s->show_date) {
			char *t;
			if (fsl_julian_to_iso8601(te->mtime, iso8601, false))
				*(t = strchr(iso8601, 'T')) = ' ';
			else
				rc = FSL_RC_ERROR;
		}
		line = fsl_mprintf("%s%s%.*s  %s%s%s%s", idstr ? idstr : "",
		    (*iso8601 && idstr) ?  "  " : "", ISO8601_DATE_HHMM,
		    *iso8601 ? iso8601 : "", te->basename, modestr,
		    targetlnk ? " -> ": "", targetlnk ? targetlnk : "");
		fsl_free(idstr);
		fsl_free(targetlnk);
		if (rc || line == NULL)
			return RC(rc ? rc : FSL_RC_RANGE, "%s",
			    rc ? "fsl_julian_to_iso8601" : "fsl_mprintf");
		rc = formatln(&wcstr, &wstrlen, line, view->ncols, 0);
		if (rc) {
			fsl_free(line);
			break;
		}
		if (n == s->selected_idx) {
			if (view->active)
				wattr_on(view->window, A_REVERSE, NULL);
			s->selected_entry = te;
		}
		if (s->colour)
			c = match_colour(&s->colours, line);
		if (c)
			wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
		waddwstr(view->window, wcstr);
		if (c)
			wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
		if (wstrlen < view->ncols - 1)
			waddch(view->window, '\n');
		if (n == s->selected_idx && view->active)
			wattr_off(view->window, A_REVERSE, NULL);
		fsl_free(line);
		fsl_free(wcstr);
		wcstr = NULL;
		++n;
		++s->ndisplayed;
		s->last_entry_onscreen = te;
		if (--limit <= 0)
			break;
	}

	return rc;
}

static int
tree_entry_get_symlink_target(char **targetlnk, struct fnc_tree_entry *te)
{
	struct stat	 s;
	fsl_buffer	 fb = fsl_buffer_empty;
	char		*buf = NULL;
	ssize_t		 nbytes, bufsz;
	int		 rc = 0;

	*targetlnk = NULL;

	fsl_file_canonical_name2(fcli_cx()->ckout.dir, te->path, &fb, false);
	if (lstat(fsl_buffer_cstr(&fb), &s) == -1) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_ACCESS), "lstat(%s)",
		    fsl_buffer_cstr(&fb));
		goto end;
	}

	bufsz = s.st_size ? (s.st_size + 1 /* NUL */) : PATH_MAX;
	buf = fsl_malloc(bufsz);
	if (buf == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "fsl_malloc");
		goto end;
	}

	nbytes = readlink(fsl_buffer_cstr(&fb), buf, bufsz);
	if (nbytes == -1) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_IO), "readlink(%s)",
		    fsl_buffer_cstr(&fb));
		goto end;
	}
	buf[nbytes] = '\0';	/* readlink() does _not_ NUL terminate */
end:
	fsl_buffer_clear(&fb);
	if (rc)
		fsl_free(buf);
	*targetlnk = buf;
	return rc;
	/*
	 * XXX Not sure if we should rely on fossil(1) populating symlinks
	 * with the path of the target source file to obtain the target link.
	 */
	/* fsl_cx		*f = fcli_cx(); */
	/* fsl_buffer	 blob = fsl_buffer_empty; */
	/* fsl_id_t	 rid; */

	/* if (!((te->mode & (S_IFDIR | S_IFLNK)) == S_IFLNK)) */
	/* 	return RC(FSL_RC_TYPE, "file not symlink [%s]", te->path); */
	/* rc = fsl_sym_to_rid(f, te->uuid, FSL_SATYPE_ANY, &rid); */
	/* if (!rc) */
	/* 	rc = fsl_content_blob(f, rid, &blob); */
	/* if (rc) */
	/* 	return rc; */

	/* *targetlnk = fsl_strdup(fsl_buffer_str(&blob)); */
	/* fsl_buffer_clear(&blob); */
}

static int
tree_input_handler(struct fnc_view **new_view, struct fnc_view *view, int ch)
{
	struct fnc_view			*branch_view, *timeline_view;
	struct fnc_tree_view_state	*s = &view->state.tree;
	struct fnc_tl_thread_cx		*tcx = NULL;
	struct fnc_tree_entry		*te;
	int				 n, start_col = 0, rc = 0;

	switch (ch) {
	case 'b':
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		branch_view = view_open(view->nlines, view->ncols,
		    view->start_ln, start_col, FNC_VIEW_BRANCH);
		if (branch_view == NULL)
			return RC(FSL_RC_ERROR, "%s", "view_open");
		rc = open_branch_view(branch_view, BRANCH_LS_OPEN_CLOSED, NULL,
		    0, 0);
		if (rc) {
			view_close(branch_view);
			return rc;
		}
		tcx = fcli_cx()->clientState.state;
		tcx->needs_reset = true;
		view->active = false;
		branch_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, branch_view);
			view->focus_child = true;
		} else
			*new_view = branch_view;
		break;
	case 'c':
		s->colour = !s->colour;
		break;
	case 'd':
		s->show_date = !s->show_date;
		break;
	case 'i':
		s->show_id = !s->show_id;
		break;
	case 't':
		if (!s->selected_entry)
			break;
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		rc = timeline_tree_entry(&timeline_view, start_col, s);
		if (rc)
			return rc;
		view->active = false;
		timeline_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, timeline_view);
			view->focus_child = true;
		} else
			*new_view = timeline_view;
		break;
	case 'g':
		if (!fnc_home(view))
			break;
		/* FALL THROUGH */
	case KEY_HOME:
		s->selected_idx = 0;
		if (s->tree == s->root)
			s->first_entry_onscreen = &s->tree->entries[0];
		else
			s->first_entry_onscreen = NULL;
		break;
	case KEY_END:
	case 'G':
		s->selected_idx = 0;
		te = &s->tree->entries[s->tree->nentries - 1];
		for (n = 0; n < view->nlines - 3; ++n) {
			if (te == NULL) {
				if(s->tree != s->root) {
					s->first_entry_onscreen = NULL;
					++n;
				}
				break;
			}
			s->first_entry_onscreen = te;
			te = get_tree_entry(s->tree, te->idx - 1);
		}
		if (n > 0)
			s->selected_idx = n - 1;
		break;
	case KEY_UP:
	case 'k':
		if (s->selected_idx > 0) {
			--s->selected_idx;
			break;
		}
		tree_scroll_up(s, 1);
		break;
	case KEY_PPAGE:
	case CTRL('b'):
		if (s->tree == s->root) {
			if (&s->tree->entries[0] == s->first_entry_onscreen)
				s->selected_idx = 0;
		} else {
			if (s->first_entry_onscreen == NULL)
				s->selected_idx = 0;
		}
		tree_scroll_up(s, MAX(0, view->nlines - 3));
		break;
	case KEY_DOWN:
	case 'j':
		if (s->selected_idx < s->ndisplayed - 1) {
			++s->selected_idx;
			break;
		}
		if (get_tree_entry(s->tree, s->last_entry_onscreen->idx + 1)
		    == NULL)
			break;	/* Reached last entry. */
		tree_scroll_down(s, 1);
		break;
	case KEY_NPAGE:
	case CTRL('f'):
		if (get_tree_entry(s->tree, s->last_entry_onscreen->idx + 1)
		    == NULL) {
			/*
			 * When the last entry on screen is the last node in the
			 * tree move cursor to it instead of scrolling the view.
			 */
			if (s->selected_idx < s->ndisplayed - 1)
				s->selected_idx = s->ndisplayed - 1;
			break;
		}
		tree_scroll_down(s, view->nlines - 3);
		break;
	case KEY_BACKSPACE:
	case KEY_ENTER:
	case KEY_LEFT:
	case KEY_RIGHT:
	case '\r':
	case 'h':
	case 'l':
		/*
		 * h/backspace/arrow-left: return to parent dir irrespective
		 * of selected entry type (unless already at root).
		 * l/arrow-right: move into selected dir entry.
		 */
		if (ch != KEY_RIGHT && ch != 'l' && (s->selected_entry == NULL
		    || ch == 'h' || ch == KEY_BACKSPACE || ch == KEY_LEFT)) {
			struct fnc_parent_tree	*parent;
			/* h/backspace/left-arrow pressed or ".." selected. */
			if (s->tree == s->root)
				break;
			parent = TAILQ_FIRST(&s->parents);
			TAILQ_REMOVE(&s->parents, parent,
			    entry);
			fnc_object_tree_close(s->tree);
			s->tree = parent->tree;
			s->first_entry_onscreen = parent->first_entry_onscreen;
			s->selected_entry = parent->selected_entry;
			s->selected_idx = parent->selected_idx;
			fsl_free(parent);
		} else if (s->selected_entry != NULL &&
		    S_ISDIR(s->selected_entry->mode)) {
			struct fnc_tree_object	*subtree = NULL;
			rc = tree_builder(s->repo, &subtree,
			    s->selected_entry->path);
			if (rc)
				break;
			rc = visit_subtree(s, subtree);
			if (rc) {
				fnc_object_tree_close(subtree);
				break;
			}
		} else if (s->selected_entry != NULL &&
		    S_ISREG(s->selected_entry->mode)) {
			struct fnc_view *blame_view;
			int start_col = view_is_parent(view) ?
			    view_split_start_col(view->start_col) : 0;

			rc = blame_tree_entry(&blame_view, start_col,
			    s->selected_entry, &s->parents, s->commit_id);
			if (rc)
				break;
			view->active = false;
			blame_view->active = true;
			if (view_is_parent(view)) {
				rc = view_close_child(view);
				if (rc)
					return rc;
				view_set_child(view, blame_view);
				view->focus_child = true;
			} else
				*new_view = blame_view;
		}
		break;
	case KEY_RESIZE:
		if (view->nlines >= 4 && s->selected_idx >= view->nlines - 3)
			s->selected_idx = view->nlines - 4;
		break;
	default:
		break;
	}

	return rc;
}

static int
timeline_tree_entry(struct fnc_view **new_view, int start_col,
    struct fnc_tree_view_state *s)
{
	struct fnc_view	*timeline_view;
	char		*path;
	int		 rc = 0;

	*new_view = NULL;

	timeline_view = view_open(0, 0, 0, start_col, FNC_VIEW_TIMELINE);
	if (timeline_view == NULL)
		return RC(FSL_RC_ERROR, "%s", "view_open");

	/* Construct repository relative path for timeline query. */
	rc = tree_entry_path(&path, &s->parents, s->selected_entry);
	if (rc)
		return rc;

	rc = open_timeline_view(timeline_view, s->rid, path, NULL);
	if (!rc)
		*new_view = timeline_view;

	fsl_free(path);
	return rc;
}

static void
tree_scroll_up(struct fnc_tree_view_state *s, int maxscroll)
{
	struct fnc_tree_entry	*te;
	int			 isroot, i = 0;

	isroot = s->tree == s->root;

	if (s->first_entry_onscreen == NULL)
		return;

	te = get_tree_entry(s->tree, s->first_entry_onscreen->idx - 1);
	while (i++ < maxscroll) {
		if (te == NULL) {
			if (!isroot)
				s->first_entry_onscreen = NULL;
			break;
		}
		s->first_entry_onscreen = te;
		te = get_tree_entry(s->tree, te->idx - 1);
	}
}

static void
tree_scroll_down(struct fnc_tree_view_state *s, int maxscroll)
{
	struct fnc_tree_entry	*next, *last;
	int			 n = 0;

	if (s->first_entry_onscreen)
		next = get_tree_entry(s->tree,
		    s->first_entry_onscreen->idx + 1);
	else
		next = &s->tree->entries[0];

	last = s->last_entry_onscreen;
	while (next && last && n++ < maxscroll) {
		last = get_tree_entry(s->tree, last->idx + 1);
		if (last) {
			s->first_entry_onscreen = next;
			next = get_tree_entry(s->tree, next->idx + 1);
		}
	}
}

static int
visit_subtree(struct fnc_tree_view_state *s, struct fnc_tree_object *subtree)
{
	struct fnc_parent_tree	*parent;

	parent = calloc(1, sizeof(*parent));
	if (parent == NULL)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR), "%s", "calloc");

	parent->tree = s->tree;
	parent->first_entry_onscreen = s->first_entry_onscreen;
	parent->selected_entry = s->selected_entry;
	parent->selected_idx = s->selected_idx;
	TAILQ_INSERT_HEAD(&s->parents, parent, entry);
	s->tree = subtree;
	s->selected_idx = 0;
	s->first_entry_onscreen = NULL;

	return 0;
}

static int
blame_tree_entry(struct fnc_view **new_view, int start_col,
    struct fnc_tree_entry *te, struct fnc_parent_trees *parents,
    fsl_uuid_str commit_id)
{
	struct fnc_view	*blame_view;
	char		*path;
	int		 rc = 0;

	*new_view = NULL;

	rc = tree_entry_path(&path, parents, te);
	if (rc)
		return rc;

	blame_view = view_open(0, 0, 0, start_col, FNC_VIEW_BLAME);
	if (blame_view == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "view_open");
		goto end;
	}

	rc = open_blame_view(blame_view, path, commit_id, 0, 0);
	if (rc)
		view_close(blame_view);
	else
		*new_view = blame_view;
end:
	fsl_free(path);
	return rc;
}

static int
tree_search_init(struct fnc_view *view)
{
	struct fnc_tree_view_state *s = &view->state.tree;

	s->matched_entry = NULL;
	return 0;
}

static int
tree_search_next(struct fnc_view *view)
{
	struct fnc_tree_view_state	*s = &view->state.tree;
	struct fnc_tree_entry		*te = NULL;
	int				 rc = 0;

	if (view->searching == SEARCH_DONE) {
		view->search_status = SEARCH_CONTINUE;
		return rc;
	}

	if (s->matched_entry) {
		if (view->searching == SEARCH_FORWARD) {
			if (s->selected_entry)
				te = &s->tree->entries[s->selected_entry->idx
				    + 1];
			else
				te = &s->tree->entries[0];
		} else {
			if (s->selected_entry == NULL)
				te = &s->tree->entries[s->tree->nentries - 1];
			else
				te = &s->tree->entries[s->selected_entry->idx
				    - 1];
		}
	} else {
		if (view->searching == SEARCH_FORWARD)
			te = &s->tree->entries[0];
		else
			te = &s->tree->entries[s->tree->nentries - 1];
	}

	while (1) {
		if (te == NULL) {
			if (s->matched_entry == NULL) {
				view->search_status = SEARCH_CONTINUE;
				return rc;
			}
			if (view->searching == SEARCH_FORWARD)
				te = &s->tree->entries[0];
			else
				te = &s->tree->entries[s->tree->nentries - 1];
		}

		if (match_tree_entry(te, &view->regex)) {
			view->search_status = SEARCH_CONTINUE;
			s->matched_entry = te;
			break;
		}

		if (view->searching == SEARCH_FORWARD)
			te = &s->tree->entries[te->idx + 1];
		else
			te = &s->tree->entries[te->idx - 1];
	}

	if (s->matched_entry) {
		s->first_entry_onscreen = s->matched_entry;
		s->selected_idx = 0;
	}

	return rc;
}

static int
match_tree_entry(struct fnc_tree_entry *te, regex_t *regex)
{
	regmatch_t regmatch;

	return regexec(regex, te->basename, 1, &regmatch, 0) == 0;
}

struct fnc_tree_entry *
get_tree_entry(struct fnc_tree_object *tree, int i)
{
	if (i < 0 || i >= tree->nentries)
		return NULL;

	return &tree->entries[i];
}

/* Find entry in tree with basename name. */
static struct fnc_tree_entry *
find_tree_entry(struct fnc_tree_object *tree, const char *name, size_t len)
{
	int	idx;

	/* Entries are sorted in strcmp() order. */
	for (idx = 0; idx < tree->nentries; ++idx) {
		struct fnc_tree_entry *te = &tree->entries[idx];
		int cmp = strncmp(te->basename, name, len);
		if (cmp < 0)
			continue;
		if (cmp > 0)
			break;
		if (te->basename[len] == '\0')
			return te;
	}
	return NULL;
}

static int
close_tree_view(struct fnc_view *view)
{
	struct fnc_tree_view_state	*s = &view->state.tree;

	free_colours(&s->colours);

	fsl_free(s->tree_label);
	s->tree_label = NULL;
	fsl_free(s->commit_id);
	s->commit_id = NULL;

	while (!TAILQ_EMPTY(&s->parents)) {
		struct fnc_parent_tree *parent;
		parent = TAILQ_FIRST(&s->parents);
		TAILQ_REMOVE(&s->parents, parent, entry);
		if (parent->tree != s->root)
			fnc_object_tree_close(parent->tree);
		fsl_free(parent);

	}

	if (s->tree != NULL && s->tree != s->root)
		fnc_object_tree_close(s->tree);
	if (s->root)
		fnc_object_tree_close(s->root);
	if (s->repo)
		fnc_close_repository_tree(s->repo);

	return 0;
}

static void
fnc_object_tree_close(struct fnc_tree_object *tree)
{
	int	idx;

	for (idx = 0; idx < tree->nentries; ++idx) {
		fsl_free(tree->entries[idx].basename);
		fsl_free(tree->entries[idx].path);
		fsl_free(tree->entries[idx].uuid);
	}

	fsl_free(tree->entries);
	fsl_free(tree);
}

static void
fnc_close_repository_tree(struct fnc_repository_tree *repo)
{
	struct fnc_repo_tree_node *next, *tn;

	tn = repo->head;
	while (tn) {
		next = tn->next;
		fsl_free(tn);
		tn = next;
	}
	fsl_free(repo);
}

static int
cmd_config(const fcli_command *argv)
{
	const char	*opt = NULL, *value = NULL;
	char		*prev, *v;
	enum fnc_opt_id	 setid;
	int		 rc = FSL_RC_OK;
#ifdef __OpenBSD__
	const fsl_cx	*const f = fcli_cx();
	fsl_buffer	 buf = fsl_buffer_empty;

	fsl_file_dirpart(fsl_cx_db_file_repo(f, NULL), -1, &buf, false);
	rc = init_unveil(fsl_buffer_cstr(&buf), fsl_cx_ckout_dir_name(f, NULL),
	    true);
	if (rc)
		return rc;
#endif

	rc = fcli_process_flags(argv->flags);
	if (rc || (rc = fcli_has_unused_flags(false)))
		return rc;

	opt = fcli_next_arg(true);
	if (opt == NULL || fnc_init.lsconf) {
		if (fnc_init.unset) {
			fnc_init.err = RC(FSL_RC_MISSING_INFO,
			    "%s", "-u|--unset requires <setting>");
			usage();
			/* NOT REACHED */
		}
		return fnc_conf_lsopt(fnc_init.lsconf ? false : true);
	}

	setid = fnc_conf_str2enum(opt);
	if (!setid)
		return RC(FSL_RC_NOT_FOUND, "invalid setting: %s", opt);

	value = fcli_next_arg(true);
	if (value || fnc_init.unset) {
		if (value && fnc_init.unset)
			return RC(FSL_RC_MISUSE, "\n--unset or set %s to %s?",
			    opt, value);
		prev = fnc_conf_getopt(setid, true);
		rc = fnc_conf_setopt(setid, value, fnc_init.unset);
		if (!rc)
			f_out("%s: %s -> %s (local)", fnc_conf_enum2str(setid),
			    prev ? prev : "default", value ? value : "default");
		fsl_free(prev);
	} else {
		v = fnc_conf_getopt(setid, true);
		f_out("%s = %s", fnc_conf_enum2str(setid), v ? v : "default");
		fsl_free(v);
	}

	return rc;
}

static int
fnc_conf_lsopt(bool all)
{
	char	*value = NULL;
	int	 idx, last = 0;
	size_t	 maxlen = 0;

	for (idx = FNC_START_SETTINGS + 1; idx < FNC_EOF_SETTINGS; ++idx) {
		last = (value = fnc_conf_getopt(idx, true)) ? idx : last;
		maxlen = MAX(fsl_strlen(fnc_opt_name[idx]), maxlen);
		fsl_free(value);
	}

	if (!last && !all) {
		f_out("No user-defined settings: "
		    "'%s config' for list of available settings.",
		    fcli_progname());
		return 0;
	}

	for (idx = FNC_START_SETTINGS + 1;  idx < FNC_EOF_SETTINGS;  ++idx) {
		value = fnc_conf_getopt(idx, true);
		if (value || all)
			f_out("%-*s%s%s%c", maxlen + 2, fnc_opt_name[idx],
			    value ? " = " : "", value ? value : "",
			    all ? (idx + 1 < FNC_EOF_SETTINGS ? '\n' : '\0') :
			    idx < last ? '\n' : '\0');
		fsl_free(value);
		value = NULL;
	}

	return 0;
}

static enum fnc_opt_id
fnc_conf_str2enum(const char *str)
{
	enum fnc_opt_id	idx;

	for (idx = FNC_START_SETTINGS + 1;  idx < FNC_EOF_SETTINGS;  ++idx)
		if (!fsl_stricmp(str, fnc_opt_name[idx]))
			return idx;

	return FNC_START_SETTINGS;
}

static const char *
fnc_conf_enum2str(enum fnc_opt_id id)
{
	if (id <= FNC_START_SETTINGS || id >= FNC_EOF_SETTINGS)
		return NULL;

	return fnc_opt_name[id];
}

static int
view_close_child(struct fnc_view *view)
{
	int	rc = 0;

	if (view->child == NULL)
		return rc;

	rc = view_close(view->child);
	view->child = NULL;

	return rc;
}

static void
view_set_child(struct fnc_view *view, struct fnc_view *child)
{
	view->child = child;
	child->parent = view;
}

static int
set_colours(struct fnc_colours *s, enum fnc_view_id vid)
{
	int	rc = 0;

	switch (vid) {
	case FNC_VIEW_DIFF: {
		static const char *regexp_diff[] = {
		    "^((checkin|wiki|ticket|technote) "
		    "[0-9a-f]|hash [+-] |\\[[+~>-]] |[+-]{3} )",
		    "^user:", "^date:", "^tags:", "^-", "^\\+", "^@@"
		};
		const int pairs_diff[][2] = {
		    {FNC_COLOUR_DIFF_META, init_colour(FNC_COLOUR_DIFF_META)},
		    {FNC_COLOUR_USER, init_colour(FNC_COLOUR_USER)},
		    {FNC_COLOUR_DATE, init_colour(FNC_COLOUR_DATE)},
		    {FNC_COLOUR_DIFF_TAGS, init_colour(FNC_COLOUR_DIFF_TAGS)},
		    {FNC_COLOUR_DIFF_MINUS, init_colour(FNC_COLOUR_DIFF_MINUS)},
		    {FNC_COLOUR_DIFF_PLUS, init_colour(FNC_COLOUR_DIFF_PLUS)},
		    {FNC_COLOUR_DIFF_CHUNK, init_colour(FNC_COLOUR_DIFF_CHUNK)}
		};
		rc = set_colour_scheme(s, pairs_diff, regexp_diff,
		    nitems(regexp_diff));
		break;
	}
	case FNC_VIEW_TREE: {
		static const char *regexp_tree[] = {"@ ->", "/$", "\\*$", "^$"};
		const int pairs_tree[][2] = {
		    {FNC_COLOUR_TREE_LINK, init_colour(FNC_COLOUR_TREE_LINK)},
		    {FNC_COLOUR_TREE_DIR, init_colour(FNC_COLOUR_TREE_DIR)},
		    {FNC_COLOUR_TREE_EXEC, init_colour(FNC_COLOUR_TREE_EXEC)},
		    {FNC_COLOUR_COMMIT, init_colour(FNC_COLOUR_COMMIT)}
		};
		rc = set_colour_scheme(s, pairs_tree, regexp_tree,
		    nitems(regexp_tree));
		break;
	}
	case FNC_VIEW_TIMELINE: {
		static const char *regexp_timeline[] = {"^$", "^$", "^$"};
		const int pairs_timeline[][2] = {
		    {FNC_COLOUR_COMMIT, init_colour(FNC_COLOUR_COMMIT)},
		    {FNC_COLOUR_USER, init_colour(FNC_COLOUR_USER)},
		    {FNC_COLOUR_DATE, init_colour(FNC_COLOUR_DATE)}
		};
		rc = set_colour_scheme(s, pairs_timeline, regexp_timeline,
		    nitems(regexp_timeline));
		break;
	}
	case FNC_VIEW_BLAME: {
		static const char *regexp_blame[] = {"^"};
		const int pairs_blame[][2] = {
		    {FNC_COLOUR_COMMIT, init_colour(FNC_COLOUR_COMMIT)}
		};
		rc = set_colour_scheme(s, pairs_blame, regexp_blame,
		    nitems(regexp_blame));
		break;
	}
	case FNC_VIEW_BRANCH: {
		static const char *regexp_branch[] = {
		    "^\\ +", "^ -", "@$", "\\*$"
		};
		const int pairs_branch[][2] = {
		    {FNC_COLOUR_BRANCH_OPEN,
		        init_colour(FNC_COLOUR_BRANCH_OPEN)},
		    {FNC_COLOUR_BRANCH_CLOSED,
		        init_colour(FNC_COLOUR_BRANCH_CLOSED)},
		    {FNC_COLOUR_BRANCH_CURRENT,
		        init_colour(FNC_COLOUR_BRANCH_CURRENT)},
		    {FNC_COLOUR_BRANCH_PRIVATE,
		         init_colour(FNC_COLOUR_BRANCH_PRIVATE)}
		};
		rc = set_colour_scheme(s, pairs_branch, regexp_branch,
		    nitems(regexp_branch));
		break;
	}
	default:
		rc = RC(FSL_RC_TYPE, "invalid fnc_view_id: %s", vid);
	}

	return rc;
}

static int
set_colour_scheme(struct fnc_colours *colours, const int (*pairs)[2],
    const char **regexp, int n)
{
	struct fnc_colour	*colour;
	int			 idx, rc = 0;

	for (idx = 0; idx < n; ++idx) {
		colour = fsl_malloc(sizeof(*colour));
		if (colour == NULL)
			return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR),
			    "%s", "fsl_malloc");

		rc = regcomp(&colour->regex, regexp[idx],
		    REG_EXTENDED | REG_NEWLINE | REG_NOSUB);
		if (rc) {
			static char regerr[512];
			regerror(rc, &colour->regex, regerr, sizeof(regerr));
			fsl_free(colour);
			return RC(FSL_RC_ERROR, "regcomp(%s) -> %s",
			    regexp[idx], regerr);
		}

		colour->scheme = pairs[idx][0];
		init_pair(colour->scheme, pairs[idx][1], -1);
		STAILQ_INSERT_HEAD(colours, colour, entries);
	}

	return rc;
}

static int
init_colour(enum fnc_opt_id id)
{
	char	*val = NULL;
	int	 rc = 0;

	val = fnc_conf_getopt(id, false);

	if (val == NULL)
		return default_colour(id);

	if (!fsl_stricmp(val, "black"))
		rc = COLOR_BLACK;
	else if (!fsl_stricmp(val, "red"))
		rc = COLOR_RED;
	else if (!fsl_stricmp(val, "green"))
		rc = COLOR_GREEN;
	else if (!fsl_stricmp(val, "yellow"))
		rc = COLOR_YELLOW;
	else if (!fsl_stricmp(val, "blue"))
		rc = COLOR_BLUE;
	else if (!fsl_stricmp(val, "magenta"))
		rc = COLOR_MAGENTA;
	else if (!fsl_stricmp(val, "cyan"))
		rc = COLOR_CYAN;
	else if (!fsl_stricmp(val, "white"))
		rc = COLOR_WHITE;
	else if (!fsl_stricmp(val, "default"))
		rc = -1;  /* Terminal default foreground colour. */

	fsl_free(val);
	return rc ? rc : default_colour(id);
}

/*
 * Lookup setting id from the repository db. If not found, search envvars. If
 * found, return a dynamically allocated string obtained from fsl_db_g_text() or
 * strdup(), which must be disposed of by the caller. Alternatively, if ls is
 * set, search local settings and envvars for id. If found, dynamically allocate
 * and return a formatted string for pretty printing the current state of id,
 * which the caller must free. In either case, if not found, return NULL.
 */
static char *
fnc_conf_getopt(enum fnc_opt_id id, bool ls)
{
	fsl_cx	*const f = fcli_cx();
	fsl_db	*db = NULL;
	char	*optval = NULL, *envvar = NULL;

	db = fsl_needs_repo(f);

	if (!db) {
		/* Theoretically, this shouldn't happen. */
		RC(FSL_RC_DB, "%s", "fsl_needs_repo");
		return NULL;
	}

	optval = fsl_db_g_text(db, NULL,
	    "SELECT value FROM config WHERE name=%Q", fnc_conf_enum2str(id));

	if (optval == NULL || ls)
		envvar = fsl_strdup(getenv(fnc_conf_enum2str(id)));

	if (ls && (optval || envvar)) {
		char *showopt = fsl_mprintf("%s%s%s%s%s",
		    optval ? optval : "", optval ? " (local)" : "",
		    optval && envvar ? ", " : "",
		    envvar ? envvar : "", envvar ? " (envvar)" : "");
		fsl_free(optval);
		fsl_free(envvar);
		optval = showopt;
	}

	return ls ? optval : (optval ? optval : envvar);
}

static int
default_colour(enum fnc_opt_id id)
{
	if (id == FNC_COLOUR_COMMIT)
		return COLOR_GREEN;
	if (id == FNC_COLOUR_USER)
		return COLOR_CYAN;
	if (id == FNC_COLOUR_DATE)
		return COLOR_YELLOW;
	if (id == FNC_COLOUR_DIFF_META)
		return COLOR_GREEN;
	if (id == FNC_COLOUR_DIFF_MINUS)
		return COLOR_MAGENTA;
	if (id == FNC_COLOUR_DIFF_PLUS)
		return COLOR_CYAN;
	if (id == FNC_COLOUR_DIFF_CHUNK)
		return COLOR_YELLOW;
	if (id == FNC_COLOUR_DIFF_TAGS)
		return COLOR_MAGENTA;
	if (id == FNC_COLOUR_TREE_LINK)
		return COLOR_MAGENTA;
	if (id == FNC_COLOUR_TREE_DIR)
		return COLOR_CYAN;
	if (id == FNC_COLOUR_TREE_EXEC)
		return COLOR_GREEN;
	if (id == FNC_COLOUR_BRANCH_OPEN)
		return COLOR_CYAN;
	if (id == FNC_COLOUR_BRANCH_CLOSED)
		return COLOR_MAGENTA;
	if (id == FNC_COLOUR_BRANCH_CURRENT)
		return COLOR_GREEN;
	if (id == FNC_COLOUR_BRANCH_PRIVATE)
		return COLOR_YELLOW;

	return -1;  /* Terminal default foreground colour. */
}

static int
fnc_conf_setopt(enum fnc_opt_id id, const char *val, bool unset)
{
	fsl_cx	*const f = fcli_cx();
	fsl_db	*db = NULL;

	db = fsl_needs_repo(f);

	if (!db)  /* Theoretically, this shouldn't happen. */
		return RC(FSL_RC_DB, "%s", "fsl_needs_repo");

	if (unset)
		return fsl_db_exec(db, "DELETE FROM config WHERE name=%Q",
		    fnc_conf_enum2str(id));

	return fsl_db_exec(db,
	    "INSERT OR REPLACE INTO config(name, value, mtime) "
	    "VALUES(%Q, %Q, now())", fnc_conf_enum2str(id), val);
}

struct fnc_colour *
get_colour(struct fnc_colours *colours, int scheme)
{
	struct fnc_colour *c = NULL;

	STAILQ_FOREACH(c, colours, entries) {
		if (c->scheme == scheme)
			return c;
	}

	return NULL;
}

struct fnc_colour *
match_colour(struct fnc_colours *colours, const char *line)
{
	struct fnc_colour *c = NULL;

	STAILQ_FOREACH(c, colours, entries) {
		if (match_line(line, &c->regex, 0, NULL))
			return c;
	}

	return NULL;
}

static int
match_line(const char *line, regex_t *regex, size_t nmatch,
    regmatch_t *regmatch)
{
	return regexec(regex, line, nmatch, regmatch, 0) == 0;
}

static void
free_colours(struct fnc_colours *colours)
{
	struct fnc_colour *c;

	while (!STAILQ_EMPTY(colours)) {
		c = STAILQ_FIRST(colours);
		STAILQ_REMOVE_HEAD(colours, entries);
		regfree(&c->regex);
		fsl_free(c);
	}
}

/*
 * Emulate vim(1) gg: User has 1 sec to follow first 'g' keypress with another.
 */
static bool
fnc_home(struct fnc_view *view)
{
	bool	home = true;

	halfdelay(10);	/* Block for 1 second, then return ERR. */
	if (wgetch(view->window) != 'g')
		home = false;
	cbreak();	/* Return to blocking mode on user input. */

	return home;
}

static int
cmd_blame(fcli_command const *argv)
{
	fsl_cx		*const f = fcli_cx();
	struct fnc_view	*view;
	char		*path = NULL;
	fsl_uuid_str	 commit_id = NULL;
	fsl_id_t	 tip = 0, rid = 0;
	long		 nlimit = 0;
	int		 rc = 0;

	rc = fcli_process_flags(argv->flags);
	if (rc || (rc = fcli_has_unused_flags(false)))
		goto end;
	if (!fcli_next_arg(false)) {
		rc = RC(FSL_RC_MISSING_INFO,
		    "%s blame requires versioned file path", fcli_progname());
		goto end;
	}

	if (fnc_init.nrecords.zlimit) {
		char *n = (char *)fnc_init.nrecords.zlimit;
		bool timed;
		if (n[fsl_strlen(n) - 1] == 's') {
			n[fsl_strlen(n) - 1] = '\0';
			timed = true;
		}
		if ((rc = strtonumcheck(&nlimit, n, INT_MIN, INT_MAX)))
			goto end;
		if (timed)
			nlimit *= -1;
	}

	if (fnc_init.sym || fnc_init.reverse) {
		if (fnc_init.reverse) {
			if (!fnc_init.sym) {
				rc = RC(FSL_RC_MISSING_INFO,
				    "%s blame --reverse requires --commit",
				    fcli_progname());
				goto end;
			}
			rc = fsl_sym_to_rid(f, "tip", FSL_SATYPE_CHECKIN, &tip);
			if (rc)
				goto end;
		}
		rc = fsl_sym_to_rid(f, fnc_init.sym, FSL_SATYPE_CHECKIN, &rid);
		if (rc)
			goto end;
	} else if (!fnc_init.sym) {
		fsl_ckout_version_info(f, &rid, NULL);
		if (!rid)  /* -R|--repo option used */
			fsl_sym_to_rid(f, "tip", FSL_SATYPE_CHECKIN, &rid);
	}

	rc = map_repo_path(&path);
	if (rc) {
		if (rc != FSL_RC_NOT_FOUND || !fnc_init.sym)
			goto end;
		/* Path may be valid in repository tree of specified commit. */
		rc = 0;
		fcli_err_reset();
	}

	commit_id = fsl_rid_to_uuid(f, rid);
	if (rc || (path[0] == '/' && path[1] == '\0')) {
		rc = rc ? rc : RC(FSL_RC_MISSING_INFO,
		    "%s blame requires versioned file path", fcli_progname());
		goto end;
	}

	rc = init_curses();
	if (rc)
		goto end;
#ifdef __OpenBSD__
	rc = init_unveil(fsl_cx_db_file_repo(f, NULL),
	    fsl_cx_ckout_dir_name(f, NULL), false);
	if (rc)
		goto end;
#endif

	view = view_open(0, 0, 0, 0, FNC_VIEW_BLAME);
	if (view == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", view_open);
		goto end;
	}

	rc = open_blame_view(view, path, commit_id, tip, nlimit);
	if (rc)
		goto end;
	rc = view_loop(view);
end:
	fsl_free(path);
	fsl_free(commit_id);
	return rc;
}

static int
open_blame_view(struct fnc_view *view, char *path, fsl_uuid_str commit_id,
    fsl_id_t tip, int nlimit)
{
	struct fnc_blame_view_state	*s = &view->state.blame;
	int				 rc = 0;

	CONCAT(STAILQ, _INIT)(&s->blamed_commits);

	s->path = fsl_strdup(path);
	if (s->path == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_strdup");

	rc = fnc_commit_qid_alloc(&s->blamed_commit, commit_id);
	if (rc) {
		fsl_free(s->path);
		return rc;
	}

	CONCAT(STAILQ, _INSERT_HEAD)(&s->blamed_commits, s->blamed_commit,
	    entry);
	memset(&s->blame, 0, sizeof(s->blame));
	s->first_line_onscreen = 1;
	s->last_line_onscreen = view->nlines;
	s->selected_line = 1;
	s->blame_complete = false;
	s->commit_id = commit_id;
	s->blame.origin = tip;
	s->blame.nlimit = nlimit;
	s->spin_idx = 0;
	s->colour = !fnc_init.nocolour && has_colors();

	if (s->colour) {
		STAILQ_INIT(&s->colours);
		rc = set_colours(&s->colours, FNC_VIEW_BLAME);
		if (rc)
			return rc;
	}

	view->show = show_blame_view;
	view->input = blame_input_handler;
	view->close = close_blame_view;
	view->search_init = blame_search_init;
	view->search_next = blame_search_next;

	return run_blame(view);
}

static int
run_blame(struct fnc_view *view)
{
	fsl_cx				*const f = fcli_cx();
	struct fnc_blame_view_state	*s = &view->state.blame;
	struct fnc_blame		*blame = &s->blame;
	fsl_deck			 d = fsl_deck_empty;
	fsl_buffer			 buf = fsl_buffer_empty;
	fsl_annotate_opt		*opt = NULL;
	const fsl_card_F		*cf;
	char				*filepath = NULL;
	char				*master = NULL, *root = NULL;
	int				 rc = 0;

	/*
	 * Trim prefixed '/' if path has been processed by map_repo_path(),
	 * which only occurs when the -c option has not been passed.
	 * XXX This slash trimming is cumbersome; we should not prefix a slash
	 * in map_repo_path() as we only want the slash for displaying an
	 * absolute-repository-relative path, so we should prefix it only then.
	 */
	filepath = s->path[0] != '/' ? s->path : s->path + 1;

	rc = fsl_deck_load_sym(f, &d, s->blamed_commit->id, FSL_SATYPE_CHECKIN);
	if (rc)
		goto end;

	cf = fsl_deck_F_search(&d, filepath);
	if (cf == NULL) {
		rc = RC(FSL_RC_NOT_FOUND, "'%s' not found in tree [%s]",
		    filepath, s->blamed_commit->id);
		goto end;
	}
	rc = fsl_card_F_content(f, cf, &buf);
	if (rc)
		goto end;

	/*
	 * We load f with the actual file content to map line offsets so we
	 * accurately find tokens when running a search.
	 */
	blame->f = tmpfile();
	if (blame->f == NULL) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_IO), "%s", "tmpfile");
		goto end;
	}

	opt = &blame->thread_cx.blame_opt;
	opt->filename = fsl_strdup(filepath);
	fcli_fax((char *)opt->filename);
	rc = fsl_sym_to_rid(f, s->blamed_commit->id, FSL_SATYPE_CHECKIN,
	    &opt->versionRid);
	if (rc)
		goto end;
	opt->originRid = blame->origin;    /* tip when -r is passed */
	if (blame->nlimit < 0)
		opt->limitMs = abs(blame->nlimit) * 1000;
	else
		opt->limitVersions = blame->nlimit;
	opt->out = blame_cb;
	opt->outState = &blame->cb_cx;

	rc = fnc_dump_buffer_to_file(&blame->filesz, &blame->nlines,
	    &blame->line_offsets, blame->f, &buf);
	if (rc)
		goto end;
	if (blame->nlines == 0) {
		s->blame_complete = true;
		goto end;
	}

	/* Don't include EOF \n in blame line count. */
	if (blame->line_offsets[blame->nlines - 1] == blame->filesz)
		--blame->nlines;

	blame->lines = calloc(blame->nlines, sizeof(*blame->lines));
	if (blame->lines == NULL) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_ERROR), "%s", "calloc");
		goto end;
	}

	master = fsl_config_get_text(f, FSL_CONFDB_REPO, "main-branch", NULL);
	if (master == NULL) {
		master = fsl_strdup("trunk");
		if (master == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
			goto end;
		}
	}
	root = fsl_mprintf("root:%s", master);
	rc = fsl_sym_to_uuid(f, root, FSL_SATYPE_CHECKIN,
	    &blame->cb_cx.root_commit, NULL);
	if (rc) {
		rc = RC(rc, "%s", "fsl_sym_to_uuid");
		goto end;
	}

	blame->cb_cx.view = view;
	blame->cb_cx.lines = blame->lines;
	blame->cb_cx.nlines = blame->nlines;
	blame->cb_cx.commit_id = fsl_strdup(s->blamed_commit->id);
	if (blame->cb_cx.commit_id == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		goto end;
	}
	blame->cb_cx.quit = &s->done;

	blame->thread_cx.path = s->path;
	blame->thread_cx.cb_cx = &blame->cb_cx;
	blame->thread_cx.complete = &s->blame_complete;
	blame->thread_cx.cancel_cb = cancel_blame;
	blame->thread_cx.cancel_cx = &s->done;
	s->blame_complete = false;

	if (s->first_line_onscreen + view->nlines - 1 > blame->nlines) {
		s->first_line_onscreen = 1;
		s->last_line_onscreen = view->nlines;
		s->selected_line = 1;
	}

end:
	fsl_free(master);
	fsl_free(root);
	fsl_deck_finalize(&d);
	fsl_buffer_clear(&buf);
	if (rc)
		stop_blame(blame);
	return rc;
}

/*
 * Write file content in buf to out file. Record the number of lines in the file
 * in nlines, and total bytes written in filesz. Assign byte offsets of each
 * line to the dynamically allocated *line_offsets, which must eventually be
 * disposed of by the caller. Flush and rewind out file when done.
 */
static int
fnc_dump_buffer_to_file(off_t *filesz, int *nlines, off_t **line_offsets,
    FILE *out, fsl_buffer *buf)
{
	off_t		 off = 0, total_len = 0;
	size_t		 len, n, i = 0, nalloc = 0;
	int		 rc = 0;
	const int	 alloc_chunksz = MIN(512, BUFSIZ);

	if (line_offsets)
		*line_offsets = NULL;
	if (filesz)
		*filesz = 0;
	if (nlines)
		*nlines = 0;

	len = buf->used;
	if (len == 0)
		return rc;  /* empty file */

	if (nlines) {
		if (line_offsets && *line_offsets == NULL) {
			*nlines = 1;
			nalloc = alloc_chunksz;
			*line_offsets = calloc(nalloc, sizeof(**line_offsets));
			if (*line_offsets == NULL)
				return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR),
				    "%s", "calloc");

			/* Consume the first line. */
			while (i < len) {
				if (buf->mem[i] == '\n')
					break;
				++i;
			}
		}
		/* Scan '\n' offsets. */
		while (i < len) {
			if (buf->mem[i] != '\n') {
				++i;
				continue;
			}
			++(*nlines);
			if (line_offsets && nalloc < (size_t)*nlines) {
				size_t n, oldsz, newsz;
				off_t *new = NULL;

				n = *nlines + alloc_chunksz;
				oldsz = nalloc * sizeof(**line_offsets);
				newsz = n * sizeof(**line_offsets);
				if (newsz <= oldsz) {
					size_t b = oldsz - newsz;
					if (b < oldsz / 2 &&
					    b < (size_t)getpagesize()) {
						memset((char *)*line_offsets
						    + newsz, 0, b);
						goto allocated;
					}
				}
				new = fsl_realloc(*line_offsets, newsz);
				if (new == NULL) {
					fsl_free(*line_offsets);
					*line_offsets = NULL;
					return RC(FSL_RC_ERROR, "%s",
					    "fsl_realloc");
				}
				*line_offsets = new;
allocated:
				nalloc = n;
			}
			if (line_offsets) {
				off = total_len + i + 1;
				(*line_offsets)[*nlines - 1] = off;
			}
			++i;
		}
	}
	n = fwrite(buf->mem, 1, len, out);
	if (n != len)
		return RC(ferror(out) ? fsl_errno_to_rc(errno, FSL_RC_IO)
		    : FSL_RC_IO, "%s", "fwrite");
	total_len += len;

	if (fflush(out) != 0)
		return RC(fsl_errno_to_rc(errno, FSL_RC_IO), "%s", "fflush");
	rewind(out);

	if (filesz)
		*filesz = total_len;

	return rc;
}

static int
show_blame_view(struct fnc_view *view)
{
	struct fnc_blame_view_state	*s = &view->state.blame;
	int				 rc = 0;

	if (!s->blame.thread_id && !s->blame_complete) {
		rc = pthread_create(&s->blame.thread_id, NULL, blame_thread,
		    &s->blame.thread_cx);
		if (rc)
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_create");

		halfdelay(1);	/* Fast refresh while annotating.  */
	}

	if (s->blame_complete)
		cbreak();	/* Return to blocking mode. */

	rc = draw_blame(view);
	drawborder(view);

	return rc;
}

static void *
blame_thread(void *state)
{
	fsl_cx				*const f = fcli_cx();
	struct fnc_blame_thread_cx	*cx = state;
	int				 rc0, rc;

	rc = block_main_thread_signals();
	if (rc)
		return (void *)(intptr_t)rc;

	rc = fsl_annotate(f, &cx->blame_opt);
	if (rc && fsl_cx_err_get_e(f)->code == FSL_RC_BREAK) {
		fcli_err_reset();
		rc = 0;
	}

	rc0 = pthread_mutex_lock(&fnc_mutex);
	if (rc0)
		return (void *)(intptr_t)RC(fsl_errno_to_rc(rc0, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_lock");

	*cx->complete = true;

	rc0 = pthread_mutex_unlock(&fnc_mutex);
	if (rc0 && !rc)
		rc = RC(fsl_errno_to_rc(rc0, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_unlock");

	return (void *)(intptr_t)rc;
}

static int
blame_cb(void *state, fsl_annotate_opt const * const opt,
    fsl_annotate_step const * const step)
{
	struct fnc_blame_cb_cx	*cx = state;
	struct fnc_blame_line	*line;
	int			 rc = 0;

	rc = pthread_mutex_lock(&fnc_mutex);
	if (rc)
		return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_lock");

	if (*cx->quit) {
		rc = fcli_err_set(FSL_RC_BREAK, "user quit");
		goto end;
	}

	line = &cx->lines[step->lineNumber - 1];
	if (line->annotated)
		goto end;

	if (step->mtime) {
		line->id = fsl_strdup(step->versionHash);
		if (line->id == NULL) {
			rc = RC(FSL_RC_ERROR, "%s", fsl_strdup);
			goto end;
		}
		line->annotated = true;
	} else
		line->id = NULL;

	/* -r can return lines with no version, so use root check-in. */
	if (opt->originRid && !line->id) {
		line->id = fsl_strdup(cx->root_commit);
		line->annotated = true;
	}

	++cx->nlines;
end:
	rc = pthread_mutex_unlock(&fnc_mutex);
	if (rc)
		rc = RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_unlock");
	return rc;
}

static int
draw_blame(struct fnc_view *view)
{
	struct fnc_blame_view_state	*s = &view->state.blame;
	struct fnc_blame		*blame = &s->blame;
	struct fnc_blame_line		*blame_line;
	regmatch_t			*regmatch = &view->regmatch;
	struct fnc_colour		*c = NULL;
	wchar_t				*wcstr;
	char				*line = NULL;
	fsl_uuid_str			 prev_id = NULL;
	ssize_t				 linelen;
	size_t				 linesz = 0;
	int				 width, lineno = 0, nprinted = 0;
	int				 rc = 0;
	const int			 idfield = 11;  /* Prefix + space. */

	rewind(blame->f);
	werase(view->window);

	if ((line = fsl_mprintf("checkin %s", s->blamed_commit->id)) == NULL) {
		rc = RC(fsl_errno_to_rc(errno, FSL_RC_ERROR),
		    "%s", "fsl_mprintf");
		return rc;
	}

	rc = formatln(&wcstr, &width, line, view->ncols, 0);
	fsl_free(line);
	line = NULL;
	if (rc)
		return rc;
	if (screen_is_shared(view))
		wattron(view->window, A_REVERSE);
	if (s->colour)
		c = get_colour(&s->colours, FNC_COLOUR_COMMIT);
	if (c)
		wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
	waddwstr(view->window, wcstr);
	while (width < view->ncols) {
		waddch(view->window, ' ');
		++width;
	}
	if (c)
		wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
	if (screen_is_shared(view))
		wattroff(view->window, A_REVERSE);
	fsl_free(wcstr);
	wcstr = NULL;
	if (width < view->ncols - 1)
		waddch(view->window, '\n');

	line = fsl_mprintf("[%d/%d] %s%s%s %c",
	    MIN(blame->nlines, s->first_line_onscreen - 1 + s->selected_line),
	    blame->nlines, s->blame_complete ? "" : "annotating... ",
	    fnc_init.sym ? "/" : "", s->path,
	    s->blame_complete ? ' ' : SPINNER[s->spin_idx]);
	if (SPINNER[++s->spin_idx] == '\0')
		s->spin_idx = 0;
	rc = formatln(&wcstr, &width, line, view->ncols, 0);
	fsl_free(line);
	line = NULL;
	if (rc)
		return rc;
	waddwstr(view->window, wcstr);
	fsl_free(wcstr);
	wcstr = NULL;
	if (width < view->ncols - 1)
		waddch(view->window, '\n');

	s->eof = false;
	while (nprinted < view->nlines - 2) {
		linelen = getline(&line, &linesz, blame->f);
		if (linelen == -1) {
			if (feof(blame->f)) {
				s->eof = true;
				break;
			}
			fsl_free(line);
			return RC(ferror(blame->f) ? fsl_errno_to_rc(errno,
			    FSL_RC_IO) : FSL_RC_IO, "%s", "getline");
		}
		if (++lineno < s->first_line_onscreen)
			continue;

		if (view->active && nprinted == s->selected_line - 1)
			wattr_on(view->window, A_REVERSE, NULL);

		if (blame->nlines > 0) {
			blame_line = &blame->lines[lineno - 1];
			if (blame_line->annotated && prev_id &&
			    fsl_uuidcmp(prev_id, blame_line->id) == 0 &&
			    !(view->active &&
			    nprinted == s->selected_line - 1)) {
				waddstr(view->window, "          ");
			} else if (blame_line->annotated) {
				char *id_str;
				id_str = fsl_strndup(blame_line->id,
				    idfield - 1);
				if (id_str == NULL) {
					fsl_free(line);
					return RC(FSL_RC_ERROR, "%s",
					    "fsl_strdup");
				}
				if (s->colour)
					c = get_colour(&s->colours,
					    FNC_COLOUR_COMMIT);
				if (c)
					wattr_on(view->window,
					    COLOR_PAIR(c->scheme), NULL);
				wprintw(view->window, "%.*s", idfield - 1,
				    id_str);
				if (c)
					wattr_off(view->window,
					    COLOR_PAIR(c->scheme), NULL);
				fsl_free(id_str);
				prev_id = blame_line->id;
			} else {
				waddstr(view->window, "..........");
				prev_id = NULL;
			}
		} else {
			waddstr(view->window, "..........");
			prev_id = NULL;
		}

		if (view->active && nprinted == s->selected_line - 1)
			wattr_off(view->window, A_REVERSE, NULL);
		waddstr(view->window, " ");

		if (view->ncols <= idfield) {
			width = idfield;
			wcstr = wcsdup(L"");
			if (wcstr == NULL) {
				rc = RC(fsl_errno_to_rc(errno, FSL_RC_RANGE),
				    "%s", "wcsdup");
				fsl_free(line);
				return rc;
			}
		} else if (s->first_line_onscreen + nprinted == s->matched_line
		    && regmatch->rm_so >= 0 &&
		    regmatch->rm_so < regmatch->rm_eo) {
			rc = write_matched_line(&width, line,
			    view->ncols - idfield, idfield,
			    view->window, regmatch);
			if (rc) {
				fsl_free(line);
				return rc;
			}
			width += idfield;
		} else {
			rc = formatln(&wcstr, &width, line,
			    view->ncols - idfield, idfield);
			waddwstr(view->window, wcstr);
			fsl_free(wcstr);
			wcstr = NULL;
			width += idfield;
		}

		if (width <= view->ncols - 1)
			waddch(view->window, '\n');
		if (++nprinted == 1)
			s->first_line_onscreen = lineno;
	}
	fsl_free(line);
	s->last_line_onscreen = lineno;

	drawborder(view);

	return rc;
}

static int
blame_input_handler(struct fnc_view **new_view, struct fnc_view *view, int ch)
{
	struct fnc_view			*branch_view, *diff_view;
	struct fnc_blame_view_state	*s = &view->state.blame;
	int				 start_col = 0, rc = 0;

	switch (ch) {
	case 'q':
		s->done = true;
		if (s->selected_commit)
			fnc_commit_artifact_close(s->selected_commit);
		break;
	case 'c':
		s->colour = !s->colour;
		break;
	case 'g':
		if (!fnc_home(view))
			break;
	case KEY_HOME:
		s->selected_line = 1;
		s->first_line_onscreen = 1;
		break;
	case KEY_END:
	case 'G':
		if (s->blame.nlines < view->nlines - 2) {
			s->selected_line = s->blame.nlines;
			s->first_line_onscreen = 1;
		} else {
			s->selected_line = view->nlines - 2;
			s->first_line_onscreen = s->blame.nlines -
			    (view->nlines - 3);
		}
		break;
	case KEY_UP:
	case 'k':
		if (s->selected_line > 1)
			--s->selected_line;
		else if (s->selected_line == 1 && s->first_line_onscreen > 1)
			--s->first_line_onscreen;
		break;
	case KEY_PPAGE:
	case CTRL('b'):
		if (s->first_line_onscreen == 1) {
			s->selected_line = 1;
			break;
		}
		if (s->first_line_onscreen > view->nlines - 2)
			s->first_line_onscreen -= (view->nlines - 2);
		else
			s->first_line_onscreen = 1;
		break;
	case KEY_DOWN:
	case 'j':
		if (s->selected_line < view->nlines - 2 &&
		    s->first_line_onscreen +
		    s->selected_line <= s->blame.nlines)
			++s->selected_line;
		else if (s->last_line_onscreen < s->blame.nlines)
			++s->first_line_onscreen;
		break;
	case 'b':
	case 'p': {
		fsl_uuid_cstr id = NULL;
		id = get_selected_commit_id(s->blame.lines, s->blame.nlines,
		    s->first_line_onscreen, s->selected_line);
		if (id == NULL)
			break;
		if (ch == 'p') {
			fsl_cx		*const f = fcli_cx();
			fsl_db		*db = fsl_needs_repo(f);
			fsl_deck	 d = fsl_deck_empty;
			fsl_id_t	 rid = fsl_uuid_to_rid(f, id);
			fsl_uuid_str	 pid = fsl_db_g_text(db, NULL,
			    "SELECT uuid FROM plink, blob WHERE plink.cid=%d "
			    "AND blob.rid=plink.pid AND plink.isprim", rid);
			if (pid == NULL)
				break;
			/* Check file exists in parent check-in. */
			rc = fsl_deck_load_sym(f, &d, pid, FSL_SATYPE_CHECKIN);
			if (rc) {
				fsl_deck_finalize(&d);
				fsl_free(pid);
				return RC(rc, "%s", "fsl_deck_load_sym");
			}
			rc = fsl_deck_F_rewind(&d);
			if (rc) {
				fsl_deck_finalize(&d);
				fsl_free(pid);
				return RC(rc, "%s", "fsl_deck_F_rewind");
			}
			if (fsl_deck_F_search(&d, s->path +
			    (fnc_init.sym ? 0 : 1)) == NULL) {
				char *m = fsl_mprintf("-- %s not in [%.12s] --",
				    s->path + (fnc_init.sym ? 0 : 1), pid);
				if (m == NULL)
					rc = RC(FSL_RC_ERROR, "%s",
					    "fsl_mprintf");
				wattr_on(view->window, A_BOLD, NULL);
				mvwaddstr(view->window,
				    view->start_ln + view->nlines - 1, 0, m);
				wclrtoeol(view->window);
				wattr_off(view->window, A_BOLD, NULL);
				update_panels();
				doupdate();
				sleep(1);
				fsl_deck_finalize(&d);
				fsl_free(pid);
				fsl_free(m);
				break;
			}
			rc = fnc_commit_qid_alloc(&s->blamed_commit, pid);
			if (rc)
				return rc;
		} else {
			if (!fsl_uuidcmp(id, s->blamed_commit->id))
				break;
			rc = fnc_commit_qid_alloc(&s->blamed_commit, id);
		}
		if (rc)
			break;
		s->done = true;
		rc = stop_blame(&s->blame);
		s->done = false;
		if (rc)
			break;
		CONCAT(STAILQ, _INSERT_HEAD)(&s->blamed_commits,
		    s->blamed_commit, entry);
		rc = run_blame(view);
		if (rc)
			break;
		break;
	}
	case KEY_BACKSPACE:
	case 'B': {
		struct fnc_commit_qid *first;
		first = CONCAT(STAILQ, _FIRST)(&s->blamed_commits);
		if (!fsl_uuidcmp(first->id, s->commit_id))
			break;
		s->done = true;
		rc = stop_blame(&s->blame);
		s->done = false;
		if (rc)
			break;
		CONCAT(STAILQ, _REMOVE_HEAD)(&s->blamed_commits, entry);
		fnc_commit_qid_free(s->blamed_commit);
		s->blamed_commit = CONCAT(STAILQ, _FIRST)(&s->blamed_commits);
		rc = run_blame(view);
		if (rc)
			break;
		break;
	}
	case 'T':
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		branch_view = view_open(view->nlines, view->ncols,
		    view->start_ln, start_col, FNC_VIEW_BRANCH);
		if (branch_view == NULL)
			return RC(FSL_RC_ERROR, "%s", "view_open");
		rc = open_branch_view(branch_view, BRANCH_LS_OPEN_CLOSED, NULL,
		    0, 0);
		if (rc) {
			view_close(branch_view);
			return rc;
		}
		view->active = false;
		branch_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, branch_view);
			view->focus_child = true;
		} else
			*new_view = branch_view;
		break;
	case KEY_ENTER:
	case '\r': {
		fsl_cx				*const f = fcli_cx();
		struct fnc_commit_artifact	*commit = NULL;
		fsl_stmt			*q = NULL;
		fsl_uuid_cstr			 id = NULL;

		id = get_selected_commit_id(s->blame.lines, s->blame.nlines,
		    s->first_line_onscreen, s->selected_line);
		if (id == NULL)
			break;
		if (s->selected_commit)
			fnc_commit_artifact_close(s->selected_commit);
		if (rc)
			break;
		q = fsl_stmt_malloc();
		rc = commit_builder(&commit, fsl_uuid_to_rid(f, id), q);
		fsl_stmt_finalize(q);
		if (rc) {
			fnc_commit_artifact_close(commit);
			break;
		}
		if (view_is_parent(view))
		    start_col = view_split_start_col(view->start_col);
		diff_view = view_open(0, 0, 0, start_col, FNC_VIEW_DIFF);
		if (diff_view == NULL) {
			fnc_commit_artifact_close(commit);
			rc = RC(FSL_RC_ERROR, "%s", "view_open");
			break;
		}
		rc = open_diff_view(diff_view, commit, DEF_DIFF_CTX,
		    fnc_init.ws, fnc_init.invert, !fnc_init.quiet, NULL, true,
		    NULL);
		s->selected_commit = commit;
		if (rc) {
			fnc_commit_artifact_close(commit);
			view_close(diff_view);
			break;
		}
		view->active = false;
		diff_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				break;
			view_set_child(view, diff_view);
			view->focus_child = true;
		} else
			*new_view = diff_view;
		if (rc)
			break;
		break;
	}
	case KEY_NPAGE:
	case CTRL('f'):
	case ' ':
		if (s->last_line_onscreen >= s->blame.nlines && s->selected_line
		    >= MIN(s->blame.nlines, view->nlines - 2))
			break;
		if (s->last_line_onscreen >= s->blame.nlines &&
		    s->selected_line < view->nlines - 2) {
			s->selected_line = MIN(s->blame.nlines,
			    view->nlines - 2);
			break;
		}
		if (s->last_line_onscreen + view->nlines - 2 <= s->blame.nlines)
			s->first_line_onscreen += view->nlines - 2;
		else
			s->first_line_onscreen =
			    s->blame.nlines - (view->nlines - 3);
		break;
	case KEY_RESIZE:
		if (s->selected_line > view->nlines - 2) {
			s->selected_line = MIN(s->blame.nlines,
			    view->nlines - 2);
		}
		break;
	default:
		break;
	}
	return rc;
}

static int
blame_search_init(struct fnc_view *view)
{
	struct fnc_blame_view_state *s = &view->state.blame;

	s->matched_line = 0;
	return 0;
}

static int
blame_search_next(struct fnc_view *view)
{
	struct fnc_blame_view_state	*s = &view->state.blame;
	char				*line = NULL;
	ssize_t				 linelen;
	size_t				 linesz = 0;
	int				 lineno;

	if (view->searching == SEARCH_DONE) {
		view->search_status = SEARCH_CONTINUE;
		return 0;
	}

	if (s->matched_line) {
		if (view->searching == SEARCH_FORWARD)
			lineno = s->matched_line + 1;
		else
			lineno = s->matched_line - 1;
	} else {
		if (view->searching == SEARCH_FORWARD)
			lineno = 1;
		else
			lineno = s->blame.nlines;
	}

	while (1) {
		off_t offset;

		if (lineno <= 0 || lineno > s->blame.nlines) {
			if (s->matched_line == 0) {
				view->search_status = SEARCH_CONTINUE;
				break;
			}

			if (view->searching == SEARCH_FORWARD)
				lineno = 1;
			else
				lineno = s->blame.nlines;
		}

		offset = s->blame.line_offsets[lineno - 1];
		if (fseeko(s->blame.f, offset, SEEK_SET) != 0) {
			fsl_free(line);
			return RC(fsl_errno_to_rc(errno, FSL_RC_IO),
			    "%s", "fseeko");
		}
		linelen = getline(&line, &linesz, s->blame.f);
		if (linelen != -1 && regexec(&view->regex, line, 1,
		    &view->regmatch, 0) == 0) {
			view->search_status = SEARCH_CONTINUE;
			s->matched_line = lineno;
			break;
		}
		if (view->searching == SEARCH_FORWARD)
			++lineno;
		else
			--lineno;
	}
	fsl_free(line);

	if (s->matched_line) {
		s->first_line_onscreen = s->matched_line;
		s->selected_line = 1;
	}

	return 0;
}

static fsl_uuid_cstr
get_selected_commit_id(struct fnc_blame_line *lines, int nlines,
    int first_line_onscreen, int selected_line)
{
	struct fnc_blame_line *line;

	if (nlines <= 0)
		return NULL;

	line = &lines[first_line_onscreen - 1 + selected_line - 1];

	return line->id;
}

static int
fnc_commit_qid_alloc(struct fnc_commit_qid **qid, fsl_uuid_cstr id)
{
	int rc = 0;

	*qid = calloc(1, sizeof(**qid));
	if (*qid == NULL)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ERROR), "%s", "calloc");

	(*qid)->id = fsl_strdup(id);
	if ((*qid)->id == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		fnc_commit_qid_free(*qid);
		*qid = NULL;
	}

	return rc;
}

static int
close_blame_view(struct fnc_view *view)
{
	struct fnc_blame_view_state	*s = &view->state.blame;
	int				 rc = 0;

	rc = stop_blame(&s->blame);

	while (!CONCAT(STAILQ, _EMPTY)(&s->blamed_commits)) {
		struct fnc_commit_qid *blamed_commit;
		blamed_commit = CONCAT(STAILQ, _FIRST)(&s->blamed_commits);
		CONCAT(STAILQ, _REMOVE_HEAD)(&s->blamed_commits, entry);
		fnc_commit_qid_free(blamed_commit);
	}

	fsl_free(s->path);
	free_colours(&s->colours);

	return rc;
}

static int
stop_blame(struct fnc_blame *blame)
{
	int idx, rc = 0;

	if (blame->thread_id) {
		int retval;
		rc = pthread_mutex_unlock(&fnc_mutex);
		if (rc)
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_unlock");
		rc = pthread_join(blame->thread_id, (void **)&retval);
		if (rc)
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_join");
		rc = pthread_mutex_lock(&fnc_mutex);
		if (rc)
			return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
			    "%s", "pthread_mutex_lock");
		if (!rc && fsl_cx_err_get_e(fcli_cx())->code == FSL_RC_BREAK) {
			rc = 0;
			fcli_err_reset();
		}
		blame->thread_id = 0;
	}
	if (blame->f) {
		if (fclose(blame->f) == EOF && rc == 0)
			rc = RC(fsl_errno_to_rc(errno, FSL_RC_IO), "%s",
			    fclose);
		blame->f = NULL;
	}
	if (blame->lines) {
		for (idx = 0; idx < blame->nlines; ++idx)
			fsl_free(blame->lines[idx].id);
		fsl_free(blame->lines);
		blame->lines = NULL;
	}

	fsl_free(blame->cb_cx.root_commit);
	blame->cb_cx.root_commit = NULL;
	fsl_free(blame->cb_cx.commit_id);
	blame->cb_cx.commit_id = NULL;
	fsl_free(blame->line_offsets);

	return rc;
}

static int
cancel_blame(void *state)
{
	int	*done = state;
	int	 rc = 0;

	rc = pthread_mutex_lock(&fnc_mutex);
	if (rc)
		return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_unlock");

	if (*done)
		rc = fcli_err_set(FSL_RC_BREAK, "user quit");

	rc = pthread_mutex_unlock(&fnc_mutex);
	if (rc)
		return RC(fsl_errno_to_rc(rc, FSL_RC_ACCESS),
		    "%s", "pthread_mutex_lock");

	return rc;
}

static void
fnc_commit_qid_free(struct fnc_commit_qid *qid)
{
	fsl_free(qid->id);
	fsl_free(qid);
}

static int
cmd_branch(fcli_command const *argv)
{
	struct fnc_view	*view;
	char		*glob = NULL;
	double		 dateline;
	int		 branch_flags, rc = 0, when = 0;

	rc = fcli_process_flags(argv->flags);
	if (rc || (rc = fcli_has_unused_flags(false)))
		return rc;

	branch_flags = BRANCH_LS_OPEN_CLOSED;
	if (fnc_init.open && fnc_init.closed)
		return RC(FSL_RC_MISUSE, "%s",
		    "--open and --close are mutually exclusive options");
	else if (fnc_init.open)
		branch_flags = BRANCH_LS_OPEN_ONLY;
	else if (fnc_init.closed)
		branch_flags = BRANCH_LS_CLOSED_ONLY;

	if (fnc_init.sort) {
		if (!fsl_strcmp(fnc_init.sort, "mru"))
			FLAG_SET(branch_flags, BRANCH_SORT_MTIME);
		else if (!fsl_strcmp(fnc_init.sort, "state"))
			FLAG_SET(branch_flags, BRANCH_SORT_STATUS);
		else
			return RC(FSL_RC_MISUSE, "invalid sort order: %s",
			    fnc_init.sort);
	}
	if (fnc_init.noprivate)
		FLAG_SET(branch_flags, BRANCH_LS_NO_PRIVATE);
	if (fnc_init.reverse)
		FLAG_SET(branch_flags, BRANCH_SORT_REVERSE);

	if (fnc_init.after && fnc_init.before) {
		return RC(FSL_RC_MISUSE, "%s",
		    "--before and --after are mutually exclusive options");
	} else if (fnc_init.after || fnc_init.before) {
		const char *d = NULL;
		d = fnc_init.after ? fnc_init.after : fnc_init.before;
		when = fnc_init.after ? 1 : -1;
		rc = fnc_date_to_mtime(&dateline, d, when);
		if (rc)
			return rc;
	}
	glob = fsl_strdup(fcli_next_arg(true));

	rc = init_curses();
	if (rc)
		goto end;
#ifdef __OpenBSD__
	const fsl_cx *const f = fcli_cx();
	rc = init_unveil(fsl_cx_db_file_repo(f, NULL),
	    fsl_cx_ckout_dir_name(f, NULL), false);
	if (rc)
		goto end;
#endif

	view = view_open(0, 0, 0, 0, FNC_VIEW_BRANCH);
	if (view == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "view_open");
		goto end;
	}

	rc = open_branch_view(view, branch_flags, glob, dateline, when);
	if (rc)
		goto end;

	rc = view_loop(view);
end:
	if (rc)
		fnc_free_branches(&view->state.branch.branches);
	fsl_free(glob);
	return rc;
}

static int
open_branch_view(struct fnc_view *view, int branch_flags, const char *glob,
    double dateline, int when)
{
	struct fnc_branch_view_state	*s = &view->state.branch;
	int				 rc = 0;

	s->selected_branch = 0;
	s->colour = !fnc_init.nocolour && has_colors();
	s->branch_flags = branch_flags;
	s->branch_glob = glob;
	s->dateline = dateline;
	s->when = when;

	rc = fnc_load_branches(s);
	if (rc)
		return rc;

	if (s->colour) {
		STAILQ_INIT(&s->colours);
		rc = set_colours(&s->colours, FNC_VIEW_BRANCH);
	}

	view->show = show_branch_view;
	view->input = branch_input_handler;
	view->close = close_branch_view;
	view->search_init = branch_search_init;
	view->search_next = branch_search_next;

	return rc;
}

static int
fnc_load_branches(struct fnc_branch_view_state *s)
{
	fsl_cx		*const f = fcli_cx();
	fsl_buffer	 sql = fsl_buffer_empty;
	fsl_stmt	*stmt = NULL;
	char		*curr_branch = NULL;
	fsl_id_t	 ckoutrid;
	int		 rc = 0;

	rc = create_tmp_branchlist_table();
	if (rc)
		goto end;

	TAILQ_INIT(&s->branches);
	s->nbranches = 0;

	switch (FLAG_CHK(s->branch_flags, BRANCH_LS_BITMASK)) {
	case BRANCH_LS_OPEN_CLOSED:
		rc = fsl_buffer_append(&sql,
		    "SELECT name, isprivate, isclosed, mtime"
		    " FROM tmp_brlist WHERE 1", -1);
		break;
	case BRANCH_LS_OPEN_ONLY:
		rc = fsl_buffer_append(&sql,
		    "SELECT name, isprivate, isclosed, mtime"
		    " FROM tmp_brlist WHERE NOT isclosed", -1);
		break;
	case BRANCH_LS_CLOSED_ONLY:
		rc = fsl_buffer_append(&sql,
		    "SELECT name, isprivate, isclosed, mtime"
		    " FROM tmp_brlist WHERE isclosed", -1);
		break;
	}
	if (rc)
		goto end;

	if (s->branch_glob) {
		char *op = NULL, *str = NULL;
		rc = fnc_make_sql_glob(&op, &str, s->branch_glob,
		    !fnc_str_has_upper(s->branch_glob));
		if (!rc)
			fsl_buffer_appendf(&sql, " AND name %q %Q", op, str);
		fsl_free(op);
		fsl_free(str);
		if (rc)
			goto end;
	}

	if (FLAG_CHK(s->branch_flags, BRANCH_LS_NO_PRIVATE)) {
		rc = fsl_buffer_append(&sql, " AND NOT isprivate", -1);
		if (rc)
			goto end;
	}

	if (FLAG_CHK(s->branch_flags, BRANCH_SORT_MTIME))
		rc = fsl_buffer_append(&sql, " ORDER BY -mtime", -1);
	else if (FLAG_CHK(s->branch_flags, BRANCH_SORT_STATUS))
		rc = fsl_buffer_append(&sql, " ORDER BY isclosed", -1);
	else
		rc = fsl_buffer_append(&sql, " ORDER BY name COLLATE nocase",
		    -1);
	if (!rc && FLAG_CHK(s->branch_flags, BRANCH_SORT_REVERSE))
		rc = fsl_buffer_append(&sql," DESC", -1);
	if (rc)
		goto end;

	stmt = fsl_stmt_malloc();
	if (stmt == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "fsl_stmt_malloc");
		goto end;
	}

	rc = fsl_cx_prepare(f, stmt, fsl_buffer_cstr(&sql));
	if (rc)
		goto end;

	fsl_ckout_version_info(f, &ckoutrid, NULL);
	curr_branch = fsl_db_g_text(fsl_needs_repo(f), NULL,
	    "SELECT value FROM tagxref WHERE rid=%d AND tagid=%d", ckoutrid, 8);

	while (fsl_stmt_step(stmt) == FSL_RC_STEP_ROW) {
		struct fnc_branch *new_branch;
		struct fnc_branchlist_entry *be;
		const char *brname = fsl_stmt_g_text(stmt, 0, NULL);
		bool priv = (curr_branch && fsl_stmt_g_int32(stmt, 1) == 1);
		bool open = fsl_stmt_g_int32(stmt, 2) == 0;
		double mtime = fsl_stmt_g_int64(stmt, 3);
		bool curr = curr_branch && !fsl_strcmp(curr_branch, brname);
		if ((s->when > 0 && mtime < s->dateline) ||
		    (s->when < 0 && mtime > s->dateline))
			continue;
		rc = alloc_branch(&new_branch, brname, mtime, open, priv, curr);
		if (rc)
			goto end;
		rc = fnc_branchlist_insert(&be, &s->branches, new_branch);
		if (rc)
			goto end;
		if (be)
			be->idx = s->nbranches++;
	}
	s->first_branch_onscreen = TAILQ_FIRST(&s->branches);

	if (!stmt->rowCount)
		rc = RC(FSL_RC_BREAK, "no matching records: %s",
		    s->branch_glob);
end:
	fsl_stmt_finalize(stmt);
	fsl_free(curr_branch);
	fsl_buffer_clear(&sql);
	return rc;
}

static int
create_tmp_branchlist_table(void)
{
	fsl_cx			*const f = fcli_cx();
	fsl_db			*db = fsl_needs_repo(f);  /* -R|--repo option */
	static const char	 tmp_branchlist_table[] =
	    "CREATE TEMP TABLE IF NOT EXISTS tmp_brlist AS "
	    "SELECT tagxref.value AS name,"
	    " max(event.mtime) AS mtime,"
	    " EXISTS(SELECT 1 FROM tagxref AS tx WHERE tx.rid=tagxref.rid"
	    "  AND tx.tagid=(SELECT tagid FROM tag WHERE tagname='closed')"
	    "  AND tx.tagtype > 0) AS isclosed,"
	    " (SELECT tagxref.value FROM plink CROSS JOIN tagxref"
	    "  WHERE plink.pid=event.objid"
	    "  AND tagxref.rid=plink.cid"
	    "  AND tagxref.tagid=(SELECT tagid FROM tag WHERE tagname='branch')"
	    "  AND tagtype>0) AS mergeto,"
	    " count(*) AS nckin,"
	    " (SELECT uuid FROM blob WHERE rid=tagxref.rid) AS ckin,"
	    " event.bgcolor AS bgclr,"
	    " EXISTS(SELECT 1 FROM private WHERE rid=tagxref.rid) AS isprivate "
	    "FROM tagxref, tag, event "
	    "WHERE tagxref.tagid=tag.tagid"
	    " AND tagxref.tagtype>0"
	    " AND tag.tagname='branch'"
	    " AND event.objid=tagxref.rid "
	    "GROUP BY 1;";
	int rc = 0;

	if (!db)
		return RC(FSL_RC_NOT_A_CKOUT, "%s", "fsl_needs_repo");
	rc = fsl_db_exec(db, tmp_branchlist_table);

	return rc ? RC(fsl_cx_uplift_db_error2(f, db, rc), "%s", "fsl_db_exec")
	    : rc;
}

static int
alloc_branch(struct fnc_branch **branch, const char *name, double mtime,
    bool open, bool priv, bool curr)
{
	fsl_uuid_str	id = NULL;
	char		iso8601[ISO8601_TIMESTAMP], *date = NULL;
	int		rc = 0;

	*branch = calloc(1, sizeof(**branch));
	if (*branch == NULL)
		return RC(FSL_RC_ERROR, "%s", "calloc");

	rc = fsl_sym_to_uuid(fcli_cx(), name, FSL_SATYPE_ANY, &id, NULL);
	if (rc || id == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "fsl_sym_to_uuid");
		fnc_branch_close(*branch);
		*branch = NULL;
		return rc;
	}

	fsl_julian_to_iso8601(mtime, iso8601, false);
	date = fsl_mprintf("%.*s", ISO8601_DATE_ONLY, iso8601);

	(*branch)->id = id;
	(*branch)->name = fsl_strdup(name);
	(*branch)->date = date;
	(*branch)->open = open;
	(*branch)->private = priv;
	(*branch)->current = curr;
	if ((*branch)->name == NULL) {
		rc = RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		fnc_branch_close(*branch);
		*branch = NULL;
	}

	return rc;
}

static int
fnc_branchlist_insert(struct fnc_branchlist_entry **newp,
    struct fnc_branchlist_head *branches, struct fnc_branch *branch)
{
	struct fnc_branchlist_entry *new, *be;

	*newp = NULL;

	new = fsl_malloc(sizeof(*new));
	if (new == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_malloc");
	new->branch = branch;
	*newp = new;

	be = TAILQ_LAST(branches, fnc_branchlist_head);
	if (!be) {
		/* Empty list; add first branch. */
		TAILQ_INSERT_HEAD(branches, new, entries);
		return 0;
	}

	/*
	 * Deduplicate (extremely unlikely or impossible?) entries on insert.
	 * Don't force lexicographical order; we already retrieved the branch
	 * names from the database using a query to obtain (a) lexicographical
	 * or (b) user-specified sorted results (i.e., MRU or LRU).
	 */
	while (be) {
		if (!fsl_strcmp(be->branch->name, new->branch->name)) {
			/* Duplicate entry. */
			fsl_free(new);
			*newp = NULL;
			return 0;
		}
		be = TAILQ_PREV(be, fnc_branchlist_head, entries);
	}

	/* No duplicates; add to end of list. */
	TAILQ_INSERT_TAIL(branches, new, entries);
	return 0;
}

static int
show_branch_view(struct fnc_view *view)
{
	struct fnc_branch_view_state	*s = &view->state.branch;
	struct fnc_branchlist_entry	*be;
	struct fnc_colour		*c = NULL;
	char				*line = NULL;
	wchar_t				*wline;
	int				 limit, n, width, rc = 0;

	werase(view->window);
	s->ndisplayed = 0;

	limit = view->nlines;
	if (limit == 0)
		return rc;

	be = s->first_branch_onscreen;

	if ((line = fsl_mprintf("branches [%d/%d]", be->idx + s->selected + 1,
	    s->nbranches)) == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_mprintf");

	rc = formatln(&wline, &width, line, view->ncols, 0);
	if (rc) {
		fsl_free(line);
		return rc;
	}
	if (screen_is_shared(view))
		wattron(view->window, A_REVERSE);
	waddwstr(view->window, wline);
	while (width < view->ncols) {
		waddch(view->window, ' ');
		++width;
	}
	if (screen_is_shared(view))
		wattroff(view->window, A_REVERSE);
	fsl_free(wline);
	wline = NULL;
	fsl_free(line);
	line = NULL;
	if (width < view->ncols - 1)
		waddch(view->window, '\n');
	if (--limit <= 0)
		return rc;

	n = 0;
	while (be && limit > 0) {
		char *line = NULL;

		line = fsl_mprintf(" %c%s%s%s%s%s%s%c %s",
		    be->branch->open ? '+' : '-', be->branch->name,
		    be->branch->private ? "*" : "",
		    be->branch->current ? "@" : "", s->show_id ?  " [" : "",
		    s->show_id ? be->branch->id : "", s->show_id ? "]" : "",
		    s->show_date ? ':' : 0,
		    s->show_date ? be->branch->date : 0);
		if (line == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_mprintf");

		if (s->colour)
			c = match_colour(&s->colours, line);

		rc = formatln(&wline, &width, line, view->ncols, 0);
		if (rc) {
			fsl_free(line);
			return rc;
		}

		if (n == s->selected) {
			if (view->active)
				wattr_on(view->window, A_REVERSE, NULL);
			s->selected_branch = be;
		}
		if (c)
			wattr_on(view->window, COLOR_PAIR(c->scheme), NULL);
		waddwstr(view->window, wline);
		if (c)
			wattr_off(view->window, COLOR_PAIR(c->scheme), NULL);
		if (width < view->ncols - 1)
			waddch(view->window, '\n');
		if (n == s->selected && view->active)
			wattr_off(view->window, A_REVERSE, NULL);

		fsl_free(line);
		fsl_free(wline);
		wline = NULL;
		++n;
		++s->ndisplayed;
		--limit;
		s->last_branch_onscreen = be;
		be = TAILQ_NEXT(be, entries);
	}

	drawborder(view);
	return rc;
}

static int
branch_input_handler(struct fnc_view **new_view, struct fnc_view *view, int ch)
{
	struct fnc_branch_view_state	*s = &view->state.branch;
	struct fnc_view			*timeline_view, *tree_view;
	/* struct fnc_tl_thread_cx		*tcx = NULL; */
	struct fnc_branchlist_entry	*be;
	int				 start_col = 0, n, rc = 0;

	switch (ch) {
	case 'c':
		s->colour = !s->colour;
		break;
	case 'd':
		s->show_date = !s->show_date;
		break;
	case 'i':
		s->show_id = !s->show_id;
		break;
	case KEY_ENTER:
	case '\r':
	case ' ':
		if (!s->selected_branch)
			break;
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		rc = tl_branch_entry(&timeline_view, start_col,
		    s->selected_branch);
		view->active = false;
		timeline_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, timeline_view);
			view->focus_child = true;
		} else
			*new_view = timeline_view;
		break;
	case 's':
		/*
		 * Toggle branch list sort order (cf. branch --sort option):
		 * lexicographical (default) -> most recently used -> state
		 */
		if (FLAG_CHK(s->branch_flags, BRANCH_SORT_MTIME)) {
			FLAG_CLR(s->branch_flags, BRANCH_SORT_MTIME);
			FLAG_SET(s->branch_flags, BRANCH_SORT_STATUS);
		} else if (FLAG_CHK(s->branch_flags, BRANCH_SORT_STATUS))
			FLAG_CLR(s->branch_flags, BRANCH_SORT_STATUS);
		else
			FLAG_SET(s->branch_flags, BRANCH_SORT_MTIME);
		fnc_free_branches(&s->branches);
		rc = fnc_load_branches(s);
		/* May need to reset the commit builder query. */
		/* tcx = fcli_cx()->clientState.state; */
		/* if (tcx) */
		/*	tcx->needs_reset = true; */
		break;
	case 't':
		if (!s->selected_branch)
			break;
		if (view_is_parent(view))
			start_col = view_split_start_col(view->start_col);
		rc = browse_branch_tree(&tree_view, start_col,
		    s->selected_branch);
		if (rc || tree_view == NULL)
			break;
		view->active = false;
		tree_view->active = true;
		if (view_is_parent(view)) {
			rc = view_close_child(view);
			if (rc)
				return rc;
			view_set_child(view, tree_view);
			view->focus_child = true;
		} else
			*new_view = tree_view;
		break;
	case 'g':
		if (!fnc_home(view))
			break;
		/* FALL THROUGH */
	case KEY_HOME:
		s->selected = 0;
		s->first_branch_onscreen = TAILQ_FIRST(&s->branches);
		break;
	case KEY_END:
	case 'G':
		s->selected = 0;
		be = TAILQ_LAST(&s->branches, fnc_branchlist_head);
		for (n = 0; n < view->nlines - 1; ++n) {
			if (be == NULL)
				break;
			s->first_branch_onscreen = be;
			be = TAILQ_PREV(be, fnc_branchlist_head, entries);
		}
		if (n > 0)
			s->selected = n - 1;
		break;
	case KEY_UP:
	case 'k':
		if (s->selected > 0) {
			--s->selected;
			break;
		}
		branch_scroll_up(s, 1);
		break;
	case KEY_DOWN:
	case 'j':
		if (s->selected < s->ndisplayed - 1) {
			++s->selected;
			break;
		}
		if (TAILQ_NEXT(s->last_branch_onscreen, entries) == NULL)
			/* Reached last entry. */
			break;
		branch_scroll_down(s, 1);
		break;
	case KEY_PPAGE:
	case CTRL('b'):
		if (s->first_branch_onscreen == TAILQ_FIRST(&s->branches))
			s->selected = 0;
		branch_scroll_up(s, MAX(0, view->nlines - 1));
		break;
	case KEY_NPAGE:
	case CTRL('f'):
		if (TAILQ_NEXT(s->last_branch_onscreen, entries) == NULL) {
			/* No more entries off-page; move cursor down. */
			if (s->selected < s->ndisplayed - 1)
				s->selected = s->ndisplayed - 1;
			break;
		}
		branch_scroll_down(s, view->nlines - 1);
		break;
	case CTRL('l'):
	case 'R':
		fnc_free_branches(&s->branches);
		s->branch_glob = NULL; /* Shared pointer. */
		s->when = 0;
		s->branch_flags = BRANCH_LS_OPEN_CLOSED;
		rc = fnc_load_branches(s);
		break;
	case KEY_RESIZE:
		if (view->nlines >= 2 && s->selected >= view->nlines - 1)
			s->selected = view->nlines - 2;
		break;
	default:
		break;
	}

	return rc;
}

static int
tl_branch_entry(struct fnc_view **new_view, int start_col,
    struct fnc_branchlist_entry *be)
{
	struct fnc_view	*timeline_view;
	fsl_id_t	 rid;
	int		 rc = 0;

	*new_view = NULL;

	rid = fsl_uuid_to_rid(fcli_cx(), be->branch->id);
	if (rid < 0)
		return RC(rc, "%s", "fsl_uuid_to_rid");

	timeline_view = view_open(0, 0, 0, start_col, FNC_VIEW_TIMELINE);
	if (timeline_view == NULL)
		return RC(FSL_RC_ERROR, "%s", "view_open");

	rc = open_timeline_view(timeline_view, rid, "/", NULL);
	if (!rc)
		*new_view = timeline_view;
	return rc;
}

static int
browse_branch_tree(struct fnc_view **new_view, int start_col,
    struct fnc_branchlist_entry *be)
{
	struct fnc_view	*tree_view;
	fsl_id_t	 rid;
	int		 rc = 0;

	*new_view = NULL;

	rid = fsl_uuid_to_rid(fcli_cx(), be->branch->id);
	if (rid < 0)
		return RC(rc, "%s", "fsl_uuid_to_rid");

	tree_view = view_open(0, 0, 0, start_col, FNC_VIEW_TREE);
	if (tree_view == NULL)
		return RC(FSL_RC_ERROR, "%s", "view_open");

	rc = open_tree_view(tree_view, "/", rid);
	if (!rc)
		*new_view = tree_view;
	return rc;
}

static void
branch_scroll_up(struct fnc_branch_view_state *s, int maxscroll)
{
	struct fnc_branchlist_entry	*be;
	int				 idx = 0;

	if (s->first_branch_onscreen == TAILQ_FIRST(&s->branches))
		return;

	be = TAILQ_PREV(s->first_branch_onscreen, fnc_branchlist_head, entries);
	while (idx++ < maxscroll) {
		if (be == NULL)
			break;
		s->first_branch_onscreen = be;
		be = TAILQ_PREV(be, fnc_branchlist_head, entries);
	}
}

static void
branch_scroll_down(struct fnc_branch_view_state *s, int maxscroll)
{
	struct fnc_branchlist_entry	*next, *last;
	int				 idx = 0;

	if (s->first_branch_onscreen)
		next = TAILQ_NEXT(s->first_branch_onscreen, entries);
	else
		next = TAILQ_FIRST(&s->branches);

	last = s->last_branch_onscreen;
	while (next && last && idx++ < maxscroll) {
		last = TAILQ_NEXT(last, entries);
		if (last) {
			s->first_branch_onscreen = next;
			next = TAILQ_NEXT(next, entries);
		}
	}
}

static int
branch_search_init(struct fnc_view *view)
{
	struct fnc_branch_view_state *s = &view->state.branch;

	s->matched_branch = NULL;
	return 0;
}

static int
branch_search_next(struct fnc_view *view)
{
	struct fnc_branch_view_state	*s = &view->state.branch;
	struct fnc_branchlist_entry	*be = NULL;

	if (view->searching == SEARCH_DONE) {
		view->search_status = SEARCH_CONTINUE;
		return 0;
	}

	if (s->matched_branch) {
		if (view->searching == SEARCH_FORWARD) {
			if (s->selected_branch)
				be = TAILQ_NEXT(s->selected_branch, entries);
			else
				be = TAILQ_PREV(s->selected_branch,
				    fnc_branchlist_head, entries);
		} else {
			if (s->selected_branch == NULL)
				be = TAILQ_LAST(&s->branches,
				    fnc_branchlist_head);
			else
				be = TAILQ_PREV(s->selected_branch,
				    fnc_branchlist_head, entries);
		}
	} else {
		if (view->searching == SEARCH_FORWARD)
			be = TAILQ_FIRST(&s->branches);
		else
			be = TAILQ_LAST(&s->branches, fnc_branchlist_head);
	}

	while (1) {
		if (be == NULL) {
			if (s->matched_branch == NULL) {
				view->search_status = SEARCH_CONTINUE;
				return 0;
			}
			if (view->searching == SEARCH_FORWARD)
				be = TAILQ_FIRST(&s->branches);
			else
				be = TAILQ_LAST(&s->branches,
				    fnc_branchlist_head);
		}

		if (match_branchlist_entry(be, &view->regex)) {
			view->search_status = SEARCH_CONTINUE;
			s->matched_branch = be;
			break;
		}

		if (view->searching == SEARCH_FORWARD)
			be = TAILQ_NEXT(be, entries);
		else
			be = TAILQ_PREV(be, fnc_branchlist_head, entries);
	}

	if (s->matched_branch) {
		s->first_branch_onscreen = s->matched_branch;
		s->selected = 0;
	}

	return 0;
}

static int
match_branchlist_entry(struct fnc_branchlist_entry *be, regex_t *regex)
{
	regmatch_t regmatch;

	return regexec(regex, be->branch->name, 1, &regmatch, 0) == 0;
}

static int
close_branch_view(struct fnc_view *view)
{
	struct fnc_branch_view_state *s = &view->state.branch;

	fnc_free_branches(&s->branches);
	free_colours(&s->colours);

	return 0;
}

static void
fnc_free_branches(struct fnc_branchlist_head *branches)
{
	struct fnc_branchlist_entry *be;

	while (!TAILQ_EMPTY(branches)) {
		be = TAILQ_FIRST(branches);
		TAILQ_REMOVE(branches, be, entries);
		fnc_branch_close(be->branch);
		fsl_free(be);
	}
}

static void
fnc_branch_close(struct fnc_branch *branch)
{
	fsl_free(branch->name);
	fsl_free(branch->date);
	fsl_free(branch->id);
	fsl_free(branch);
}

/*
 * Assign path to **inserted->path, with optional ->data assignment, and insert
 * in lexicographically sorted order into the doubly-linked list rooted at
 * *pathlist. If path is not unique, return without adding a duplicate entry.
 */
static int
fnc_pathlist_insert(struct fnc_pathlist_entry **inserted,
    struct fnc_pathlist_head *pathlist, const char *path, void *data)
{
	struct fnc_pathlist_entry	*new, *pe;
	int				 rc = 0;

	if (inserted)
		*inserted = NULL;

	new = fsl_malloc(sizeof(*new));
	if (new == NULL)
		return RC(FSL_RC_ERROR, "%s", "fsl_malloc");
	new->path = path;
	new->pathlen = fsl_strlen(path);
	new->data = data;

	/*
	 * Most likely, supplied paths will be sorted (e.g., fnc diff *.c), so
	 * post-order traversal will be more efficient when inserting entries.
	 */
	pe = TAILQ_LAST(pathlist, fnc_pathlist_head);
	while (pe) {
		int cmp = fnc_path_cmp(pe->path, new->path, pe->pathlen,
		    new->pathlen);
		if (cmp == 0) {
			fsl_free(new);  /* Duplicate path; don't insert. */
			return rc;
		} else if (cmp < 0) {
			TAILQ_INSERT_AFTER(pathlist, pe, new, entry);
			if (inserted)
				*inserted = new;
			return rc;
		}
		pe = TAILQ_PREV(pe, fnc_pathlist_head, entry);
	}

	TAILQ_INSERT_HEAD(pathlist, new, entry);
	if (inserted)
		*inserted = new;
	return rc;
}

static int
fnc_path_cmp(const char *path1, const char *path2, size_t len1, size_t len2)
{
	size_t	minlen;
	size_t	idx = 0;

	/* Trim any leading path separators. */
	while (path1[0] == '/') {
		++path1;
		--len1;
	}
	while (path2[0] == '/') {
		++path2;
		--len2;
	}
	minlen = MIN(len1, len2);

	/* Skip common prefix. */
	while (idx < minlen && path1[idx] == path2[idx])
		++idx;

	/* Are path lengths exactly equal (exluding path separators)? */
	if (len1 == len2 && idx >= minlen)
		return 0;

	/* Trim any redundant trailing path seperators. */
	while (path1[idx] == '/' && path1[idx + 1] == '/')
		++path1;
	while (path2[idx] == '/' && path2[idx + 1] == '/')
		++path2;

	/* Ignore trailing path separators. */
	if (path1[idx] == '/' && path1[idx + 1] == '\0' && path2[idx] == '\0')
		return 0;
	if (path2[idx] == '/' && path2[idx + 1] == '\0' && path1[idx] == '\0')
		return 0;

	/* Order children in subdirectories directly after their parents. */
	if (path1[idx] == '/' && path2[idx] == '\0')
		return 1;
	if (path2[idx] == '/' && path1[idx] == '\0')
		return -1;
	if (path1[idx] == '/' && path2[idx] != '\0')
		return -1;
	if (path2[idx] == '/' && path1[idx] != '\0')
		return 1;

	/* Character immediately after the common prefix determines order. */
	return (unsigned char)path1[idx] < (unsigned char)path2[idx] ? -1 : 1;
}

static void
fnc_pathlist_free(struct fnc_pathlist_head *pathlist)
{
	struct fnc_pathlist_entry *pe;

	while ((pe = TAILQ_FIRST(pathlist)) != NULL) {
		TAILQ_REMOVE(pathlist, pe, entry);
		free(pe);
	}
}

static void
fnc_show_version(void)
{
	printf("%s %s\n", fcli_progname(), PRINT_VERSION);
}

static int
strtonumcheck(long *ret, const char *nstr, const int min, const int max)
{
	const char	*ptr;
	int		 n;

	ptr = NULL;
	errno = 0;

	n = strtonum(nstr, min, max, &ptr);
	if (errno == ERANGE)
		return RC(FSL_RC_RANGE, "<n> out of range: -n|--limit=%s [%s]",
		    nstr, ptr);
	else if (errno != 0 || errno == EINVAL)
		return RC(FSL_RC_MISUSE, "<n> not a number: -n|--limit=%s [%s]",
		    nstr, ptr);
	else if (ptr && *ptr != '\0')
		return RC(FSL_RC_MISUSE,
		    "invalid char in <n>: -n|--limit=%s [%s]", nstr, ptr);

	*ret = n;
	return 0;
}

/*
 * Attempt to parse string d, which must resemble either an ISO8601 formatted
 * date (e.g., 2021-10-10, 2020-01-01T10:10:10), disgregarding any trailing
 * garbage or space characters such that "2021-10-10x" or "2020-01-01 10:10:10"
 * will pass, or an _unambiguous_ DD/MM/YYYY or MM/DD/YYYY formatted date. Upon
 * success, use when to determine which time component to add to the date (i.e.,
 * 1 sec before or after midnight), and convert to an mtime suitable for
 * comparisons with repository mtime fields and assign to *ret. Upon failure,
 * the error state will be updated with an appropriate error message and code.
 */
static int
fnc_date_to_mtime(double *ret, const char *d, int when)
{
	struct tm	t = {0, 0, 0, 0, 0, 0};
	char		iso8601[ISO8601_TIMESTAMP];

	/* Fill the tm structure. */
	if (strptime(d, "%Y-%m-%d", &t) == NULL) {
		/* If not YYYY-MM-DD, try MM/DD/YYYY and DD/MM/YYYY. */
		if (strptime(d, "%D", &t) != NULL) {
			/* If MM/DD/YYYY, check if it could be DD/MM/YYYY too */
			if (strptime(d, "%d/%m/%Y", &t) != NULL)
				return RC(FSL_RC_AMBIGUOUS,
				    "ambiguous date [%s]", d);
		} else if (strptime(d, "%d/%m/%Y", &t) != NULL) {
			/* If DD/MM/YYYY, check if it could be MM/DD/YYYY too */
			if (strptime(d, "%D", &t) != NULL)
				return RC(FSL_RC_AMBIGUOUS,
				    "ambiguous date [%s]", d);
		} else
			return RC(FSL_RC_TYPE, "unable to parse date: %s", d);
	}

	/* Format tm into ISO8601 string then convert to mtime. */
	if (when > 0)	/* After date d. */
		strftime(iso8601, ISO8601_TIMESTAMP, "%FT23:59:59", &t);
	else		/* Before date d. */
		strftime(iso8601, ISO8601_TIMESTAMP, "%FT00:00:01", &t);
	if (!fsl_iso8601_to_julian(iso8601, ret))
		return RC(FSL_RC_ERROR, "fsl_iso8601_to_julian(%s)", iso8601);

	return 0;
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

static bool
fnc_str_has_upper(const char *str)
{
	int	idx;

	for (idx = 0; str[idx]; ++idx)
		if (fsl_isupper(str[idx]))
			return true;

	return false;
}

/*
 * If fold is true, construct a pairing for SQL queries using the SQLite LIKE
 * operator to fold case with dynamically allocated strings such that:
 *   *op = "LIKE"
 *   *glob = "%%%%str%%%%"
 * Otherwise, construct a case-sensitive pairing:
 *   *op = "GLOB"
 *   *glob = "*str*"
 * Both *op and *glob must be disposed of by the caller. Return non-zero on
 * allocation failure, else return zero.
 */
static int
fnc_make_sql_glob(char **op, char **glob, const char *str, bool fold)
{
	if (fold) {
		*op = fsl_strdup("LIKE");
		if (*op == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		*glob = fsl_mprintf("%%%%%s%%%%", str);
		if (*glob == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_mprintf");
	} else {
		*op = fsl_strdup("GLOB");
		if (*op == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_strdup");
		*glob = fsl_mprintf("*%s*", str);
		if (*glob == NULL)
			return RC(FSL_RC_ERROR, "%s", "fsl_mprintf");
	}

	return 0;
}

#ifdef __OpenBSD__
/*
 * Read permissions for the below unveil() calls are self-evident; we need
 * to read the repository and ckout databases, and ckout dir for most all fnc
 * operations. Write and create permissions are briefly listed inline, but we
 * effectively veil the entire fs except the repo db, ckout dir, and /tmp.
 */
static int
init_unveil(const char *repodb, const char *ckoutdir, bool cfg)
{
	/* w repo db for 'fnc config' command: fnc_conf_setopt(). */
	if (unveil(repodb, cfg ? "rwc" : "rw") == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ACCESS),
		    "unveil(%s, \"rw\")", repodb);

	/* w .fslckout for fsl_ckout_changes_scan() in cmd_diff(). */
	if (unveil(ckoutdir, "rw") == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ACCESS),
		    "unveil(%s, \"rw\")", ckoutdir);

	/* rwc /tmp for tmpfile() in help(), create_diff(), and run_blame(). */
	if (unveil(P_tmpdir, "rwc") == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ACCESS),
		    "unveil(%s, \"rwc\")", P_tmpdir);

	if (unveil(NULL, NULL) == -1)
		return RC(fsl_errno_to_rc(errno, FSL_RC_ACCESS),
		    "%s", "unveil");

	return FSL_RC_OK;
}
#endif

