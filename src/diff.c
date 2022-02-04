/*
 * Copyright (c) 2022 Mark Jamsek <mark@jamsek.com>
 * Copyright (c) 2013-2021 Stephan Beal, the Libfossil authors and contributors.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file contains the original libfossil diff implementation, published by
 * Stephan Beal, which has been superceded by libfossil diff 2.0. The code has
 * been imported with the author's blessing to hack on for fnc's opinionated
 * diff features.
 */

#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>  /* memmove() */

#include "diff.h"

#define nitems(_a)	(sizeof((_a)) / sizeof((_a)[0]))
#define MIN(_a, _b)	((_a) < (_b) ? (_a) : (_b))
#define MAX(_a, _b)	((_a) > (_b) ? (_a) : (_b))
#define starts_with(_str, _pfx) (!fsl_strncmp(_str, _pfx, sizeof(_pfx) - 1))
#define LENGTH(_ln)	((_ln)->n)  /* Length of a fsl_dline */
#define UTF_CONT(_ch)	(((_ch) & 0xc0) == 0x80)
#define FLAG_SET(f, b)	((f) |= (b))
#define FLAG_CHK(f, b)	((f) & (b))
#define FLAG_TOG(f, b)	((f) ^= (b))
#define FLAG_CLR(f, b)	((f) &= ~(b))

/*
 * Column indices for struct sbsline.cols[]
 */
#define SBS_LLINE 0	/* Left line number */
#define SBS_LTEXT 1	/* Left text */
#define SBS_MID   2	/* Middle separator column */
#define SBS_RLINE 3	/* Right line number */
#define SBS_RTEXT 4	/* Right text */

/*
 * ANSI escape codes: https://en.wikipedia.org/wiki/ANSI_escape_code
 */
#define ANSI_COLOR_BLACK(BOLD)		((BOLD) ? "\x1b[30m" : "\x1b[30m")
#define ANSI_COLOR_RED(BOLD)		((BOLD) ? "\x1b[31;1m" : "\x1b[31m")
#define ANSI_COLOR_GREEN(BOLD)		((BOLD) ? "\x1b[32;1m" : "\x1b[32m")
#define ANSI_COLOR_YELLOW(BOLD)		((BOLD) ? "\x1b[33;1m" : "\x1b[33m")
#define ANSI_COLOR_BLUE(BOLD)		((BOLD) ? "\x1b[34;1m" : "\x1b[34m")
#define ANSI_COLOR_MAGENTA(BOLD)	((BOLD) ? "\x1b[35;1m" : "\x1b[35m")
#define ANSI_COLOR_CYAN(BOLD)		((BOLD) ? "\x1b[36;1m" : "\x1b[36m")
#define ANSI_COLOR_WHITE(BOLD)		((BOLD) ? "\x1b[37;1m" : "\x1b[37m")
#define ANSI_DIFF_ADD(BOLD)		ANSI_COLOR_GREEN(BOLD)
#define ANSI_DIFF_RM(BOLD)		ANSI_COLOR_RED(BOLD)
#define ANSI_DIFF_MOD(BOLD)		ANSI_COLOR_BLUE(BOLD)
#define ANSI_BG_BLACK(BOLD)		((BOLD) ? "\x1b[40;1m" : "\x1b[40m")
#define ANSI_BG_RED(BOLD)		((BOLD) ? "\x1b[41;1m" : "\x1b[41m")
#define ANSI_BG_GREEN(BOLD)		((BOLD) ? "\x1b[42;1m" : "\x1b[42m")
#define ANSI_BG_YELLOW(BOLD)		((BOLD) ? "\x1b[43;1m" : "\x1b[43m")
#define ANSI_BG_BLUE(BOLD)		((BOLD) ? "\x1b[44;1m" : "\x1b[44m")
#define ANSI_BG_MAGENTA(BOLD)		((BOLD) ? "\x1b[45;1m" : "\x1b[45m")
#define ANSI_BG_CYAN(BOLD)		((BOLD) ? "\x1b[46;1m" : "\x1b[46m")
#define ANSI_BG_WHITE(BOLD)		((BOLD) ? "\x1b[47;1m" : "\x1b[47m")
#define ANSI_RESET_COLOR		"\x1b[39;49m"
#define ANSI_RESET_ALL			"\x1b[0m"
#define ANSI_RESET			ANSI_RESET_ALL
/* #define ANSI_BOLD     ";1m" */

int
fnc_diff_text_raw(fsl_buffer const *blob1, fsl_buffer const *blob2,
    int flags, int **out)
{
	return fnc_diff_blobs(blob1, blob2, NULL, NULL, NULL, NULL, 0, 0,
	    flags, out);
}

int
fnc_diff_text_to_buffer(fsl_buffer const *blob1, fsl_buffer const *blob2,
    fsl_buffer *out, enum line_type **lines, uint32_t *nlines, short context,
    short sbswidth, int flags)
{
	return (blob1 && blob2 && out) ?
	    fnc_diff_blobs(blob1, blob2, fsl_output_f_buffer, out, lines,
	    nlines, context, sbswidth, flags, NULL) : FSL_RC_MISUSE;
}

int
fnc_diff_text(fsl_buffer const *blob1, fsl_buffer const *blob2,
    fsl_output_f out, void *state, short context, short sbswidth, int flags)
{
	return fnc_diff_blobs(blob1, blob2, out, state, NULL, NULL,
	    context, sbswidth, flags, NULL);
}

/*
 *
 * Diff two arbitrary blobs and either stream output to the third argument
 * or return an array of copy/delete/insert triples via the final argument.
 * The third XOR final argument must be set.
 *
 * If the third argument is not NULL:
 *   state     opaque state value passed to the third when emitting output
 *   context   number of context lines (negative values fallback to default)
 *   sbswidth  sbs diff width (0 = unidiff; negative values fallback to default)
 *
 * If the final argument is not NULL, it is assigned a pointer to the result
 * array of copy/delete/insert triples. Ownership is transfered to the caller,
 * who must eventually dispose of it with fsl_free().
 *
 * Return 0 on success, any number of other codes on error.
 */
int
fnc_diff_blobs(fsl_buffer const *blob1, fsl_buffer const *blob2,
    fsl_output_f out, void *state, enum line_type **lines, uint32_t *nlines,
    /* void *regex, */ uint16_t context, short sbswidth, int flags,
    int **rawdata)
{
	fsl__diff_cx	c = fsl__diff_cx_empty;
	int		rc;

	if (!blob1 || !blob2 || (out && rawdata) || (!out && !rawdata))
		return FSL_RC_MISUSE;

	if (context < 0)
		context = 5;
	else if (context & ~FSL__LINE_LENGTH_MASK)
		context = FSL__LINE_LENGTH_MASK;

	if (FLAG_CHK(flags, FNC_DIFF_INVERT)) {
		fsl_buffer const *tmp = blob1;
		blob1 = blob2;
		blob2 = tmp;
	}

	if ((flags & FNC_DIFF_IGNORE_ALLWS) == FNC_DIFF_IGNORE_ALLWS)
		c.cmpLine = fsl_dline_cmp_ignore_ws;
	else
		c.cmpLine = fsl_dline_cmp;

	/* Prepare input files. */
	rc = fsl_break_into_dlines(fsl_buffer_cstr(blob1),
	    (fsl_int_t)fsl_buffer_size(blob1), (uint32_t*)&c.nFrom, &c.aFrom,
	    flags);
	if (rc)
		goto end;
	rc = fsl_break_into_dlines(fsl_buffer_cstr(blob2),
	    (fsl_int_t)fsl_buffer_size(blob2), (uint32_t*)&c.nTo, &c.aTo,
	    flags);
	if (rc)
		goto end;

	/* Compute the difference */
	rc = fsl__diff_all(&c);
	/* fsl__dump_triples(&c, __FILE__, __LINE__); */  /* DEBUG */
	if (rc)
		goto end;
	if (FLAG_CHK(flags, FNC_DIFF_NOTTOOBIG)) {
		int i, m, n;
		int *a = c.aEdit;
		int mx = c.nEdit;

		for (i = m = n = 0; i < mx; i += 3) {
			m += a[i];
			n += a[i + 1] + a[i + 2];
		}
		if (!n || n > 10000) {
			rc = FSL_RC_RANGE;
			/* diff_errmsg(out, DIFF_TOO_MANY_CHANGES, flags); */
			goto end;
		}
	}
	/* fsl__dump_triples(&c, __FILE__, __LINE__); */  /* DEBUG */
	if (!FLAG_CHK(flags, FNC_DIFF_NOOPT))
		fsl__diff_optimize(&c);
	/* fsl__dump_triples(&c, __FILE__, __LINE__); */  /* DEBUG */

	/*
	 * For SBS diffs, compute total number of lines and set file column
	 * width to the longest line in the diff and encode result in flags.
	 * XXX I'm ambivalent about converting unified diffs to the line_type
	 * interface as it's extra allocations for no immediate gain but creates
	 * opportunities to do cool things; leave it till we want to do them?
	 */
	if ((!sbswidth && FLAG_CHK(flags, FNC_DIFF_SIDEBYSIDE)) ||
	    sbswidth < 0) {
		flags |= ((int)(sbswidth & 0xFFFF)) << 8;
		FLAG_SET(flags, FNC_DIFF_SIDEBYSIDE);
		rc = alloc_lines_and_width(&c, &flags, context, *nlines, lines);
		if (rc)
			goto end;
	}

	if (out) {
		/*
		 * Compute a context or side-by-side diff.
		 * XXX Missing regex support.
		 */
		struct diff_out_state dos = diff_out_state_empty;
		if (FLAG_CHK(flags, FNC_DIFF_PROTOTYPE)) {
			memset(&dos.proto, 0, sizeof(dos.proto));
			dos.proto.file = blob1;
		}
		dos.out = out;
		dos.state = state;
		dos.lines = *lines;
		dos.idx = nlines;
		dos.ansi = !!(FLAG_CHK(flags, FNC_DIFF_ANSI_COLOR));
		if (FLAG_CHK(flags, FNC_DIFF_SIDEBYSIDE))
			rc = sbsdiff(&c, &dos, NULL /*regex*/, context, flags);
		else
			rc = unidiff(&c, &dos, NULL /*regex*/, context, flags);
	} else if (rawdata) {
		/* Return array of COPY/DELETE/INSERT triples. */
		*rawdata = c.aEdit;
		c.aEdit = NULL;
	}
end:
	fsl_free(c.aFrom);
	fsl_free(c.aTo);
	fsl_free(c.aEdit);
	return rc;
}

