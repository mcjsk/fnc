/*
 * Copyright (c) 2022 Mark Jamsek <mark@bsdbox.org>
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

#include "libfossil.h"

enum fnc_diff_flag {
	FNC_DIFF_IGNORE_EOLWS	= 0x01,
	FNC_DIFF_IGNORE_ALLWS	= 0x03,
	FNC_DIFF_SIDEBYSIDE	= 1 <<  2,
	FNC_DIFF_VERBOSE	= 1 <<  3,
	FNC_DIFF_BRIEF		= 1 <<  4,
	FNC_DIFF_HTML		= 1 <<  5,
	FNC_DIFF_LINENO		= 1 <<  6,
	FNC_DIFF_NOOPT		= 1 <<  7,  /* og. 0x0100 */
	FNC_DIFF_INVERT		= 1 <<  8,  /* og. 0x0200 */
	FNC_DIFF_NOTTOOBIG	= 1 <<  9,  /* og. 0x0800 */
	FNC_DIFF_STRIP_EOLCR	= 1 << 10,  /* og. 0x1000 */
	FNC_DIFF_ANSI_COLOR	= 1 << 11,  /* og. 0x2000 */
	FNC_DIFF_PROTOTYPE	= 1 << 12
#define FNC_DIFF_CONTEXT_EX	(((uint64_t)0x04) << 32)  /* Allow 0 context */
#define FNC_DIFF_CONTEXT_MASK	((uint64_t)0x0000ffff)    /* Default context */
#define FNC_DIFF_WIDTH_MASK	((uint64_t)0x00ff0000)    /* SBS column width */
};

struct diff_out_state {
	fsl_output_f	 out;		/* Output callback */
	void		*state;		/* State for this->out() */
	int		 rc;		/* Error reporting */
	char		 ansi;		/* ANSI colour code */
	struct {
		const fsl_buffer	*file;		/* Diffed file */
		char			*signature;	/* Matching function */
		uint32_t		 lastmatch;	/* Match line index */
		uint32_t		 lastline;	/* Last line scanned */
		fsl_size_t		 offset;	/* Match byte offset */
	} proto;
};
static const struct diff_out_state diff_out_state_empty =
    { NULL, NULL, 0, 0, { NULL, NULL, 0, 0, 0 } };

struct sbsline {
	struct diff_out_state	*output;
	fsl_buffer		*cols[5];	/* Pointers to output columns */
	const char		*tag;		/* <span> tag */
	const char		*tag2;		/* <span> tag */
	int			 idx;		/* Write tag before idx */
	int			 end;		/* Close tag before end */
	int			 idx2;		/* Write tag2 before idx2 */
	int			 end2;		/* Close tag2 before end2 */
	int			 width;		/* Max column width in diff */
	bool			 esc;		/* Escape html characters */
	void			*regex;		/* Colour matching lines */
};

int		 fnc_diff_text_raw(fsl_buffer const *, fsl_buffer const *,
		    int, int **);
int		 fnc_diff_text_to_buffer(fsl_buffer const *, fsl_buffer const *,
		    fsl_buffer *, short, short, int );
int		 fnc_diff_text(fsl_buffer const *, fsl_buffer const *,
		    fsl_output_f, void *, short, short, int );
int		 fnc_diff_blobs(fsl_buffer const *, fsl_buffer const *,
		    fsl_output_f, void *, uint16_t, short, int, int **);
int		 fnc_output_f_diff_out(void *, void const *, fsl_size_t);
int		 diff_outf(struct diff_out_state *, char const *, ... );
int		 diff_out(struct diff_out_state * const, void const *,
		    fsl_int_t);
char		*match_chunk_function(struct diff_out_state *const, uint32_t);
int		 buffer_copy_lines_from(fsl_buffer *const,
		    const fsl_buffer *const, fsl_size_t *, fsl_size_t,
		    fsl_size_t);
uint64_t	 fnc_diff_flags_convert(int);
int		 diff_context_lines(uint64_t);
int		 match_dline(fsl_dline *, fsl_dline *);
bool		 longest_common_subsequence(const char *z, int, const char *,
		    int, int *);
int		 unidiff(fsl__diff_cx *, struct diff_out_state *, void *,
		    uint16_t, uint64_t);
int		 unidiff_lineno(struct diff_out_state *, int, int, bool);
int		 unidiff_txt( struct diff_out_state *const, char, fsl_dline *,
		    int, void *);
int		 sbsdiff(fsl__diff_cx *, struct diff_out_state *, void *,
		    uint16_t, uint64_t);
int		 sbsdiff_width(uint64_t);
int		 sbsdiff_separator(struct sbsline *, int, int);
int		 sbsdiff_lineno(struct sbsline *, int, int);
void		 sbsdiff_shift_left(struct sbsline *, const char *);
void		 sbsdiff_simplify_line(struct sbsline *, const char *);
int		 sbsdiff_column(struct diff_out_state *,
		    fsl_buffer const *, int);
int		 sbsdiff_txt(struct sbsline *, fsl_dline *, int);
int		 sbsdiff_newline(struct sbsline *);
int		 sbsdiff_space(struct sbsline *, int, int);
int		 sbsdiff_marker(struct sbsline *, const char *, const char *);
int		 sbsdiff_close_gap(int *);
unsigned char	*sbsdiff_align(fsl_dline *, int, fsl_dline *, int);
int		 sbsdiff_write_change(struct sbsline *, fsl_dline *, int,
		    fsl_dline *, int);
