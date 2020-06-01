#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "yp_dstring.h"
#include "yp_rpc.h"

#define XFPRINTF(...) \
do {\
	if (fprintf(__VA_ARGS__) < 0) goto error;\
} while (0);

#define yp_seterrmessage(s)				\
strncpy(errmessage, s, YP_MAX_JRERRMESSAGE)

#define yp_seterrmessagef(...) 				\
snprintf(errmessage, YP_MAX_JRERRMESSAGE, __VA_ARGS__)

static int errcode;
static char errmessage[YP_MAX_JRERRMESSAGE];
static FILE *file;

// lexical analyzer
enum YP_TOKTYPE {
	YP_T_ERROR = -1,
	YP_T_EMPTY,
	YP_T_LBRACE,
	YP_T_RBRACE,
	YP_T_COMMA,
	YP_T_COLON,
	YP_T_LBRACKET,
	YP_T_RBRACKET,
	YP_T_STRING,
	YP_T_NUMBER,
	YP_T_TRUE,
	YP_T_FALSE,
	YP_T_NULL,
	YP_T_EOF
};

char *yp_strtoktype [] = {
	"empty",
	"left brace",
	"right brace",
	"comma",
	"colon",
	"left bracket",
	"right bracket",
	"quoted string",
	"number",
	"true",
	"false",
	"null",
	"end of file"
};

struct yp_token {
	struct yp_dstring str;
	enum YP_TOKTYPE type;
};

struct yp_token curtoken;
struct yp_token nexttoken;

static int _yp_gettoken(struct yp_token *t)
{
	int c;

	yp_dstrclear(&(t->str));

	while ((c = getc(file)) == ' ' || c == '\t' || c == '\n');

	ungetc(c, file);
	
	if ((c = getc(file)) == EOF) {
		if (ferror(file))
			goto ioerror;

		t->type = YP_T_EOF;
	}
	else if (c == '{') {
		if (yp_dstraddc(&(t->str), c) < 0)
			goto allocerror;
		t->type = YP_T_LBRACE;
	}
	else if (c == '}') {
		if (yp_dstraddc(&(t->str), c) < 0)
			goto allocerror;
		t->type = YP_T_RBRACE;
	}
	else if (c == '[') {
		if (yp_dstraddc(&(t->str), c) < 0)
			goto allocerror;
		t->type = YP_T_LBRACKET;
	}
	else if (c == ']') {
		if (yp_dstraddc(&(t->str), c) < 0)
			goto allocerror;
		t->type = YP_T_RBRACKET;
	}
	else if (c == ',') {
		if (yp_dstraddc(&(t->str), c) < 0)
			goto allocerror;
		t->type = YP_T_COMMA;

	}
	else if (c == ':') {
		if (yp_dstraddc(&(t->str), c) < 0)
			goto allocerror;
		t->type = YP_T_COLON;
	}
	else if (c == '"') {
		while ((c = getc(file)) != EOF) {
			if (c == '\\') {
				if ((c = getc(file)) == EOF) {
					if (ferror(file))
						goto ioerror;
						
					yp_seterrmessage("Unexpected EOF.");
					goto parseerror;
				}

				if (c == 'b')		c = '\b';
				else if (c == 'f')	c = '\f';
				else if (c == 'n')	c = '\n';
				else if (c == 'r')	c = '\r';
				else if (c == 't')	c = '\t';
				else if (c == 'b')	c = '\b';
			}
			else if (c == '"')
				break;
		
			if (yp_dstraddc(&(t->str), c) < 0)
				goto allocerror;
		}

		if (c == EOF) {
			if (ferror(file))
				goto ioerror;
				
			yp_seterrmessage("Unexpected EOF.");
			goto parseerror;		
		}

		t->type = YP_T_STRING;
	}
	else if (c == '+' || c == '-' || isdigit(c)) {
		if (yp_dstraddc(&(t->str), c) < 0)
			goto allocerror;

		while ((c = getc(file)) != EOF) {
			if (!isdigit(c)) {
				ungetc(c, file);
				break;
			}

			if (yp_dstraddc(&(t->str), c) < 0) {
				goto allocerror;
			}
		}

		if (c == EOF) {
			if (ferror(file))
				goto ioerror;
				
			yp_seterrmessage("Unexpected EOF.");
			goto parseerror;	
		}
		
		t->type = YP_T_NUMBER;
	}
	else if (isalpha(c)) {
		do {
			if (yp_dstraddc(&(t->str), c) < 0)
				goto allocerror;
		} while ((c = getc(file)) != EOF && isalpha(c));

		if (c == EOF) {
			if (ferror(file))
				goto ioerror;
				
			yp_seterrmessage("Unexpected EOF.");
			goto parseerror;	
		}
		
		if (strcmp(t->str.data, "true") == 0)
			t->type = YP_T_TRUE;
		else if (strcmp(t->str.data, "false") == 0)
			t->type = YP_T_FALSE;
		else if (strcmp(t->str.data, "null") == 0)
			t->type = YP_T_NULL;
		else {
			yp_seterrmessagef("Unknown keyword: \"%s\".",
				t->str.data);
			goto parseerror;
		}
	}
	else {
		yp_seterrmessagef("Unknown token: \"%s\".",
			t->str.data);
		goto parseerror;
	}

	return 0;

allocerror:
	errcode = YP_ERR_JRALLOC;
	yp_seterrmessage("Cannot allocate memory.");
	return (-1);

ioerror:
	errcode = errno;
	yp_seterrmessage(strerror(errno));
	return (-1);

parseerror:
	errcode = YP_ERR_JRPARSE;
	return (-1);
}