/*
 * Iterate copy elements in the c->aEdit 3-tuple to compute start and end lines,
 * including surrounding ctxt, of each chunk in the diff. Add result to nlines,
 * allocate the total and assign to *lines, which must be disposed of by the
 * caller. For SBS diffs, find the longest line and encode the length in *flags.
 */
int
alloc_lines_and_width(fsl__diff_cx *c, int *flags, uint16_t ctxt,
    uint32_t nlines, enum line_type **lines)
{
	int		i, j, k;
	int		endl, endr, start; /* Start and end of l+r chunks */
	uint32_t	n = 0;		   /* Number of lines in the diff */
	unsigned short	sz, sbswidth;	   /* Size of each line and max width */

	j = endl = endr = start = 0;
	k = c->nEdit;
	sbswidth = (*flags & FNC_DIFF_SIDEBYSIDE) ? sbsdiff_width(*flags) : 0;

	/* Find minimal copy/delete/insert triples. */
	while (k > 2 && c->aEdit[k - 1] == 0 && c->aEdit[k - 2] == 0)
		k -= 3;

	/*
	 * c[0] = first line in the diff and we accumulate each c[n += 3] to
	 * find the start of the next change (after adding any insertions);
	 * that is, each copy element by itself does not correspond to a line.
	 */
	do {
		int e, e2, s;

		/*
		 * Compute number of lines in both the left and right chunk.
		 * Both chunks will have the _same_ start line, but may have
		 * distinct end lines.
		 */
		start += c->aEdit[j];		  /* Chunk start line */
		endl = endr = start;
		endl += MAX(c->aEdit[j + 1], 0);  /* End of left side chunk */
		endr += MAX(c->aEdit[j + 2], 0);  /* End of right side chunk */
		s = MAX(start - ctxt, 0);	  /* Include leading ctxt */
		e = MIN(endl + ctxt, c->nFrom);	  /* L include trailing ctxt */
		e2 = MIN(endr + ctxt, c->nTo);	  /* R include trailing ctxt */
		n += MAX(MAX(e, e2) - s + ctxt + 2, 0); /* Max lines in chunk */

		/*
		 * Now find the longest line. We'll make both the left and right
		 * columns the width of the longest line from either side.
		 * XXX Consider keeping max width of both sides to make each
		 * column only as wide as its longest line; we can set a min
		 * default in diffs with added/removed files.
		 */
		for (i = s; sbswidth && (i < e || i < e2); ++i) {
			if (i < e) {
				/* Left side line length */
				sz = c->aFrom[i].n;
				if (sz > sbswidth)
					sbswidth = etcount(c->aFrom[i].z, sz);
			}
			if (i < e2) {
				/* Right side line length */
				sz = c->aTo[i].n;
				if (sz > sbswidth)
					sbswidth = etcount(c->aTo[i].z, sz);
			}
		}
		if (i >= c->nFrom && i >= c->nTo)
			break;

		/* Account for insertions before adding lines to next chunk. */
		start += c->aEdit[j + 2];
	} while ((j += 3) < k - 3);

	*flags |= ((int)(sbswidth & 0xFFF)) << 16;  /* Encode max width */

	*lines = fsl_realloc(*lines, (nlines + n) * sizeof(enum line_type *));
	memset((*lines + nlines), 0, sizeof(enum line_type *) * n);

	return *lines ? FSL_RC_OK : FSL_RC_ERROR;
}

/*
 * Convert str byte size n to actual column width by expanding tabs and
 * accounting for unicode continuation bytes. Return the result.
 */
unsigned short
etcount(const char *str, unsigned short n)
{
	unsigned short c = 0;

	while (str && str[c] != '\n') {
		/* Expand tabs */
		if (str[c] == '\t')
			n += 8 - (c % 8);
		if (UTF_CONT(str[c]))
			--n;
		++c;
	}

	return n;
}

/*
 * Convert mask of public fnc_diff_flag (32-bit) values to the Fossil-internal
 * 64-bit bitmask used by the DIFF_xxx macros because fossil(1) uses the macro
 * approach and a low-level encoding of data in the bitmask (e.g., the context
 * lines count). The public API hides the lower-level flags and allows the
 * internal API to take care of the encoding.
 */
uint64_t
fnc_diff_flags_convert(int mask)
{
	uint64_t rc = 0U;

#define DO(f)	if ((mask & f) == f) rc |= (((uint64_t)f) << 24)
	DO(FNC_DIFF_IGNORE_EOLWS);
	DO(FNC_DIFF_IGNORE_ALLWS);
	DO(FSL_DIFF2_LINE_NUMBERS);
	DO(FNC_DIFF_SIDEBYSIDE);
	DO(FNC_DIFF_NOTTOOBIG);
	DO(FNC_DIFF_STRIP_EOLCR);
	DO(FNC_DIFF_VERBOSE);
	DO(FNC_DIFF_BRIEF);
	DO(FNC_DIFF_HTML);
	DO(FNC_DIFF_NOOPT);
	DO(FNC_DIFF_INVERT);
#undef DO
	return rc;
}

static inline int
min(int a, int b)
{
	return a < b ? a : b;
}

/*
 * Return the number between 0 and 100 that is smaller the closer lline and
 * rline match. Return 0 for a perfect match. Return 100 if lline and rline
 * are completely different. The current algorithm is as follows:
 *
 *   1. Remove leading and trailing whitespace.
 *   2. Truncate both strings to at most 250 characters
 *   3. Find the length of the longest common subsequence
 *   4. Longer common subsequences yield lower scores.
 */
int
match_dline(fsl_dline *lline, fsl_dline *rline)
{
	const char	*l, *r;		/* Left and right strings */
	int		 nleft, nright;	/* Bytes in l and r */
	int		 avg;		/* Average length of l and r */
	int		 i, j, k;	/* Loop counters */
	int		 best = 0;	/* Current longest match found */
	int		 score;		/* Final score (0-100) */
	unsigned char	 c;		/* Character being examined */
	unsigned char	 idx1[256];	/* idx1[c]: r[] idx of first char c */
	unsigned char	 idx2[256];	/* idx2[i]: r[] idx of next r[i] char */

	l = lline->z;
	r = rline->z;
	nleft = lline->n;
	nright = rline->n;

	/* Consume leading and trailing whitespace of l string. */
	while (nleft > 0 && fsl_isspace(l[0])) {
		nleft--;
		l++;
	}
	while (nleft > 0 && fsl_isspace(l[nleft - 1]))
		nleft--;

	/* Consume leading and trailing whitespace of r string. */
	while (nright > 0 && fsl_isspace(r[0])) {
		nright--;
		r++;
	}
	while (nright > 0 && fsl_isspace(r[nright-1]))
		nright--;

	/* If needed, truncate strings to 250 chars, and find average length. */
	nleft = min(nleft, 250);
	nright = min(nright, 250);
	avg = (nleft + nright) / 2;
	if (avg == 0)
		return 0;

	/* If equal, return max score. */
	if (nleft == nright && !memcmp(l, r, nleft))
		return 0;

	memset(idx1, 0, sizeof(idx1));

	/* Make both l[] and r[] 1-indexed */
	l--;
	r--;

	/* Populate character index. */
	for (i = nright; i > 0; i--) {
		c = (unsigned char)r[i];
		idx2[i] = idx1[c];
		idx1[c] = i;
	}

	/* Find longest common subsequence. */
	best = 0;
	for (i = 1; i <= nleft - best; i++) {
		c = (unsigned char)l[i];
		for (j = idx1[c]; j > 0 && j < nright - best; j = idx2[j]) {
			int limit = min(nleft - i, nright - j);
			for (k = 1; k <= limit && l[k + i] == r[k + j]; k++){}
			if (k > best)
				best = k;
		}
	}
	score = (best > avg) ? 0 : (avg - best) * 100 / avg;

#if 0
	fprintf(stderr, "A: [%.*s]\nright: [%.*s]\nbest=%d avg=%d score=%d\n",
			nleft, l+1, nright, r+1, best, avg, score);
#endif

	return score;  /* Return the result */
}

