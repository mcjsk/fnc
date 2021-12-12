/*
 * Copyright (c) 2021 Mark Jamsek <mark@jamsek.com>
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
 * All configurable fnc settings, which can be stored in either the fossil(1)
 * repository (e.g., ./repo.fossil) or shell envvars with `export SETTING=val`.
 */
#define SETTINGS(SET)						\
	SET(START_SETTINGS),			/*  0 */	\
	SET(COLOUR_COMMIT),			/*  1 */	\
	SET(COLOUR_USER),			/*  2 */	\
	SET(COLOUR_DATE),			/*  3 */	\
	SET(COLOUR_DIFF_META),			/*  4 */	\
	SET(COLOUR_DIFF_MINUS),			/*  5 */	\
	SET(COLOUR_DIFF_PLUS),			/*  6 */	\
	SET(COLOUR_DIFF_CHUNK),			/*  7 */	\
	SET(COLOUR_DIFF_TAGS),			/*  8 */	\
	SET(COLOUR_TREE_LINK),			/*  9 */	\
	SET(COLOUR_TREE_DIR),			/* 10 */	\
	SET(COLOUR_TREE_EXEC),			/* 11 */	\
	SET(COLOUR_BRANCH_OPEN),		/* 12 */	\
	SET(COLOUR_BRANCH_CLOSED),		/* 13 */	\
	SET(COLOUR_BRANCH_CURRENT),		/* 14 */	\
	SET(COLOUR_BRANCH_PRIVATE),		/* 15 */	\
	SET(VIEW_SPLIT_MODE),			/* 16 */	\
	SET(VIEW_SPLIT_WIDTH),			/* 17 */	\
	SET(VIEW_SPLIT_HEIGHT),			/* 18 */	\
	SET(EOF_SETTINGS)

#define GEN_ENUM(_id)	FNC_##_id
#define GEN_STR(_id)	("FNC_" #_id)

enum fnc_opt_id {
	SETTINGS(GEN_ENUM)
};

static const char *fnc_opt_name[] = {
	SETTINGS(GEN_STR)
};
