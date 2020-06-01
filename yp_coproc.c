#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>

#include "libyottadb.h"

#include "yp_coproc.h"
#include "yp_rpc.h"

#define REQID_MAX 1024

FILE *cldreqfile;
FILE *cldresfile;
FILE *parreqfile;
FILE *parresfile;

int pid;

int initilized = 0;

char ydbphpfiles[PATH_MAX + 1];

void yp_seterror(int errcode, const char *errmessage)
{
	yp_errcode = errcode;
	strncpy(yp_errmessage, errmessage, YP_MAX_ERRMESSAGE);
}

static int yp_nextreqid()
{
	static int reqid = 0;

	return (reqid != REQID_MAX) ? (reqid++) : 0;
}

#define YP_COPROCSETERROR(ec, em)	\
do {					\
	errcode = ec;			\
	errmessage = em;		\
} while (0);

static int yp_ydbinit(struct yp_request *req, struct yp_response *res)
{
	char *errmessage;
	int errcode;
	char serr[YDB_MAX_MSG];
	char *yc;
	int i, ii;
	int s;

	// set enviroment variables
	if (setenv("gtmroutines", ydbphpfiles, 1) < 0) {
		YP_COPROCSETERROR(errno, strerror(errno));
		goto setroutineserror;
	}

	if ((yc = malloc(strlen("GTMCI=")
		+ strlen(ydbphpfiles)
		+ strlen("/ydbphp.ci") + 1)) == NULL) {

		YP_COPROCSETERROR(YDB_ERR_ALLOC,
			"Cannot allocate memory.");
		goto gtmciallocerror;
	}
	
	sprintf(yc, "GTMCI=%s/ydbphp.ci", ydbphpfiles);
	
	if (putenv(yc) != 0) {
		YP_COPROCSETERROR(errno, strerror(errno));
		goto gtmciputerror;
	}
	
	for (i = 0; i < req->paramcount; ++i) {
		char *c;

		if ((c = strchr(req->param[i], '=')) == NULL) {
			YP_COPROCSETERROR(YDB_ERR_ENV,
				"'=' symbol not found in enviroment variable definition.");
			goto eqerror;
		}

		*c++ = '\0';

		if (strcmp(req->param[i], "gtmroutines") == 0) {
			const char *gr;
			char *newgr;

			gr = getenv("gtmroutines");

			if ((newgr = malloc(strlen("gtmroutines=")
				+ strlen(gr)
				+ strlen(c) + 1)) == NULL) {

				YP_COPROCSETERROR(YDB_ERR_ALLOC,
					"Cannot allocate memory.");
				goto nroutinesallocerror;
			}

			sprintf(newgr, "gtmroutines=%s %s", gr, c);

			if (putenv(newgr) != 0) {
				YP_COPROCSETERROR(errno, strerror(errno));	
				goto nroutinesputerror;
			}
		}
		else if (strcmp(req->param[i], "GTMCI") == 0) {
			YP_COPROCSETERROR(YDB_ERR_ENV,
				"GTMCI cannot be changed.");
			goto ngtmciputerror;
		}
		else {
			if (setenv(req->param[i], c, 1) < 0) {
				YP_COPROCSETERROR(errno, strerror(errno));	
				goto setenverror;
			}
		}
	}

	// init YottaDB	
	if ((s = ydb_init()) != YDB_OK) {
		ydb_zstatus(serr, YDB_MAX_MSG);

		YP_COPROCSETERROR(s, serr);	
		goto ydbiniterror;
	}

	// create response indicating successive initializaton
	if ((s = yp_createresponse(res, req->id, "", 0, NULL) < 0)) {
		YP_COPROCSETERROR(s, yp_jrstrerror());	
		return (-1);
	}

	return 0;

ydbiniterror:
setenverror:
ngtmciputerror:
nroutinesputerror:
nroutinesallocerror:
eqerror:
	for (ii = i - 1; i >= 0; --ii)
		unsetenv(req->param[ii]);

gtmciputerror:
gtmciallocerror:
setroutineserror:
	if (yp_createresponse(res, req->id, NULL,  errcode,
		errmessage) < 0) {
 		return (-1);
 	}

	return 0;
}