/*
 * The two text segments left and right are known to be different on both ends,
 * but they might have a common segment in the middle. If they do not have a
 * common segment, return false. If they do have a large common segment,
 * identify bounds and return true.
 *   left	 String on the left
 *   int nleft	 Bytes in left
 *   right	 String on the right
 *   int nright	 Bytes in right
 *   lcs	 Identify bounds of LCS here
 *
 * Bounds are set as follows:
 *   lcs[0] = start of the common segment in left
 *   lcs[1] = end of the common segment in left
 *   lcs[2] = start of the common segment in right
 *   lcs[3] = end of the common segment in right
 *
 * n.b. This computation is for display purposes only and does not have to be
 * optimal or exact.
 */
bool
find_lcs(const char *left, int nleft, const char *right,
    int nright, int *lcs)
{
	const unsigned char	*l, *r;		/* Left and right strings */
	unsigned int		 probe;		/* Probe to compare target */
	unsigned int		 t[3];		/* 4-byte alignment targets */
	int			 ntargets;	/* Number of target points */
	int			 t_idx[3];	/* Index of each target */
	int	l_start, l_end, r_start, r_end;	/* Range of common segment */
	int	i, j;
	bool	rc = false;

	if (nleft < 6 || nright < 6)
		return rc;

	l = (const unsigned char*)left;
	r = (const unsigned char*)right;

	memset(lcs, 0, sizeof(int) * 4);
	t_idx[0] = i = nright / 2 - 2;
	t[0] = (r[i] << 24) | (r[i + 1] << 16) | (r[i + 2] << 8) | r[i + 3];
	probe = 0;

	if (nright < 16)
		ntargets = 1;
	else {
		t_idx[1] = i = nright / 4 - 2;
		t[1] = (r[i] << 24) | (r[i + 1] << 16) |
		    (r[i + 2] << 8) | r[i + 3];
		t_idx[2] = i = (nright * 3) / 4 - 2;
		t[2] = (r[i] << 24) | (r[i + 1] << 16) |
		    (r[i + 2] << 8) | r[i + 3];
		ntargets = 3;
	}

	probe = (l[0] << 16) | (l[1] << 8) | l[2];
	for (i = 3; i < nleft; i++) {
		probe = (probe << 8) | l[i];
		for (j = 0; j < ntargets; j++) {
			if (probe == t[j]) {
				l_start = i - 3;
				l_end = i + 1;
				r_start = t_idx[j];
				r_end = t_idx[j] + 4;
				while (l_end < nleft && r_end < nright &&
				    l[l_end] == r[r_end]) {
					l_end++;
					r_end++;
				}
				while (l_start > 0 && r_start > 0 &&
				    l[l_start - 1] == r[r_start - 1]) {
					l_start--;
					r_start--;
				}
				if (l_end - l_start > lcs[1] - lcs[0]) {
					lcs[0] = l_start;
					lcs[1] = l_end;
					lcs[2] = r_start;
					lcs[3] = r_end;
					rc = true;
				}
			}
		}
	}
	return rc;
}

/*
 * Send src to o->out(). If n is negative, use strlen() to determine length.
 */
int
diff_out(struct diff_out_state *const o, void const *src, fsl_int_t n)
{
	return o->rc = n ?
	    o->out(o->state, src, n < 0 ? strlen((char const *)src) : (size_t)n)
	    : 0;
}

/*
 * Starting from seek lines, copy n lines from src to dst. The seek starts
 * from offset bytes into src->mem. This routine does _not_ modify src.
 */
int
buffer_copy_lines_from(fsl_buffer *const dst, const fsl_buffer *const src,
    fsl_size_t *offset, fsl_size_t seek, fsl_size_t n)
{
	const char *z = (const char *)src->mem;
	fsl_size_t idx = *offset, ln = 0, start = 0;
	int rc = FSL_RC_OK;

	if (!n)
		return rc;

	while (idx < src->used) {
		if (z[idx] == '\n') {
			if (++ln == seek)
				start = idx + 1;  /* skip leading '\n' */
			if (ln == seek + n) {
				++idx;
				break;
			}
		}
		++idx;
	}

	if (dst)  /* trim trailing when copying '\n' */
		rc = fsl_buffer_append(dst, &src->mem[start], idx - start - 1);
	*offset = start;
	return rc;
}

/*
 * Scan the diffed file dst->proto->file from line pos preceding the start of
 * the current chunk for the enclosing function in which the change resides.
 * Return first match.
 */
char *
match_chunk_function(struct diff_out_state *const dst, uint32_t pos)
{
	fsl_buffer	 buf = fsl_buffer_empty;
	const char	*line;
	char		*spec = NULL;
	fsl_size_t	 offset;
	uint32_t	 last = dst->proto.lastline;

	dst->proto.lastline = pos;
	offset = dst->proto.offset;  /* Begin seek from last match */

	while (pos > 1 && pos > last) {
		buffer_copy_lines_from(&buf, dst->proto.file, &offset,
		    pos - dst->proto.lastmatch, 1);
		line = fsl_buffer_cstr(&buf);
		/*
		 * GNU C and MSVC allow '$' in identifier names.
		 * https://gcc.gnu.org/onlinedocs/gcc/Dollar-Signs.html
		 * https://docs.microsoft.com/en-us/cpp/cpp/identifiers-cpp
		 */
		if (line) {
			if (fsl_isalpha(line[0]) || line[0] == '_' ||
			    line[0] == '$'){
				if (starts_with(line, "private:")) {
					if (!spec)
						spec = " (private)";
				} else if (starts_with(line, "protected:")) {
					if (!spec)
						spec = " (protected)";
				} else if (starts_with(line, "public:")) {
					if (!spec)
						spec = " (public)";
				} else {
					/*
					 * Don't exceed 80 cols: chunk header
					 * consumes ~25, so cap signature at 55.
					 */
					char *sig = fsl_mprintf("%s%s", line,
					    spec ? spec : "");
					fsl_free(dst->proto.signature);
					dst->proto.signature =
					    fsl_mprintf("%.55s", sig);
					/*
					 * It's expensive to seek from the start
					 * of the file for each chunk when
					 * diffing large files, so save offset
					 * and line index of this match.
					 */
					dst->proto.lastmatch = pos;
					dst->proto.offset = offset;
					fsl_free(sig);
					fsl_buffer_clear(&buf);
					return dst->proto.signature;
				}
			}
		}
		/* No match, revert to last offset. */
		offset = dst->proto.offset;
		fsl_buffer_clear(&buf);
		--pos;
	}
	return dst->proto.lastmatch > 0 ? dst->proto.signature : NULL;
}

/*
 * Render the diff triples array in cx->aEdit as a side-by-side diff in out.
 *   cx		Raw diff data
 *   out	Side-by-side diff representation
 *   regex	Show changes matching this regex
 *   context	Number of context lines
 *   flags	Flags controlling the diff
 */