static enum YP_TOKTYPE yp_gettoken(struct yp_token **t)
{
	// if nexttoken not EMPTY:
	if (nexttoken.type != YP_T_EMPTY) {
		// move it to curenttoken
		if (yp_dstrcopy(&(curtoken.str), &(nexttoken.str)))
			goto allocerror;
		curtoken.type = nexttoken.type;
	
		// set nexttoken to EMPTY
		nexttoken.type = YP_T_EMPTY;
	}
	else {
		// get curtoken
		if (_yp_gettoken(&curtoken) < 0)
			return YP_T_ERROR;
	}

	*t = &curtoken;

	return curtoken.type;

allocerror:
	errcode = YP_ERR_JRALLOC;
	yp_seterrmessage("Cannot allocate memory.");
	return YP_T_ERROR;
}

static enum YP_TOKTYPE yp_peektoken(struct yp_token **t)
{
	if (nexttoken.type == YP_T_EMPTY) {
		if (_yp_gettoken(&nexttoken) < 0)
			return YP_T_ERROR;
	}

	*t = &nexttoken;

	return nexttoken.type;
}

int yp_gettokentype(struct yp_token **t, enum YP_TOKTYPE ttype)
{
	if (yp_gettoken(t) < 0)
		return (-1);

	if ((*t)->type != ttype) {
		errcode = YP_ERR_JRPARSE;
		yp_seterrmessagef(
			"Wrong token: expected %s, but got %s.",
			yp_strtoktype[ttype],
			yp_strtoktype[(*t)->type]);
		return (-1);
	}
	
	return 0;
}

static void yp_startparsing(FILE *f)
{
	yp_dstrinit(&(curtoken.str));
	yp_dstrinit(&(nexttoken.str));

	curtoken.type = nexttoken.type = YP_T_EMPTY;
	file = f;
}

static void yp_endparsing()
{
	yp_dstrdestroy(&(curtoken.str));
	yp_dstrdestroy(&(nexttoken.str));
}

// syntax analyzer
static int yp_readstring(char **s)
{
	struct yp_token *t;
	
	if (yp_gettokentype(&t, YP_T_STRING) < 0)
		return (-1);

	if ((*s = strdup(t->str.data)) == NULL) {
		errcode = YP_ERR_JRALLOC;
		yp_seterrmessage("Cannot allocate memory.");
		return (-1);
	}

	return 0;
}

static int yp_readstringmember(char **s)
{
	struct yp_token *t;

	if (yp_gettokentype(&t, YP_T_STRING) < 0)
		return (-1);
	
	if (yp_gettokentype(&t, YP_T_COLON) < 0)
		return (-1);

	return yp_readstring(s);
}

static int yp_readnumbermember(int *n)
{
	struct yp_token *t;
	
	if (yp_gettokentype(&t, YP_T_STRING) < 0)
		return (-1);
	
	if (yp_gettokentype(&t, YP_T_COLON) < 0)
		return (-1);

	if (yp_gettokentype(&t, YP_T_NUMBER) < 0)
		return (-1);

	*n = atoi(t->str.data);

	return 0;
}