#define YP_CALLINPARAMCHECK(pcount, expected)				\
do {									\
	if (pcount != expected) {					\
		YP_COPROCSETERROR(YP_ERR_JRINVALPARAMS,			\
			"Wrong number of parameters in request.");	\
									\
		goto error;						\
	}								\
} while (0);

static int yp_ydbcallin(struct yp_request *req, struct yp_response *res)
{
	char *errmessage;
	int errcode;
	char serr[YDB_MAX_MSG];
	char ret[YDB_MAX_STR];
	int s;

	// call YottaDB method, named in "method" element
	if (strcmp(req->method, "ydb_get") == 0) {
		YP_CALLINPARAMCHECK(req->paramcount, 1);

		s = ydb_ci(req->method, ret, req->param[0]);
	}
	else if (strcmp(req->method, "ydb_set") == 0) {
		YP_CALLINPARAMCHECK(req->paramcount, 2);
		
		s = ydb_ci(req->method, req->param[0], req->param[1]);
	}
	else if (strcmp(req->method, "ydb_kill") == 0) {
		YP_CALLINPARAMCHECK(req->paramcount, 1);

		s = ydb_ci(req->method, req->param[0]);
	}
	else if (strcmp(req->method, "ydb_data") == 0) {
		YP_CALLINPARAMCHECK(req->paramcount, 1);

		s = ydb_ci(req->method, ret, req->param[0]);
	}
	else if (strcmp(req->method, "ydb_order") == 0) {
		YP_CALLINPARAMCHECK(req->paramcount, 2);

		s = ydb_ci("ydb_order", ret, req->param[0], req->param[1]);	
	}
	else {
		YP_COPROCSETERROR(YP_ERR_JRUNKNOWNMETHOD,
			"Unknown method.");
		goto error;
	}
	
	if (s != 0) {
		ydb_zstatus(serr, YDB_MAX_MSG);

		YP_COPROCSETERROR(s, serr);
		goto error;
	}

	// create rsponse with result
	if (yp_createresponse(res, req->id, ret, 0, NULL) < 0)
		return (-1);

	return 0;

error:
	if (yp_createresponse(res, req->id, NULL,
		errcode, errmessage) < 0) {
		return (-1);
	}

	return 0;
}

static int yp_mainloop()
{
	int final;

	final = 0;
	while (1) {
		struct yp_request req;
		struct yp_response res;
		int r;

		// get request and invoke approptiate method
		if ((r = yp_readrequest(cldreqfile, &req)) < 0) {
			if (yp_createresponse(&res, -1,
				NULL, r, yp_jrstrerror()) < 0)
				return 1;
		
			if ((r = yp_writeresponse(&res, cldresfile)) < 0)
				return 1;
		
			return 1;
		}
		else if (strcmp(req.method, "ydb_init") == 0) {	
			if (yp_ydbinit(&req, &res) < 0)
				return 1;
		}
		else if (strcmp(req.method, "ydb_finilize") == 0) {
			if (yp_createresponse(&res,
				req.id, "", 0, NULL) < 0)
				return 1;

			final = 1;
		}
		else {
			if (yp_ydbcallin(&req, &res) < 0)
				return 1;
		}

		// request is not needed anymore
		yp_destroyrequest(&req);

		// send response
		if ((r = yp_writeresponse(&res, cldresfile)) < 0)
			return 1;

		// response is not needed too
		yp_destroyresponse(&res);

		// is "finilize" method was invoked, stop main loop	
		if (final)
			return 0;
	}

	return 1;
}