int
sbsdiff(fsl__diff_cx *cx, struct diff_out_state *dst, void *regex,
    uint16_t context, uint64_t flags)
{
	fsl_dline	*l, *r;		/* Left and right side of diff */
	fsl_buffer	 sbscols[5] = {
			    fsl_buffer_empty_m, fsl_buffer_empty_m,
			    fsl_buffer_empty_m, fsl_buffer_empty_m,
			    fsl_buffer_empty_m
			 };
	struct sbsline	 s;		/* Output line buffer */
	static int	 chunks = 0;	/* Number of chunks so far processed */
	int		 li, ri;	/* Index of next line in l[] and r[] */
	int		*c;		/* copy/delete/insert triples */
	int		 ci;		/* Index into c[] */
	int		 nc;		/* number of c[] triples to process */
	int		 max_ci;	/* Maximum value for ci */
	int		 nleft, nright; /* Number of l and r lines to output */
	int		 ntotal;	/* Total number of lines to output */
	int		 skip;		/* Number of lines to skip */
	int		 i, j, rc = FSL_RC_OK;
	uint32_t	*idx = dst->idx;
	enum line_type	*lines = dst->lines;
	bool		 showsep = false;

	li = ri = 0;
	memset(&s, 0, sizeof(s));
	s.output = dst;
	s.width = sbsdiff_width(flags);
	s.regex = regex;
	s.idx = -1;
	s.idx2 = 0;
	s.end = -1;
	max_ci = cx->nEdit;
	l = cx->aFrom;
	r = cx->aTo;
	c = cx->aEdit;
	s.esc = FLAG_CHK(flags, FNC_DIFF_HTML);

	if (s.esc)
		for (i = SBS_LLINE; i <= SBS_RTEXT; i++)
			s.cols[i] = &sbscols[i];
	else
		for (i = SBS_LLINE; i <= SBS_RTEXT; i++)
			s.cols[i] = (fsl_buffer *)dst->state;

	while (max_ci > 2 && c[max_ci-1] == 0 && c[max_ci - 2] == 0)
		max_ci -= 3;

	for (ci = 0; ci < max_ci; ci += 3 * nc) {  /* _huge_ loop */
		/* Calculate how many triples to show in a single block */
		for (nc = 1; c[ci + nc * 3] > 0 &&
		    c[ci + nc * 3] < context * 2; nc++) {}
		/* printf("ci=%d nc=%d\n", ci, nc); */
#if 0
		/*
		 * XXX Missing: re/predicate bits.
		 * If there is a regex, skip this block (i.e., generate no diff
		 * output) if the regex matches or does not match both insert
		 * and delete. Only display the block if one side matches but
		 * the other side does not.
		 */
		if (regex) {
			bool hidechange = true;
			int xa = li, xb = ri;
			for (i = 0; hidechange && i < nc; i++) {
				int c1, c2;
				xa += c[ci + i * 3];
				xb += c[ci + i * 3];
				c1 = re_dline_match(regex, &l[xa],
				    c[ci + i * 3 + 1]);
				c2 = re_dline_match(regex, &r[xb],
				    c[ci + i * 3 + 2]);
				hidechange = c1 == c2;
				xa += c[ci + i * 3 + 1];
				xb += c[ci + i * 3 + 2];
			}
			if (hidechange) {
				li = xa;
				ri = xb;
				continue;
			}
		}
#endif
		/*
		 * For the current block comprising nc triples, figure out
		 * how many lines to skip.
		 */
		/* if (c[ci] > context) */
		/* 	skip = c[ci] - context; */
		/* else */
		/* 	skip = 0; */
		if (c[ci] > context) {
			nleft = nright = context;
			skip = c[ci] - context;
		} else {
			nleft = nright = c[ci];
			skip = 0;
		}
		for (i = 0; i < nc; ++i) {
			nleft += c[ci + i * 3 + 1];
			nright += c[ci + i * 3 + 2];
		}
		if (c[ci + nc * 3] > context) {
			nleft += context;
			nright += context;
		} else {
			nleft += c[ci + nc * 3];
			nright += c[ci + nc * 3];
		}
		for (i = 1; i < nc; ++i) {
			nleft += c[ci + i * 3];
			nright += c[ci + i * 3];
		}

		/* Draw separator between blocks except the first. */
		if (showsep) {
			lines[(*idx)++] = LINE_DIFF_SEPARATOR;
			if (s.esc) {
				char ln[10];
				fsl_snprintf(ln, sizeof(ln), "%d",
				    li + skip + 1);
				rc = sbsdiff_separator(&s, fsl_strlen(ln),
				    SBS_LLINE);
				if (rc)
					goto end;
				rc = sbsdiff_separator(&s, s.width, SBS_LTEXT);
				if (!rc)
					rc = sbsdiff_separator(&s, 0, SBS_MID);
				if (rc)
					goto end;
				fsl_snprintf(ln, sizeof(ln), "%d",
				    ri + skip + 1);
				rc = sbsdiff_separator(&s, fsl_strlen(ln),
				    SBS_RLINE);
				if (rc)
					goto end;
				rc = sbsdiff_separator(&s, s.width, SBS_RTEXT);
			} else
				rc = diff_outf(s.output, "%.*c\n",
				    s.width * 2 + 16, '.');
			if (rc)
				goto end;
		}
		showsep = true;
		++chunks;
		if (s.esc)
			rc = fsl_buffer_appendf(s.cols[SBS_LLINE],
			    "<span class=\"fsl-diff-chunk-%d\"></span>",
			    chunks);

		/* Show the initial common area */
		li += skip;
		ri += skip;
		ntotal = c[ci] - skip;
		for (j = 0; !rc && j < ntotal; j++) {
			rc = sbsdiff_lineno(&s, li + j, SBS_LLINE);
			if (rc)
				break;
			s.idx = s.end = -1;
			rc = sbsdiff_txt(&s, &l[li + j], SBS_LTEXT);
			if (!rc)
				rc = sbsdiff_marker(&s, "   ", "");
			if (!rc)
				rc = sbsdiff_lineno(&s, ri + j, SBS_RLINE);
			if (!rc)
				rc = sbsdiff_txt(&s, &r[ri + j], SBS_RTEXT);
			lines[(*idx)++] = LINE_DIFF_CONTEXT;
		}
		if (rc)
			goto end;
		li += ntotal;
		ri += ntotal;

		/* Show the differences */
		for (i = 0; i < nc; i++) {
			unsigned char *alignment;
			nleft = c[ci + i * 3 + 1];	/* Lines on left */
			nright = c[ci + i * 3 + 2];	/* Lines on right */

			/*
			 * If the gap between the current change and the next
			 * change within the same block is not too great, then
			 * render them as if they are a single change.
			 */
			while (i < nc - 1 &&
			    sbsdiff_close_gap(&c[ci + i * 3])) {
				i++;
				ntotal = c[ci + i * 3];
				nleft += c[ci + i * 3 + 1] + ntotal;
				nright += c[ci + i * 3 + 2] + ntotal;
			}

			alignment = sbsdiff_align(&l[li], nleft, &r[ri],
			    nright);
			if (!alignment) {
				rc = FSL_RC_OOM;
				goto end;
			}
			for (j = 0; !rc && nleft + nright > 0; j++, (*idx)++) {
				char tag[30] = "<span class=\"fsl-diff-";
				switch (alignment[j]) {
				case 1:
					/* Delete one line from the left */
					rc = sbsdiff_lineno(&s, li, SBS_LLINE);
					if (rc)
						goto end_align;
					s.idx = 0;
					fsl_strlcat(tag, "rm\">", sizeof(tag));
					s.tag = tag;
					s.end = LENGTH(&l[li]);
					lines[*idx] = LINE_DIFF_MINUS;
					rc = sbsdiff_txt(&s, &l[li], SBS_LTEXT);
					if (rc)
						goto end_align;
					rc = sbsdiff_marker(&s, " <", "&lt;");
					if (rc)
						goto end_align;
					rc = sbsdiff_newline(&s);
					if (rc)
						goto end_align;
					assert(nleft > 0);
					nleft--;
					li++;
					break;
				case 2:
					/* Insert one line on the right */
					if (!s.esc) {
						rc = sbsdiff_space(&s,
						    s.width + 7, SBS_LTEXT);
						if (rc)
							goto end_align;
					}
					rc = sbsdiff_marker(&s, " > ", "&gt;");
					if (rc)
						goto end_align;
					rc = sbsdiff_lineno(&s, ri, SBS_RLINE);
					if (rc)
						goto end_align;
					s.idx = 0;
					fsl_strlcat(tag, "add\">", sizeof(tag));
					s.tag = tag;
					s.end = LENGTH(&r[ri]);
					lines[*idx] = LINE_DIFF_PLUS;
					rc = sbsdiff_txt(&s, &r[ri], SBS_RTEXT);
					if (rc)
						goto end_align;
					assert(nright > 0);
					nright--;
					ri++;
					break;
				case 3:
					/* Left line changed into the right */
					rc = sbsdiff_write_change(&s, &l[li],
					    li, &r[ri], ri);
					if (rc)
						goto end_align;
					assert(nleft > 0 && nright > 0);
					nleft--;
					nright--;
					li++;
					ri++;
					break;
				default: {
					/* Delete left and insert right */
					rc = sbsdiff_lineno(&s, li, SBS_LLINE);
					if (rc)
						goto end_align;
					s.idx = 0;
					fsl_strlcat(tag, "rm\">", sizeof(tag));
					s.tag = tag;
					s.end = LENGTH(&l[li]);
					rc = sbsdiff_txt(&s, &l[li], SBS_LTEXT);
					if (rc)
						goto end_align;
					lines[*idx] = LINE_DIFF_EDIT;
					rc = sbsdiff_marker(&s, " | ", "|");
					if (rc)
						goto end_align;
					rc = sbsdiff_lineno(&s, ri, SBS_RLINE);
					if (rc)
						goto end_align;
					s.idx = 0;
					s.tag = "<span class=\"fsl-diff-add\">";
					s.end = LENGTH(&r[ri]);
					rc = sbsdiff_txt(&s, &r[ri], SBS_RTEXT);
					if (rc)
						goto end_align;
					nleft--;
					nright--;
					li++;
					ri++;
					break;
				}
				}
			}
end_align:
			fsl_free(alignment);
			if (rc)
				goto end;
			if (i < nc - 1) {
				ntotal = c[ci + i * 3 + 3];
				for (j = 0; !rc && j < ntotal; j++, (*idx)++) {
					rc = sbsdiff_lineno(&s, li + j,
					    SBS_LLINE);
					s.idx = s.end = -1;
					if (rc)
						goto end;
					lines[*idx] = LINE_DIFF_CONTEXT;
					rc = sbsdiff_txt(&s, &l[li + j],
					    SBS_LTEXT);
					if (rc)
						goto end;
					rc = sbsdiff_marker(&s, "   ", "");
					if (rc)
						goto end;
					rc = sbsdiff_lineno(&s, ri + j,
					    SBS_RLINE);
					if (rc)
						goto end;
					lines[*idx] = LINE_DIFF_CONTEXT;
					rc = sbsdiff_txt(&s, &r[ri + j],
					    SBS_RTEXT);
					if (rc)
						goto end;
				}
				ri += ntotal;
				li += ntotal;
			}
		}

		/* Show the final common area */
		assert(nc == i);
		ntotal = c[ci + nc * 3];
		if (ntotal > context)
			ntotal = context;
		for (j = 0; !rc && j < ntotal; j++, (*idx)++) {
			rc = sbsdiff_lineno(&s, li + j, SBS_LLINE);
			s.idx = s.end = -1;
			lines[*idx] = LINE_DIFF_CONTEXT;
			if (!rc)
				rc = sbsdiff_txt(&s, &l[li + j], SBS_LTEXT);
			if (!rc)
				rc = sbsdiff_marker(&s, "   ", "");
			if (!rc)
				rc = sbsdiff_lineno(&s, ri + j, SBS_RLINE);
			lines[*idx] = LINE_DIFF_CONTEXT;
			if (!rc)
				rc = sbsdiff_txt(&s, &r[ri + j], SBS_RTEXT);
			if (rc)
				goto end;
		}
	}  /* diff triplet loop */

	assert(!rc);

	if (s.esc && (s.cols[SBS_LLINE]->used > 0)) {
		rc = diff_out(dst, "<table class=\"fsl-sbsdiff-cols\"><tr>\n",
		    -1);
		for (i = SBS_LLINE; !rc && i <= SBS_RTEXT; i++)
			rc = sbsdiff_column(dst, s.cols[i], i);
		if (!rc)
			rc = diff_out(dst, "</tr></table>\n", -1);
	}
end:
	for (i = 0; i < (int)nitems(sbscols); ++i)
		fsl_buffer_clear(&sbscols[i]);
	return rc;
}

