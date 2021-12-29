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

#define GEN_ENUM_SYM(pfx, id)	pfx##_##id
#define GEN_ENUM(name, pfx, info)				\
	enum name { info(pfx, GEN_ENUM_SYM) };

#define GEN_STR_SYM(pfx, id)	#pfx"_"#id
#define GEN_STR(name, pfx, info)				\
	static const char *name[] = { info(pfx, GEN_STR_SYM) };

/*
 * All configurable fnc settings, which can be stored in either the fossil(1)
 * repository (e.g., ./repo.fossil) or shell envvars with `export SETTING=val`.
 */
#define USER_OPTIONS(pfx, _)					\
	_(pfx, START_SETTINGS),					\
	_(pfx, COLOUR_COMMIT),					\
	_(pfx, COLOUR_USER),					\
	_(pfx, COLOUR_DATE),					\
	_(pfx, COLOUR_DIFF_META),				\
	_(pfx, COLOUR_DIFF_MINUS),				\
	_(pfx, COLOUR_DIFF_PLUS),				\
	_(pfx, COLOUR_DIFF_CHUNK),				\
	_(pfx, COLOUR_DIFF_TAGS),				\
	_(pfx, COLOUR_TREE_LINK),				\
	_(pfx, COLOUR_TREE_DIR),				\
	_(pfx, COLOUR_TREE_EXEC),				\
	_(pfx, COLOUR_BRANCH_OPEN),				\
	_(pfx, COLOUR_BRANCH_CLOSED),				\
	_(pfx, COLOUR_BRANCH_CURRENT),				\
	_(pfx, COLOUR_BRANCH_PRIVATE),				\
	_(pfx, COLOUR_HL_LINE),					\
	_(pfx, COLOUR_HL_SEARCH),				\
	_(pfx, VIEW_SPLIT_MODE),				\
	_(pfx, VIEW_SPLIT_WIDTH),				\
	_(pfx, VIEW_SPLIT_HEIGHT),				\
	_(pfx, EOF_SETTINGS)

#define LINE_ATTR_ENUM(pfx, _)					\
	_(pfx, AUTO),						\
	_(pfx, MONO)

#define INPUT_TYPE_ENUM(pfx, _)					\
	_(pfx, ALPHA),						\
	_(pfx, NUMERIC)

#define ENUM_INFO(_)						\
	_(fnc_opt_id, FNC, USER_OPTIONS)			\
	_(line_attr, SLINE, LINE_ATTR_ENUM)			\
	_(input_type, INPUT, INPUT_TYPE_ENUM)

#define GEN_ENUMS(name, pfx, info) GEN_ENUM(name, pfx, info)
ENUM_INFO(GEN_ENUMS)

#define STR_INFO(_)						\
	_(fnc_opt_name, FNC, USER_OPTIONS)

#define GEN_STRINGS(name, pfx, info) GEN_STR(name, pfx, info)
STR_INFO(GEN_STRINGS)