int yp_coprocinit(const char **env, int envc, const char *files)
{
	int reqp[2];
	int resp[2];

	strncpy(ydbphpfiles, files, PATH_MAX + 1);

	if (pipe(reqp) < 0) {
		yp_seterror(errno, strerror(errno));
		goto pipe1error;
	}
	
	if (pipe(resp) < 0) {
		yp_seterror(errno, strerror(errno));
		goto pipe2error;
	}

	if ((pid = fork()) == 0) {
		if (close(reqp[1]) < 0)
			exit(1);
		if (close(resp[0]) < 0)
			exit(1);

		if ((cldreqfile = fdopen(reqp[0], "r")) == NULL)
			exit(1);
		if ((cldresfile = fdopen(resp[1], "w")) == NULL)
			exit(1);

		exit(yp_mainloop());
	}
	else if (pid > 0) {
		if (close(reqp[0]) < 0) {
			yp_seterror(errno, strerror(errno));
			goto close1error;
		}

		if (close(resp[1]) < 0) {
			yp_seterror(errno, strerror(errno));
			goto close2error;
		}

		if ((parreqfile = fdopen(reqp[1], "w")) == NULL) {
			yp_seterror(errno, strerror(errno));
			goto open1error;
		}
			
		if ((parresfile = fdopen(resp[0], "r")) == NULL) {
			yp_seterror(errno, strerror(errno));
			goto open2error;
		}

		if (yp_coprocreq(NULL, "ydb_init", env, envc) < 0) {
			goto coprocerror;
		}
	
		initilized = 1;
	}
	else {
		yp_seterror(errno, strerror(errno));
		goto forkerror;
	}
	
	return 0;

coprocerror:
	fclose(parresfile);

open2error:
	fclose(parreqfile);

open1error:
close2error:
close1error:
forkerror:
	close(resp[0]);
	close(resp[1]);

pipe2error:
	close(reqp[0]);
	close(reqp[1]);

pipe1error:
	return (-1);
}

int yp_coprocfinilize()
{
	int e, s;

	e = 0;

	if (initilized == 0)
		return e;

	if (yp_coprocreq(NULL, "ydb_finilize", NULL, 0) < 0)
		e = -1;
	
	if (fclose(parreqfile) == EOF) {
		yp_seterror(errno, strerror(errno));
		e = -1;
	}

	if (fclose(parresfile) == EOF) {
		yp_seterror(errno, strerror(errno));
		e = -1;
	}

	if (waitpid(pid, &s, 0) < 0) {
		yp_seterror(errno, strerror(errno));
		e = -1;
	}

	initilized = 0;

	return e;
}

int yp_coprocreq(char **result, const char *name, const char **params,
	int paramcount)
{
	struct yp_request req;
	struct yp_response res;
	int r;

	if (!initilized && strcmp(name, "ydb_init") != 0) {
		yp_seterror(YDB_ERR_NOTINITILIZED,
			"Coprocess not initilized. \
You should run ydb_init() first.");
		goto notinitilizederror;
	}
	
	if (yp_createrequest(&req, yp_nextreqid(), name,
		params, paramcount) < 0) {
		yp_seterror(r, yp_jrstrerror());
		goto createreqerror;
	}

	if ((r = yp_writerequest(&req, parreqfile)) < 0) {
		yp_seterror(r, yp_jrstrerror());
		goto writereqerror;
	}

	if ((r = yp_readresponse(parresfile, &res)) < 0) {
		yp_seterror(r, yp_jrstrerror());
		goto readresperror;
	}

	if (res.result == NULL) {
		yp_seterror(res.errcode, res.errmessage);
		goto resulterror;
	}

	if (result != NULL)
		if ((*result = strdup(res.result)) == NULL) {
			yp_seterror(YDB_ERR_ALLOC,
				"Cannot allocate memory.");
			goto dupcerror;
		}

	yp_destroyrequest(&req);
	yp_destroyresponse(&res);

	return 0;

dupcerror:
resulterror:
	yp_destroyresponse(&res);

readresperror:
writereqerror:
	yp_destroyrequest(&req);

createreqerror:
notinitilizederror:

	return (-1);
}