/*
 * Render the diff triples array in cx->aEdit as a unified diff in out.
 *   cx		Raw diff data
 *   out	Side-by-side diff representation
 *   regex	Show changes matching this regex
 *   context	Number of context lines
 *   flags	Flags controlling the diff
 */
int
unidiff(fsl__diff_cx *cx, struct diff_out_state *dst, void *regex,
    uint16_t context, uint64_t flags)
{
	fsl_dline	*l, *r;		/* Left and right side of diff */
	static int	 chunks = 0;	/* Number of chunks so far processed */
	int		 li, ri;	/* Index of next line in l[] and r[] */
	int		*c;		/* copy/delete/insert triples */
	int		 ci;		/* Index into c[] */
	int		 nc;		/* number of c[] triples to process */
	int		 max_ci;	/* Maximum value for ci */
	int		 nleft, nright; /* Number of l and r lines to output */
	int		 ntotal;	/* Total number of lines to output */
	int		 skip;		/* Number of lines to skip */
	int		 i, j, rc = FSL_RC_OK;
	bool		 html, proto, showln, showsep = false;

	proto = FLAG_CHK(flags, FNC_DIFF_PROTOTYPE);
	showln = FLAG_CHK(flags, FNC_DIFF_LINENO);
	html = FLAG_CHK(flags, FNC_DIFF_HTML);

	l = cx->aFrom;
	r = cx->aTo;
	c = cx->aEdit;
	max_ci = cx->nEdit;
	li = ri = 0;

	while (max_ci > 2 && c[max_ci - 1] == 0 && c[max_ci - 2] == 0)
		max_ci -= 3;

	for (ci = 0; ci < max_ci; ci += 3 * nc) {
		/* Figure out how many triples to show in a single block. */
		for (nc = 1; c[ci + nc * 3] > 0 &&
		    c[ci + nc * 3] < context * 2; nc++) {}
		/* printf("ci=%d nc=%d\n", ci, nc); */

#if 0
		/*
		 * XXX Missing: re/predicate bits.
		 * If there is a regex, skip this block (i.e., generate no diff
		 * output) if the regex matches or does not match both insert
		 * and delete. Only display the block if one side matches but
		 * the other side does not.
		 */
		if (regex) {
			bool hidechange = true;
			int xa = li, xb = ri;
			for (i = 0; hidechange && i < nc; i++) {
				int c1, c2;
				xa += c[ci + i * 3];
				xb += c[ci + i * 3];
				c1 = re_dline_match(regex, &l[xa],
				    c[ci + i * 3 + 1]);
				c2 = re_dline_match(regex, &r[xb],
				    c[ci + i * 3 + 2]);
				hidechange = c1 == c2;
				xa += c[ci + i * 3 + 1];
				xb += c[ci + i * 3 + 2];
			}
			if( hidechange ) {
				li = xa;
				ri = xb;
				continue;
			}
		}
#endif
		/*
		 * For the current block comprising nc triples, figure out
		 * how many lines of l and r are to be displayed.
		 */
		if (c[ci] > context) {
			nleft = nright = context;
			skip = c[ci] - context;
		} else {
			nleft = nright = c[ci];
			skip = 0;
		}
		for (i = 0; i < nc; i++) {
			nleft += c[ci + i * 3 + 1];
			nright += c[ci + i * 3 + 2];
		}
		if (c[ci + nc * 3] > context) {
			nleft += context;
			nright += context;
		} else {
			nleft += c[ci + nc * 3];
			nright += c[ci + nc * 3];
		}
		for (i = 1; i < nc; i++) {
			nleft += c[ci + i * 3];
			nright += c[ci + i * 3];
		}

		/*
		 * Show the header for this block, or if we are doing a modified
		 * unified diff that contains line numbers, show the separator
		 * from the previous block.
		 */
		++chunks;
		if (showln) {
			if (!showsep)
				showsep = 1;  /* Don't show a top divider */
			else if (html)
				rc = diff_outf(dst,
				    "<span class=\"fsl-diff-hr\">%.*c</span>\n",
				    80, '.');
			else
				rc = diff_outf(dst, "%.95c\n", '.');
			if (!rc && html)
				rc = diff_outf(dst,
				    "<span class=\"fsl-diff-chunk-%d\"></span>",
				    chunks);
		} else {
			char const *ansi1 = "";
			char const *ansi2 = "";
			char const *ansi3 = "";
			if (html)
				rc = diff_outf(dst,
				    "<span class=\"fsl-diff-lineno\">");
#if 0
			/* Turns out this just confuses the output */
			else if (dst->ansi) {
				ansi1 = ANSI_DIFF_RM(0);
				ansi2 = ANSI_DIFF_ADD(0);
				ansi3 = ANSI_RESET;
			}
#endif
			/*
			 * If the patch changes an empty file or results in
			 * an empty file, the block header must use 0,0 as
			 * position indicator and not 1,0. Otherwise, patch
			 * would be confused and may reject the diff.
			 */
			if (!rc)
				rc = diff_outf(dst,"@@ %s-%d,%d %s+%d,%d%s @@",
				    ansi1, nleft ? li+skip+1 : 0, nleft, ansi2,
				    nright ? ri+skip+1 : 0, nright, ansi3);
			if (!rc) {
				if (html)
					rc = diff_outf(dst, "</span>");

				if (proto && li + skip > 1) {
					char *f = match_chunk_function(dst,
					    (li + skip) - 1);
					if (f != NULL)
						rc = diff_outf(dst, " %s", f);
				}
				if (!rc)
					rc = diff_out(dst, "\n", 1);
			}
		}
		if (rc)
			return rc;

		/* Show the initial common area */
		li += skip;
		ri += skip;
		ntotal = c[ci] - skip;
		for (j = 0; !rc && j < ntotal; j++) {
			if (showln)
				rc = unidiff_lineno(dst,
				    li + j + 1, ri + j + 1, html);
			if (!rc)
				rc = unidiff_txt(dst, ' ', &l[li + j], html, 0);
		}
		if (rc)
			return rc;
		li += ntotal;
		ri += ntotal;

		/* Show the differences */
		for (i = 0; i < nc; i++) {
			ntotal = c[ci + i * 3 + 1];
			for (j = 0; !rc && j < ntotal; j++) {
				if (showln)
					rc = unidiff_lineno(dst, li + j + 1, 0,
					    html);
				if (!rc)
					rc = unidiff_txt(dst, '-', &l[li + j],
					    html, regex);
			}
			if (rc)
				return rc;
			li += ntotal;
			ntotal = c[ci + i * 3 + 2];
			for (j = 0; !rc && j < ntotal; j++) {
				if (showln)
					rc = unidiff_lineno(dst, 0, ri + j + 1,
					    html);
				if (!rc)
					rc = unidiff_txt(dst, '+', &r[ri + j],
					    html, regex);
			}
			if (rc)
				return rc;
			ri += ntotal;
			if (i < nc - 1) {
				ntotal = c[ci + i * 3 + 3];
				for (j = 0; !rc && j < ntotal; j++) {
					if (showln)
						rc = unidiff_lineno(dst,
						    li + j + 1, ri + j + 1,
						    html);
					if (!rc)
						rc = unidiff_txt(dst, ' ',
						    &l[li + j], html, 0);
				}
				if (rc)
					return rc;
				ri += ntotal;
				li += ntotal;
			}
		}

		/* Show the final common area */
		assert(nc==i);
		ntotal = c[ci + nc * 3];
		if (ntotal > context)
			ntotal = context;
		for (j = 0; !rc && j < ntotal; j++) {
			if (showln)
				rc = unidiff_lineno(dst, li + j + 1, ri + j + 1,
				    html);
			if (!rc)
				rc = unidiff_txt(dst, ' ', &l[li + j], html, 0);
		}
	}  /* _big_ for() loop */
	fsl_free(dst->proto.signature);
	return rc;
}