static int yp_readarraymember(char ***a, int *count)
{
	struct yp_token *t;
	size_t size;
	int i;

	// init array
	*count = 0;
	size = 24;
	if ((*a = malloc(sizeof(char **) * size)) == NULL) {
		errcode = YP_ERR_JRALLOC;
		yp_seterrmessage("Cannot allocate memory.");
		return (-1);
	}

	// get array name
	if (yp_gettokentype(&t, YP_T_STRING) < 0)
		goto error;

	if (yp_gettokentype(&t, YP_T_COLON) < 0)
		goto error;

	// element list begin
	if (yp_gettokentype(&t, YP_T_LBRACKET) < 0)
		goto error;

	// get elements, one by one
	if (yp_readstring(*a + (*count)++) < 0)
		goto error;

	while (yp_peektoken(&t) == YP_T_COMMA) {
		if (yp_gettokentype(&t, YP_T_COMMA) < 0)
			goto error;

		if (*count >= size) {
			size *= 2;
			if ((*a = realloc(*a, sizeof(char **) * size))
				== NULL) {
				errcode = YP_ERR_JRALLOC;
				yp_seterrmessage("Cannot allocate memory.");
				return (-1);
			}
		}

		if (yp_readstring(*a + (*count)++) < 0)
			goto error;
	}

	// element list end
	if (yp_gettokentype(&t, YP_T_RBRACKET) < 0)
		goto error;
	
	return 0;

error:
	for (i = 0; i < *count; ++i)
		free((*a)[i]);

	free(*a);

	return (-1);
}

static int yp_readreqelement(char **ver, struct yp_request *r)
{
	struct yp_token *t;

	if (yp_peektoken(&t) == YP_T_ERROR)
		return (-1);
	
	if (strcmp(t->str.data, "jsonrpc") == 0) {
		if (yp_readstringmember(ver) < 0)
			return (-1);
	}
	else if (strcmp(t->str.data, "id") == 0) {
		if (yp_readnumbermember(&(r->id)) < 0)
			return (-1);
	}
	else if (strcmp(t->str.data, "method") == 0) {
		if (yp_readstringmember(&(r->method)) < 0)
			return (-1);
	}
	else if (strcmp(t->str.data, "params") == 0) {
		if (yp_readarraymember(&(r->param),
			&(r->paramcount)) < 0) {
			return (-1);
		}
	}
	else {
		errcode = YP_ERR_JRINVALRES;
		yp_seterrmessage("request have an excess element.\n");
	
		return (-1);
	}

	return 0;
}

int yp_readrequest(FILE *f, struct yp_request *r)
{
	struct yp_token *t;
	char *ver;

	ver = NULL;
	r->id = -1;
	r->method = NULL;
	r->param = NULL;
	r->paramcount = 0;
	
	yp_startparsing(f);

	// request begin
	if (yp_gettokentype(&t, YP_T_LBRACE) < 0)
		goto error;

	// reading elemnts
	if (yp_readreqelement(&ver, r) < 0)
		goto error;
	
	while (yp_peektoken(&t) == YP_T_COMMA) {
		if (yp_gettokentype(&t, YP_T_COMMA) < 0)
			goto error;
	
		if (yp_readreqelement(&ver, r) < 0)
			goto error;
	}
	
	// request end
	if (yp_gettokentype(&t, YP_T_RBRACE) < 0)
		goto error;

	// check version
	if (ver == NULL || strcmp(ver, "2.0") != 0) {
		errcode = YP_ERR_JRINVALREQ;
		yp_seterrmessage("json-rpc version doesn't match.\n");
	
		goto error;
	}
	
	// check method
	if (r->method == NULL) {
		errcode = YP_ERR_JRINVALREQ;
		yp_seterrmessage("no method element in request.\n");

		goto error;
	}

	// check id
	if (r->id < 0) {
		errcode = YP_ERR_JRINVALREQ;
		yp_seterrmessage("request should have a positive id.\n");

		goto error;
	}

	if (ver != NULL)
		free(ver);
	
	yp_endparsing();
	
	return 0;

error:
	if (ver != NULL)
		free(ver);

	yp_destroyrequest(r);
	
	yp_endparsing();
	return errcode;
}

