#ifndef YP_DSTRING_H
#define YP_DSTRING_H

struct yp_dstring {
	char *data;
	size_t len;
	size_t size;
};

int yp_dstrinit(struct yp_dstring *s);

void yp_dstrclear(struct yp_dstring *s);

int yp_dstraddc(struct yp_dstring *s, char c);

int yp_dstradds(struct yp_dstring *s, char *ss);

int yp_dstrcopy(struct yp_dstring *s1,
	struct yp_dstring *s2);

void yp_dstrdestroy(struct yp_dstring *s);

#endif