/* Extract the number of context lines from flags. */
int
diff_context_lines(uint64_t flags)
{
	int n = flags & FNC_DIFF_CONTEXT_MASK;

	if (!n && !FLAG_CHK(flags, FNC_DIFF_CONTEXT_EX))
		n = 5;

	return n;
}

/*
 * Extract column width for side-by-side diff from flags. Return appropriate
 * default if no width is specified.
 */
int
sbsdiff_width(uint64_t flags)
{
	int w = (flags & FNC_DIFF_WIDTH_MASK - 0xf) / FNC_DIFF_CONTEXT_MASK;

	if (!w)
		w = 80;

	return w;
}

/* Append a separator line of length len to column col. */
int
sbsdiff_separator(struct sbsline *dst, int len, int col)
{
	char ch = '.';

	if (len < 1) {
		len = 1;
		ch = ' ';
	}

	return fsl_buffer_appendf(dst->cols[col],
	    "<span class=\"fsl-diff-hr\">%.*c</span>\n", len, ch);
}

/*
 * fsl_output_f() implementation for use with diff_outf(). State must be a
 * struct diff_out_state *.
 */
int
fsl_output_f_diff_out(void *state, void const *src, fsl_size_t n)
{
	struct diff_out_state *const dst = (struct diff_out_state *)state;

	return dst->rc = dst->out(dst->state, src, n);
}

int
diff_outf(struct diff_out_state *dst, char const *fmt, ...)
{
	va_list va;

	va_start(va,fmt);
	fsl_appendfv(fsl_output_f_diff_out, dst, fmt, va);
	va_end(va);

	return dst->rc;
}

/* Append a column to the final output blob. */
int
sbsdiff_column(struct diff_out_state *dst, fsl_buffer const *content, int col)
{
	return diff_outf(dst,
	    "<td><div class=\"fsl-diff-%s-col\">\n"
	    "<pre>\n"
	    "%b</pre>\n"
	    "</div></td>\n",
	    col % 3 ? (col == SBS_MID ? "separator" : "text") : "lineno",
	    content);
}

/*
 * Write the text of dline into column col of SBS diff dst. If outputting HTML,
 * write the full line; otherwise, only write up to dst->width characters.
 * Expand tabs to spaces, and add newlines if col is SBS_RTEXT. Translate HTML
 * characters if esc is true. Pad with spaces to dst->width bytes if col is
 * SBS_LTEXT and esc is false.
 *
 * This comment contains multibyte unicode characters (�, �, �) in order
 * to test the ability of the diff code to handle such characters.
 */
int
sbsdiff_txt(struct sbsline *dst, fsl_dline *dline, int col)
{
	fsl_buffer	*o = dst->cols[col];
	const char	*str = dline->z;
	int		 n = dline->n;
	int		 i;	/* Number of input characters consumed */
	int		 pos;	/* Cursor position */
	int		 w = dst->width;
	int		 rc = FSL_RC_OK;
	bool		 colourise = dst->esc;
	bool		 endspan = false;
#if 0
	/*
	 * XXX Missing regex bits, but want to replace those with a predicate.
	 */
	if (colourise && dst->regex && !re_dline_match(dst->regex, dline, 1))
		colourise = false;
#endif
	for (i = pos = 0; !rc && (dst->esc || pos < w) && i < n; i++, pos++) {
		char c = str[i];
		if (colourise) {
			if (i == dst->idx) {
				rc = fsl_buffer_append(o, dst->tag, -1);
				if (rc)
					break;
				endspan = true;
				if (dst->idx2) {
					dst->idx = dst->idx2;
					dst->tag = dst->tag2;
					dst->idx2 = 0;
				}
			} else if (i == dst->end) {
				rc = fsl_buffer_append(o, "</span>", 7);
				if (rc)
					break;
				endspan = false;
				if (dst->end2) {
					dst->end = dst->end2;
					dst->end2 = 0;
				}
			}
		}
		if (c == '\t' && !dst->esc) {
			rc = fsl_buffer_append(o, " ", 1);
			while (!rc && (pos & 7) != 7 && (dst->esc || pos < w)) {
				rc = fsl_buffer_append(o, " ", 1);
				++pos;
			}
		} else if (c == '\r' || c == '\f')
			rc = fsl_buffer_append(o, " ", 1);
		else if (c == '<' && dst->esc)
			rc = fsl_buffer_append(o, "&lt;", 4);
		else if (c == '&' && dst->esc)
			rc = fsl_buffer_append(o, "&amp;", 5);
		else if (c == '>' && dst->esc)
			rc = fsl_buffer_append(o, "&gt;", 4);
		else if (c == '"' && dst->esc)
			rc = fsl_buffer_append(o, "&quot;", 6);
		else {
			rc = fsl_buffer_append(o, &str[i], 1);
			if (UTF_CONT(c))
				--pos;
		}
	}
	if (!rc && endspan)
		rc = fsl_buffer_append(o, "</span>", 7);
	if (!rc) {
		if (col == SBS_RTEXT)
			rc = sbsdiff_newline(dst);
		else if (!dst->esc)
			rc = sbsdiff_space(dst, w - pos, SBS_LTEXT);
	}

	return rc;
}

/*
 * Append newlines to columns corresponding to sbs diff format.
 *   html: all columns
 *   text: right column only
 */
int
sbsdiff_newline(struct sbsline *dst)
{
	int i, rc = FSL_RC_OK;

	for (i = dst->esc ? SBS_LLINE : SBS_RTEXT; !rc && i <= SBS_RTEXT; i++)
		rc = fsl_buffer_append(dst->cols[i], "\n", 1);

	return rc;
}

/* Append n spaces to column col in the sbs diff. */
int
sbsdiff_space(struct sbsline *dst, int n, int col)
{
	return fsl_buffer_appendf(dst->cols[col], "%*s", n, "");
}

/* Append plaintext XOR html marker into the center column of the sbs diff. */
int
sbsdiff_marker(struct sbsline *dst, const char *str, const char *html)
{
	return fsl_buffer_append(dst->cols[SBS_MID], dst->esc ? html : str, -1);
}

/* Append file line number ln to column col in the sbs diff. */
int
sbsdiff_lineno(struct sbsline *dst, int ln, int col)
{
	int rc;

	if (dst->esc)
		rc = fsl_buffer_appendf(dst->cols[col], "%d", ln + 1);
	else {
		char lnno[8];
		fsl_snprintf(lnno, 8, "%5d ", ln + 1);
		rc = fsl_buffer_appendf(dst->cols[col], "%s ", lnno);
	}

	return rc;
}

