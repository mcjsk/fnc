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
 * repository (e.g., repo.fossil) or global (i.e., $HOME/.fossil) db. Note that
 * each setting _MAY_ be mapped to a valid enum, in which case its index here
 * _MUST_ be mapped to the index/value of its enum counterpart (i.e., either
 * explicitly set equivalent values in the enum, or use offsets when indexing
 * them).
 */
#define SETTINGS(SET)					\
	SET(FNC_START_SETTINGS)		/*  0 */	\
	SET(FNC_COLOUR_DIFF_META)	/*  1 */	\
	SET(FNC_COLOUR_DIFF_MINUS)	/*  2 */	\
	SET(FNC_COLOUR_DIFF_PLUS)	/*  3 */	\
	SET(FNC_COLOUR_DIFF_CHUNK)	/*  4 */	\
	SET(FNC_COLOUR_TREE_LINK)	/*  5 */	\
	SET(FNC_COLOUR_TREE_DIR)	/*  6 */	\
	SET(FNC_COLOUR_TREE_EXEC)	/*  7 */	\
	SET(FNC_COLOUR_COMMIT)		/*  8 */	\
	SET(FNC_COLOUR_USER)		/*  9 */	\
	SET(FNC_COLOUR_DATE)		/* 10 */	\
	SET(FNC_COLOUR_TAGS)		/* 11 */	\
	SET(FNC_EOF_SETTINGS)		/* 12 */

/*
 * To construct the string array and enum:
 *	static const char *fnc_settings[] = { SETTINGS(GEN_STR) };
 *	enum settings { SETTINGS(GEN_ENUM) };
 */
#define GEN_ENUM(_id)	_id,
#define GEN_STR(_str)	#_str,

