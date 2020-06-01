#ifndef YP_COPROC_H
#define YP_COPROC_H

#include "libyottadb.h"

#define YDB_MAX_MSG 2048
#define YP_MAX_ERRMESSAGE 4096

int yp_errcode;
char yp_errmessage[YP_MAX_ERRMESSAGE];

enum YDB_ERR {
	YDB_ERR_LASTCLD = YDB_MAX_YDBERR + 1,
	YDB_ERR_NUMOFRANGE = YDB_MAX_YDBERR + 2,
	YDB_ERR_STR2LONG = YDB_MAX_YDBERR + 3,
	YDB_ERR_NOTINITILIZED = YDB_MAX_YDBERR + 4,
	YDB_ERR_ALLOC = YDB_MAX_YDBERR + 5,
	YDB_ERR_ENV = YDB_MAX_YDBERR + 6,
	YDB_ERR_WRONGARGS = YDB_MAX_YDBERR + 7
};

int yp_coprocinit(const char **env, int envc, const char *files);

int yp_coprocfinilize();

int yp_coprocreq(char **res, const char *name, const char **params,
	int paramcount);

void yp_seterror(int errcode, const char *errmessage);

#endif