/* Try to shift dst->idx as far as possible to the left. */
void
sbsdiff_shift_left(struct sbsline *dst, const char *z)
{
	int i, j;

	while ((i = dst->idx) > 0 && z[i - 1] == z[i]) {
		for (j = i + 1; j < dst->end && z[j - 1] == z[j]; j++) {}
		if (j < dst->end)
			break;
		--dst->idx;
		--dst->end;
	}
}

/*
 * Simplify line at idx and idx2 in SBS diff output:
 *    -  If idx is a null-change then move idx2 into idx
 *    -  Make sure any null-changes are in canonical form.
 *    -  Make sure all changes are at character boundaries for multibyte chars.
 */
void
sbsdiff_simplify_line(struct sbsline *dst, const char *z)
{
	if (dst->idx2 == dst->end2)
		dst->idx2 = dst->end2 = 0;
	else if (dst->idx2) {
		while (dst->idx2 > 0 && UTF_CONT(z[dst->idx2]))
			--dst->idx2;
		while (UTF_CONT(z[dst->end2]))
			++dst->end2;
	}

	if (dst->idx == dst->end) {
		dst->idx = dst->idx2;
		dst->end = dst->end2;
		dst->tag = dst->tag2;
		dst->idx2 = 0;
		dst->end2 = 0;
	}

	if (dst->idx == dst->end)
		dst->idx = dst->end = -1;
	else if (dst->idx > 0) {
		while (dst->idx > 0 && UTF_CONT(z[dst->idx]))
			--dst->idx;
		while (UTF_CONT(z[dst->end]))
			++dst->end;
	}
}

/*
 * c[] is an array of six integers: two copy/delete/insert triples for a
 * pair of adjacent differences. Return true if the gap between these two
 * differences is so small that they should be rendered as a single edit.
 */
int
sbsdiff_close_gap(int *c)
{
	return c[3] <= 2 || c[3] <= (c[1] + c[2] + c[4] + c[5]) / 8;
}

/*
 * There is a change block in which nleft lines of text on the left are
 * converted into nright lines of text on the right. This routine computes how
 * the lines on the left line up with the lines on the right.
 *
 * The return value is a buffer of unsigned characters, obtained from
 * fsl_malloc(), which needs to be disposed of by the caller. Entries in the
 * returned array have values as follows:
 *    1 = Delete the next line of left.
 *    2 = Insert the next line of right.
 *    3 = The next line of left changes into the next line of right.
 *    4 = Delete one line from left and add one line to right.
 *
 * Values larger than three indicate better matches.
 *
 * The length of the returned array will be just large enough to cause all
 * elements of left and right to be consumed.
 *
 * Algorithm: Wagner's minimum edit-distance algorithm, modified by adding a
 * cost to each match based on how well the two rows match each other.
 * Insertion and deletion costs are 50. Match costs are between 0 and 100 where
 * 0 is a perfect match 100 is a complete mismatch.
 *   left	lines of text on the left
 *   nleft	number of lines on the left
 *   right	lines of text on the right
 *   nright	number of lines on the right
 */
unsigned char
*sbsdiff_align(fsl_dline *left, int nleft, fsl_dline *right, int nright)
{
	int		 buf[100];	/* left[] stack if nright not too big */
	int		*row;		/* One row of the Wagner matrix */
	int		*ptr;		/* Space that needs to be freed */
	int		 nmatches;	/* Number of matches */
	int		 matchscore;	/* Match score */
	int		 minlen;	/* MIN(nleft, nright) */
	int		 maxlen;	/* MAX(nleft, nright) */
	int		 i, j, k;	/* Loop counters */
	unsigned char	*matrix;	/* Wagner result matrix */

	matrix = (unsigned char *)fsl_malloc((nleft + 1) * (nright + 1));

	if (!matrix)
		return NULL;
	if (!nleft) {
		memset(matrix, 2, nright);
		return matrix;
	}
	if (!nright) {
		memset(matrix, 1, nleft);
		return matrix;
	}

	/*
	 * This algorithm is O(n^2).  So if n is too big, bail out with a
	 * simple (but stupid and ugly) result that doesn't take too long.
	 */
	minlen = min(nleft, nright);
	if (nleft * nright > 100000) {
		memset(matrix, 4, minlen);
		if (nleft > minlen)
			memset(matrix + minlen, 1, nleft - minlen);
		if (nright > minlen)
			memset(matrix + minlen, 2, nright - minlen);
		return matrix;
	}

	if (nright < (int)nitems(buf) - 1) {
		ptr = 0;
		row = buf;
	} else {
		row = ptr = fsl_malloc(sizeof(row[0]) * (nright + 1));
		if (!row) {
			fsl_free(matrix);
			return NULL;
		}
	}

	/* Compute the best alignment */
	for (i = 0; i <= nright; i++) {
		matrix[i] = 2;
		row[i] = i * 50;
	}
	matrix[0] = 0;
	for (j = 1; j <= nleft; j++) {
		int p = row[0];
		row[0] = p + 50;
		matrix[j * (nright + 1)] = 1;
		for (i = 1; i <= nright; i++) {
			int nlines = row[i - 1] + 50;
			int d = 2;
			if (nlines > row[i] + 50) {
				nlines = row[i] + 50;
				d = 1;
			}
			if (nlines > p) {
				int score = match_dline(&left[j - 1],
				    &right[i - 1]);
				if ((score <= 63 || (i < j + 1 && i > j - 1))
				    && nlines > p + score) {
					nlines = p + score;
					d = 3 | score * 4;
				}
			}
			p = row[i];
			row[i] = nlines;
			matrix[j * (nright + 1) + i] = d;
		}
	}

	/* Compute the lowest-cost path back through the matrix. */
	i = nright;
	j = nleft;
	k = (nright + 1) * (nleft + 1) - 1;
	nmatches = matchscore = 0;
	while (i + j > 0) {
		unsigned char c = matrix[k];
		if (c >= 3) {
			assert(i > 0 && j > 0);
			--i;
			--j;
			++nmatches;
			matchscore += (c >> 2);
			matrix[k] = 3;
		} else if (c == 2){
			assert(i > 0);
			--i;
		} else {
			assert(j > 0);
			--j;
		}
		--k;
		matrix[k] = matrix[j * (nright + 1) + i];
	}
	++k;
	i = (nright + 1) * (nleft + 1) - k;
	memmove(matrix, &matrix[k], i);

	/*
	 * If:
	 *   1.  the alignment is more than 25% longer than the longest side; &
	 *   2.  the average match cost exceeds 15
	 * Then this is probably an alignment that will be difficult for humans
	 * to read. So instead, just show all of the right side inserted
	 * followed by all of the left side deleted.
	 *
	 * The coefficients for conditions (1) and (2) above are determined by
	 * experimentation.
	 */
	maxlen = nleft > nright ? nleft : nright;
	if (i * 4 > maxlen * 5 && (!nmatches || matchscore / nmatches > 15)) {
		memset(matrix, 4, minlen);
		if (nleft > minlen)
			memset(matrix + minlen, 1, nleft - minlen);
		if (nright > minlen)
			memset(matrix + minlen, 2, nright - minlen);
	}

	/* Return the result */
	fsl_free(ptr);
	return matrix;
}

/*
 * Write and record line type of lines that have been edited to dst. Adjust
 * highlight to cover only those parts of the line that have changed.
 *   dst	The SBS output line
 *   left	Left line of the change
 *   llnno	Line number for the left line
 *   right	Right line of the change
 *   rlnno	Line number of the right line
 */
