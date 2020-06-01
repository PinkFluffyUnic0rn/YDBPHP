#ifndef YP_RPC_H
#define YP_RPC_H

#define YP_MAX_JRERRMESSAGE 4096

enum YP_JRERROR {
	YP_ERR_JRPARSE = -32700,
	YP_ERR_JRINVALREQ = -32600,
	YP_ERR_JRUNKNOWNMETHOD = -32601,
	YP_ERR_JRINVALPARAMS = -32602,
	YP_ERR_JRINVALRES = -32001,
	YP_ERR_JRALLOC = -32004
};

struct yp_request
{
	int id;
	char *method;
	int paramcount;
	char **param;
};

struct yp_response
{
	int id;
	char *result;
	int errcode;
	char *errmessage;
};

int yp_readrequest(FILE *f, struct yp_request *r);

int yp_readresponse(FILE *f, struct yp_response *r);

int yp_writerequest(const struct yp_request *r, FILE *f);

int yp_writeresponse(const struct yp_response *r, FILE *f);

int yp_createrequest(struct yp_request *r, int id,
	const char *method, const char **params, int paramcount);

int yp_createresponse(struct yp_response *r, int id,
	const char *result, int errcode, char *errmessage);

void yp_destroyrequest(struct yp_request *r);

void yp_destroyresponse(struct yp_response *r);

char *yp_jrstrerror();

#endif
