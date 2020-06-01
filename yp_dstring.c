#include <stdlib.h>
#include <string.h>

#include "yp_dstring.h"

int yp_dstrinit(struct yp_dstring *s)
{
	if ((s->data = malloc(1024)) == NULL)
		return (-1);

	s->data[0] = '\0';
	s->size = 1024;
	s->len = 0;

	return 0;
}

void yp_dstrclear(struct yp_dstring *s)
{
	s->data[0] = '\0';
	s->len = 0;
}

int yp_dstraddc(struct yp_dstring *s, char c)
{
	if (s->len >= s->size) {
		s->size *=2;
		if ((s->data = realloc(s->data, s->size)) == NULL)
			return (-1);
	}
	
	s->data[s->len++] = c;
	s->data[s->len] = '\0';

	return 0;
}

int yp_dstradds(struct yp_dstring *s, char *ss)
{
	size_t newlen;
	
	newlen = s->len + strlen(ss);

	if (newlen + 1 >= s->size) {
		s->size = (newlen + 1) * 2;
		if ((s->data = realloc(s->data, s->size)) == NULL)
			return (-1);
	}
	
	strcat(s->data, ss);
	s->len = newlen;

	return 0;
}

int yp_dstrcopy(struct yp_dstring *s1,
	struct yp_dstring *s2)
{
	if (s2->len >= s1->size) {
		s1->size = s2->len + 1;
		if ((s1->data = realloc(s1->data, s1->size)) == NULL)
			return (-1);
	}

	strcpy(s1->data, s2->data);
	s1->len = s2->len;

	return 0;
}

void yp_dstrdestroy(struct yp_dstring *s)
{
	free(s->data);
}
