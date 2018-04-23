/* Provided by Haruhiko Okumura     1989-06-04 */
/* Improved by TonyJiang@lenovo.com 2014-09-28 */
/* Follow the author's principle,
'Use, distribute, and modify this program freely.' */

#include <stddef.h>

#if 1

#define N      2048
#define F        33
#define THRESHOLD 1
#define NIL N

#define GETC(c, p) { c = *p; p++; }
#define PUTC(p, c) { *p = c; p++; }

typedef struct _Lzss_Tree {
	int lson[N + 1];
	int rson[N + 257];
	int dad[N + 1];
	// ----- match parameters ----- //
	int pos;
	int len;
} Lzss_Tree, *PLZT;

void InitTree(PLZT pt)
{
	int i = N + 1;
	for (; i <= N + 256; i++) pt->rson[i] = NIL;
	for (i = 0; i < N; i++) pt->dad[i] = NIL;
}

void InsertNode(PLZT pt, unsigned char* text_buf, int r)
{
	unsigned char *key = text_buf + r;
	int i, p = N + 1 + key[0], cmp = 1;

	pt->rson[r] = pt->lson[r] = NIL; pt->len = 0;
	for (;;) {
		if (cmp >= 0) {
			if (pt->rson[p] != NIL) p = pt->rson[p];
			else { pt->rson[p] = r;  pt->dad[r] = p; return; }
		}
		else {
			if (pt->lson[p] != NIL) p = pt->lson[p];
			else { pt->lson[p] = r; pt->dad[r] = p; return; }
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0) break;
		if (i > pt->len) {
			pt->pos = p;
			if ((pt->len = i) >= F) break;
		}
	}
	pt->dad[r] = pt->dad[p]; pt->lson[r] = pt->lson[p]; pt->rson[r] = pt->rson[p];
	pt->dad[pt->lson[p]] = r; pt->dad[pt->rson[p]] = r;
	if (pt->rson[pt->dad[p]] == p) pt->rson[pt->dad[p]] = r;
	else pt->lson[pt->dad[p]] = r;
	pt->dad[p] = NIL;
}

void DeleteNode(PLZT pt, int p)
{
	int q;
	if (pt->dad[p] == NIL) return;  /* not in tree */
	if (pt->rson[p] == NIL) q = pt->lson[p];
	else if (pt->lson[p] == NIL) q = pt->rson[p];
	else {
		q = pt->lson[p];
		if (pt->rson[q] != NIL) {
			do { q = pt->rson[q]; } while (pt->rson[q] != NIL);
			pt->rson[pt->dad[q]] = pt->lson[q];
			pt->dad[pt->lson[q]] = pt->dad[q];
			pt->lson[q] = pt->lson[p];
			pt->dad[pt->lson[p]] = q;
		}
		pt->rson[q] = pt->rson[p]; pt->dad[pt->rson[p]] = q;
	}
	pt->dad[q] = pt->dad[p];
	if (pt->rson[pt->dad[p]] == p) pt->rson[pt->dad[p]] = q;
	else pt->lson[pt->dad[p]] = q;
	pt->dad[p] = NIL;
}

size_t lzss_encode(unsigned char* input, size_t inlen, unsigned char* output, size_t outlen)
{
	int i = 0, c, len = 0, r = N - F, s = 0, match_length, code_ptr = 1;
	unsigned char text_buf[N + F - 1], code_buf[17], mask = 1;
	unsigned char *pi = input, *po = output;

	Lzss_Tree lzt;
	InitTree(&lzt);
	code_buf[0] = 0;
	for (; i < r; i++) text_buf[i] = '\0';
	for (; len < F && pi - input < inlen; len++) {
		GETC(c, pi);
		text_buf[r + len] = c;
	}
	if (len == 0) return 0;
	for (i = 1; i <= F; i++) InsertNode(&lzt, text_buf, r - i);
	InsertNode(&lzt, text_buf, r);
	do {
		if (lzt.len > len) lzt.len = len;
		if (lzt.len <= THRESHOLD) {
			lzt.len = 1;
			code_buf[0] |= mask;
			code_buf[code_ptr++] = text_buf[r];
		}
		else {
			code_buf[code_ptr++] = (unsigned char)lzt.pos;
			code_buf[code_ptr++] = (unsigned char)(((lzt.pos >> 3) & 0xe0) | (lzt.len - (THRESHOLD + 1)));
		}
		if ((mask <<= 1) == 0) {
			for (i = 0; i < code_ptr && po - output < outlen; i++)
				PUTC(po, code_buf[i]);
			code_buf[0] = 0; code_ptr = mask = 1;
		}
		match_length = lzt.len;
		for (i = 0; i < match_length && pi - input < inlen; i++) {
			GETC(c, pi);
			DeleteNode(&lzt, s);
			text_buf[s] = c;
			if (s < F - 1) text_buf[s + N] = c;
			s = (s + 1) & (N - 1); r = (r + 1) & (N - 1);
			InsertNode(&lzt, text_buf, r);
		}
		while (i++ < match_length) {
			DeleteNode(&lzt, s);
			s = (s + 1) & (N - 1); r = (r + 1) & (N - 1);
			if (--len) InsertNode(&lzt, text_buf, r);
		}
	} while (len > 0);

	if (code_ptr > 1)
		for (i = 0; i < code_ptr && po - output < outlen; i++)
			PUTC(po, code_buf[i]);
	return po - output;
}

size_t lzss_decode(unsigned char* input, size_t inlen, unsigned char* output, size_t outlen)
{
	int i = 0, j, k, r = N - F, c;
	unsigned int flags = 0;
	unsigned char text_buf[N + F - 1], *pi = input, *po = output;

	for (; i < r; i++) text_buf[i] = '\0';
	for (; pi - input < inlen && po - output < outlen;) {
		if (((flags >>= 1) & 256) == 0) {
			GETC(c, pi);
			flags = c | 0xff00;
			if (pi - input >= inlen) break;
		}
		if (flags & 1) {
			GETC(c, pi); PUTC(po, c);
			text_buf[r++] = c;
			r &= (N - 1);
		}
		else {
			GETC(i, pi); if (pi - input >= inlen) break;
			GETC(j, pi);
			i |= ((j & 0xe0) << 3);
			j = (j & 0x1f) + THRESHOLD;
			if (po - output + j + 1 > outlen) break;
			for (k = 0; k <= j; k++) {
				c = text_buf[(i + k) & (N - 1)];
				PUTC(po, c);
				text_buf[r++] = c;
				r &= (N - 1);
			}
		}
	}
	return po - output;
}

#endif