int
sbsdiff_write_change(struct sbsline *dst, fsl_dline *left, int llnno,
    fsl_dline *right, int rlnno)
{
	static const char	 tag_rm[]   = "<span class=\"fsl-diff-rm\">";
	static const char	 tag_add[]  = "<span class=\"fsl-diff-add\">";
	static const char	 tag_chg[] = "<span class=\"fsl-diff-change\">";
	const char		*ltxt;	/* Text of the left line */
	const char		*rtxt;	/* Text of the right line */
	enum line_type		*lines = dst->output->lines;
	uint32_t		*idx = dst->output->idx;
	int	lcs[4] = {0, 0, 0, 0};	/* Bounds of common middle segment */
	int	leftsz;		/* Length of left line in bytes */
	int	rightsz;	/* Length of right line in bytes */
	int	shortest;	/* Shortest of left and right */
	int	npfx;		/* Length of common prefix */
	int	nsfx;		/* Length of common suffix */
	int	nleft;		/* leftsz - npfx - nsfx */
	int	nright;		/* rightsz - npfx - nsfx */
	int	rc = FSL_RC_OK;

	leftsz = left->n;
	ltxt = left->z;
	rightsz = right->n;
	rtxt = right->z;
	shortest = min(leftsz, rightsz);

	/* Count common prefix. */
	npfx = 0;
	while (npfx < shortest && ltxt[npfx] == rtxt[npfx])
		npfx++;

	/* Account for multibyte chars in prefix. */
	if (npfx < shortest)
		while (npfx > 0 && UTF_CONT(ltxt[npfx]))
			npfx--;

	/* Count common suffix. */
	nsfx = 0;
	if (npfx < shortest) {
		while (nsfx < shortest &&
		    ltxt[leftsz - nsfx - 1] == rtxt[rightsz - nsfx - 1])
			++nsfx;
		/* Account for multibyte chars in suffix. */
		if (nsfx < shortest)
			while (nsfx > 0 && UTF_CONT(ltxt[leftsz - nsfx]))
				--nsfx;
		if (nsfx == leftsz || nsfx == rightsz)
			npfx = 0;
	}
	if (npfx + nsfx > shortest)
		npfx = shortest - nsfx;

	/* A single chunk of text inserted on the right */
	if (npfx + nsfx == leftsz) {
		rc = sbsdiff_lineno(dst, llnno, SBS_LLINE);
		if (rc)
			return rc;
		dst->idx2 = dst->end2 = 0;
		dst->idx = dst->end = -1;
		rc = sbsdiff_txt(dst, left, SBS_LTEXT);
		if (!rc && leftsz == rightsz && ltxt[leftsz] == rtxt[rightsz]) {
			rc = sbsdiff_marker(dst, "   ", "");
			lines[*idx] = LINE_DIFF_CONTEXT;
		}
		else {
			rc = sbsdiff_marker(dst, " | ", "|");
			lines[*idx] = LINE_DIFF_EDIT;
		}

		if (!rc) {
			rc = sbsdiff_lineno(dst, rlnno, SBS_RLINE);
			if (!rc) {
				dst->idx = npfx;
				dst->end = rightsz - nsfx;
				dst->tag = tag_add;
				rc = sbsdiff_txt(dst, right, SBS_RTEXT);
			}
		}
		return rc;
	}

	/* A single chunk of text deleted from the left */
	if (npfx + nsfx == rightsz) {
		lines[*idx] = LINE_DIFF_EDIT;
		/* Text deleted from the left */
		rc = sbsdiff_lineno(dst, llnno, SBS_LLINE);
		if (rc)
			return rc;
		dst->idx2 = dst->end2 = 0;
		dst->idx = npfx;
		dst->end = leftsz - nsfx;
		dst->tag = tag_rm;
		rc = sbsdiff_txt(dst, left, SBS_LTEXT);
		if (!rc) {
			rc = sbsdiff_marker(dst, " | ", "|");
			if (!rc) {
				rc = sbsdiff_lineno(dst, rlnno, SBS_RLINE);
				if (!rc) {
					dst->idx = dst->end = -1;
					sbsdiff_txt(dst, right, SBS_RTEXT);
				}
			}
		}
		return rc;
	}

	/*
	 * At this point we know that there is a chunk of text that has
	 * changed between the left and the right. Check to see if there
	 * is a large unchanged section in the middle of that changed block.
	 */
	nleft = leftsz - nsfx - npfx;
	nright = rightsz - nsfx - npfx;
	if (dst->esc && nleft >= 6 && nright >= 6 &&
	    find_lcs(&ltxt[npfx], nleft, &rtxt[npfx], nright, lcs)) {
		rc = sbsdiff_lineno(dst, llnno, SBS_LLINE);
		if (rc)
			return rc;
		dst->idx = npfx;
		dst->end = npfx + lcs[0];
		if (lcs[2] == 0) {
			sbsdiff_shift_left(dst, left->z);
			dst->tag = tag_rm;
		} else
			dst->tag = tag_chg;
		dst->idx2 = npfx + lcs[1];
		dst->end2 = leftsz - nsfx;
		dst->tag2 = lcs[3] == nright ? tag_rm : tag_chg;
		sbsdiff_simplify_line(dst, ltxt + npfx);
		lines[*idx] = LINE_DIFF_EDIT;
		rc = sbsdiff_txt(dst, left, SBS_LTEXT);
		if (!rc)
			rc = sbsdiff_marker(dst, " | ", "|");
		if (!rc)
			rc = sbsdiff_lineno(dst, rlnno, SBS_RLINE);
		if (rc)
			return rc;
		dst->idx = npfx;
		dst->end = npfx + lcs[2];
		if (!lcs[0]) {
			sbsdiff_shift_left(dst, right->z);
			dst->tag = tag_add;
		} else
			dst->tag = tag_chg;
		dst->idx2 = npfx + lcs[3];
		dst->end2 = rightsz - nsfx;
		dst->tag2 = lcs[1]==nleft ? tag_add : tag_chg;
		sbsdiff_simplify_line(dst, rtxt + npfx);
		rc = sbsdiff_txt(dst, right, SBS_RTEXT);
		return rc;
	}

	/* If all else fails, show a single big change between left and right */
	rc = sbsdiff_lineno(dst, llnno, SBS_LLINE);
	if (!rc) {
		lines[*idx] = LINE_DIFF_EDIT;
		dst->idx2 = dst->end2 = 0;
		dst->idx = npfx;
		dst->end = leftsz - nsfx;
		dst->tag = tag_chg;
		rc = sbsdiff_txt(dst, left, SBS_LTEXT);
		if (!rc) {
			rc = sbsdiff_marker(dst, " | ", "|");
			if (!rc) {
				rc = sbsdiff_lineno(dst, rlnno, SBS_RLINE);
				if (!rc) {
					dst->end = rightsz - nsfx;
					sbsdiff_txt(dst, right, SBS_RTEXT);
				}
			}
		}
	}
	return rc;
}

/*
 * Add two line numbers to the beginning of a unified diff output line.
 *   dst	Output destination
 *   lln	Line number corresponding to the line in the left (old) file
 *   rln	Line number corresponding to the line in the right (new) file
 *   html	Specify html formatted output
 * n.b. lln or rln can be zero to leave that number field blank.
 */
int
unidiff_lineno(struct diff_out_state *dst, int lln, int rln, bool html)
{
	int rc = FSL_RC_OK;

	if (html) {
		rc = diff_out(dst, "<span class=\"fsl-diff-lineno\">", -1);
		if (rc)
			return rc;
	}

	if (lln > 0)
		rc = diff_outf(dst, "%6d ", lln);
	else
		rc = diff_out(dst, "       ", 7);

	if (!rc) {
		if (rln > 0)
			rc = diff_outf(dst, "%6d  ", rln);
		else
			rc = diff_out(dst, "        ", 8);
		if (!rc && html)
			rc = diff_out(dst, "</span>", -1);
	}

	return rc;
}

/*
 * Append a single line of unified diff text to dst.
 *   dst	Destination
 *   sign	Either a " " (context), "+" (added),  or "-" (removed) line
 *   line	The line to be output
 *   html	True if generating HTML, false for plain text
 *   regex	colourise only if line matches this regex
 */
int
unidiff_txt(struct diff_out_state *const dst, char sign, fsl_dline *line,
    int html, void *regex)
{
	char const	*ansiccode;
	int		 rc = FSL_RC_OK;

	ansiccode = !dst->ansi ? NULL : ((sign == '+') ?
	    ANSI_DIFF_ADD(0) : ((sign == '-') ? ANSI_DIFF_RM(0) : NULL));

	if (ansiccode)
		rc = diff_out(dst, ansiccode, -1);
	if (!rc)
		rc = diff_out(dst, &sign, 1);
	if (rc)
		return rc;

	if (html) {
#if 0
		/* XXX Missing regex implementation. */
		if (regex && !re_dline_match(regex, line, 1))
			sign = ' ';
		else
#endif
		/*
		 * XXX Shift below block left one tab while regex is if'd out.
		 * ----8<-----------------------------------------------------
		 */
		if (sign == '+')
			rc = diff_out(dst, "<span class=\"fsl-diff-add\">", -1);
		else if (sign == '-')
			rc = diff_out(dst, "<span class=\"fsl-diff-rm\">", -1);

		if (!rc) {
			/* Trim trailing newline */
			/* unsigned short n = line->n; */
			/* while (n > 0 && (line->z[n - 1] == '\n' || */
			/*     line->z[n - 1] == '\r')) */
			/*	--n; */
			rc = dst->rc = fsl_htmlize(dst->out, dst->state,
			    line->z, line->n);
			if (!rc && sign != ' ')
				rc = diff_out(dst, "</span>", -1);
		}
		/*
		 * XXX Shift above block left one tab while regex is if'd out.
		 * ----------------------------------------------------->8----
		 */
	} else
		rc = diff_out(dst, line->z, line->n);

	if (!rc) {
		if (ansiccode)
			rc = diff_out(dst, ANSI_RESET, -1);
		if (!rc)
			rc = diff_out(dst, "\n", 1);
	}

	return rc;
}