static int yp_readreserrorelement(struct yp_response *r)
{
	struct yp_token *t;
	
	if (yp_peektoken(&t) == YP_T_ERROR)
		return (-1);

	if (strcmp(t->str.data, "code") == 0) {
		if (yp_readnumbermember(&(r->errcode)) < 0)
			return (-1);
	}
	else if (strcmp(t->str.data, "message") == 0) {
		if (yp_readstringmember(&(r->errmessage)) < 0)
			return (-1);
	}
	else {
		errcode = YP_ERR_JRINVALRES;
		yp_seterrmessage("request have an excess element.");
	
		return (-1);
	}

	return 0;
}

static int yp_readreselement(char **ver, struct yp_response *r)
{
	struct yp_token *t;

	if (yp_peektoken(&t) == YP_T_ERROR)
		return (-1);
	
	if (strcmp(t->str.data, "jsonrpc") == 0) {
		if (yp_readstringmember(ver) < 0)
			return (-1);
	}
	else if (strcmp(t->str.data, "id") == 0) {
		if (yp_readnumbermember(&(r->id)) < 0)
			return (-1);
	}
	else if (strcmp(t->str.data, "result") == 0) {
		if (yp_readstringmember(&(r->result)) < 0)
			return (-1);
	}
	else if (strcmp(t->str.data, "error") == 0) {
		if (yp_gettokentype(&t, YP_T_STRING) < 0)
			return (-1);
		
		if (yp_gettokentype(&t, YP_T_COLON) < 0)
			return (-1);
		
		if (yp_gettokentype(&t, YP_T_LBRACE) < 0)
			return (-1);
	
		
		if (yp_readreserrorelement(r) < 0)
			return (-1);

		while (yp_peektoken(&t) == YP_T_COMMA) {
			if (yp_gettokentype(&t, YP_T_COMMA) < 0)
				return (-1);
	
			if (yp_readreserrorelement(r) < 0)
				return (-1);
		}

		if (yp_gettokentype(&t, YP_T_RBRACE) < 0)
			return (-1);
	}
	else {
		errcode = YP_ERR_JRINVALRES;
		yp_seterrmessage("request have an excess element.");
	
		return (-1);
	}

	return 0;
}

int yp_readresponse(FILE *f, struct yp_response *r)
{
	struct yp_token *t;
	char *ver;

	r->id = -1;
	ver = NULL;	
	r->result = NULL;
	r->errcode = 0;
	r->errmessage = NULL;

	yp_startparsing(f);

	// response begin
	if (yp_gettokentype(&t, YP_T_LBRACE) < 0)
		goto error;

	// reading elements
	if (yp_readreselement(&ver, r) < 0)
		goto error;
	
	while (yp_peektoken(&t) == YP_T_COMMA) {
		if (yp_gettokentype(&t, YP_T_COMMA) < 0)
			goto error;
	
		if (yp_readreselement(&ver, r) < 0)
			goto error;
	}
	
	// response end
	if (yp_gettokentype(&t, YP_T_RBRACE) < 0)
		goto error;

	// version check
	if (ver == NULL || strcmp(ver, "2.0") != 0) {
		errcode = YP_ERR_JRINVALRES;
		yp_seterrmessage("json-rpc version doesn't match.");
	
		goto error;
	}

	// if no result, should have an error element
	if (r->result == NULL
		&& (r->errcode == 0 || r->errmessage == NULL)) {
		errcode = YP_ERR_JRINVALRES;
		yp_seterrmessage("request with no result doesn't have an error element.");

		goto error;	
	}

	// if have result, should not have an error element
	if (r->result != NULL
		&& (r->errcode != 0 || r->errmessage != NULL)) {
		errcode = YP_ERR_JRINVALRES;
		yp_seterrmessage("request with result have an error element.");

		goto error;	
	}

	if (ver != NULL)	
		free(ver);

	yp_endparsing();

	return 0;

error:
	if (ver != NULL)
		free(ver);

	yp_destroyresponse(r);
	
	yp_endparsing();
	return errcode;
}

int fprintquoted(FILE *f, const char *s)
{
	const char *c, *cc;

	for (cc = c = s; ; ++cc)
		if (*cc == '"' || *cc == '\\' || *cc == '\0') {
			char b[2];
			
			if (fwrite(c, sizeof(char), cc - c, f) != (cc - c))
				return (-1);

			if (*cc == '\0')
				break;

			b[0] = '\\';
			b[1] = *cc;	
			
			if (fwrite(b, sizeof(char), 2, f) != 2) 
				return (-1);
			
			c = cc + 1;
		}

	return 0;
}

// writing functions
int yp_writerequest(const struct yp_request *r, FILE *f)
{
	int i;
	
	XFPRINTF(f, "{\n")
	XFPRINTF(f, "\t\"jsonrpc\": \"2.0\",\n")
	XFPRINTF(f, "\t\"id\": %d,\n", r->id)
	XFPRINTF(f, "\t\"method\": \"%s\"", r->method)

	if (r->paramcount != 0) {
		XFPRINTF(f, ",\n\t\"params\": [\n")
		for (i = 0; i < r->paramcount; ++i) {
			XFPRINTF(f, "\t\t\"")
			if (fprintquoted(f, r->param[i]) < 0)
				goto error;
			XFPRINTF(f, "\"%s\n",
				((i < r->paramcount - 1) ? "," : ""))
		}
		XFPRINTF(f, "\t]")
	}
	
	XFPRINTF(f, "\n}\n")

	if (fflush(f) == EOF)
		goto error;

	return 0;

error:
	errcode = errno;
	yp_seterrmessage(strerror(errno));

	return (-1);
}

int yp_writeresponse(const struct yp_response *r, FILE *f)
{
	XFPRINTF(f, "{\n")
	XFPRINTF(f, "\t\"jsonrpc\": \"2.0\",\n")
	
	if (r->id >= 0)
		XFPRINTF(f, "\t\"id\": %d,\n", r->id)

	if (r->result != NULL) {
		XFPRINTF(f, "\t\"result\": \"")
		if (fprintquoted(f, r->result) < 0)
			goto error;
		XFPRINTF(f, "\"\n")
	}
	else {
		XFPRINTF(f, "\t\"error\" : {\n")
		XFPRINTF(f, "\t\t\"code\": %d,\n", r->errcode)

		XFPRINTF(f, "\t\t\"message\": \"")
		if (fprintquoted(f, r->errmessage) < 0)
			goto error;
		XFPRINTF(f, "\"\n")

		XFPRINTF(f, "\t}\n");
	}
	
	XFPRINTF(f, "%s", "}\n");

 	if (fflush(f) == EOF)
		goto error;

	return 0;

error:
	errcode = errno;
	yp_seterrmessage(strerror(errno));

	return (-1);
}

// memory management
int yp_createrequest(struct yp_request *r, int id,
	const char *method, const char **params, int paramcount)
{
	int i;

	r->id = id;
	if ((r->method = strdup(method)) == NULL)
		goto error;

	if (paramcount != 0) {
		r->paramcount = paramcount;

		if ((r->param = malloc(sizeof(char *)
			* paramcount)) == NULL) {
			goto error;
		}

		for (i = 0; i < paramcount; ++i) {
			if ((r->param[i] = strdup(params[i])) == NULL)
				goto error;
		}
	}
	else {
		r->paramcount = 0;
		r->param = NULL;
	}

	return 0;

error:
	errcode = YP_ERR_JRALLOC;
	yp_seterrmessage("Cannot allocate memory.");
	return (-1);
}

int yp_createresponse(struct yp_response *r, int id,
	const char *result, int errcode, char *errmessage)
{
	r->id = id;

	if (result != NULL) {
		if ((r->result = strdup(result)) == NULL)
			goto error;
	
		r->errcode = 0;
		r->errmessage = NULL;
	}
	else {
		r->result = NULL;
		r->errcode = errcode;
	
		if ((r->errmessage = strdup(errmessage)) == NULL)
			goto error;
	}

	return 0;

error:
	errcode = YP_ERR_JRALLOC;
	yp_seterrmessage("Cannot allocate memory.");
	return (-1);
}

void yp_destroyrequest(struct yp_request *r)
{
	int i;

	if (r->method != NULL)
		free(r->method);

	for (i = 0; i < r->paramcount; ++i)
		free(r->param[i]);

	if (r->param != NULL)
		free(r->param);
}

void yp_destroyresponse(struct yp_response *r)
{
	if (r->result != NULL)
		free(r->result);

	if (r->errmessage != NULL)
		free(r->errmessage);
}

// error reporting
char *yp_jrstrerror()
{
	return errmessage;
}
