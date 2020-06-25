#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#include "stdio.h"

#include "yp_dstring.h"
#include "yp_coproc.h"

#define PHP_YDBPHP_VERSION "1.0"
#define PHP_YDBPHP_EXTNAME "ydbphp"

#define MAX_AGRS 1024

PHP_MINIT_FUNCTION(minit);
PHP_RINIT_FUNCTION(rinit);
PHP_RSHUTDOWN_FUNCTION(rshutdown);
PHP_MSHUTDOWN_FUNCTION(mshutdown);
PHP_FUNCTION(ydb_init);
PHP_FUNCTION(ydb_finilize);
PHP_FUNCTION(ydb_set);
PHP_FUNCTION(ydb_get);
PHP_FUNCTION(ydb_gettyped);
PHP_FUNCTION(ydb_kill);
PHP_FUNCTION(ydb_data);
PHP_FUNCTION(ydb_order);
PHP_FUNCTION(ydb_error);
PHP_FUNCTION(ydb_strerror);
PHP_FUNCTION(ydb_zsinfo);

extern zend_module_entry ydbphp_module_entry;
#define phpext_ydbphp_ptr &ydbphp_module_entry

ZEND_BEGIN_ARG_INFO_EX(arginfo_ydb_zsinfo, 0, 0, 1)
	ZEND_ARG_INFO(1, id)
	ZEND_ARG_INFO(1, sev)
	ZEND_ARG_INFO(1, text)
ZEND_END_ARG_INFO()

static zend_function_entry ydbphp_functions[] = {
	PHP_FE(ydb_init, NULL)
	PHP_FE(ydb_finilize, NULL)
	PHP_FE(ydb_set, NULL)
	PHP_FE(ydb_get, NULL)
	PHP_FE(ydb_gettyped, NULL)
	PHP_FE(ydb_kill, NULL)
	PHP_FE(ydb_data, NULL)
	PHP_FE(ydb_order, NULL)
	PHP_FE(ydb_error, NULL)
	PHP_FE(ydb_strerror, NULL)
	PHP_FE(ydb_zsinfo, arginfo_ydb_zsinfo)
	{NULL, NULL, NULL}
};

zend_module_entry ydbphp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PHP_YDBPHP_EXTNAME,
	ydbphp_functions,
	PHP_MINIT(minit),
	PHP_MSHUTDOWN(mshutdown),
	PHP_RINIT(rinit),
	PHP_RSHUTDOWN(rshutdown),
	NULL,
#if ZEND_MODULE_API_NO >= 20010901
	PHP_YDBPHP_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(ydbphp)

PHP_INI_BEGIN()
PHP_INI_ENTRY("ydbphp.routinepath", NULL, PHP_INI_SYSTEM, NULL)
PHP_INI_END()

PHP_MINIT_FUNCTION(minit)
{
	REGISTER_INI_ENTRIES();

	REGISTER_LONG_CONSTANT("YDB_SEVERITY_ERROR",
		YDB_SEVERITY_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("YDB_SEVERITY_FATAL",
		YDB_SEVERITY_FATAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("YDB_SEVERITY_INFORMATIONAL",
		YDB_SEVERITY_INFORMATIONAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("YDB_SEVERITY_SUCCESS",
		YDB_SEVERITY_SUCCESS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("YDB_SEVERITY_WARNING",
		YDB_SEVERITY_WARNING, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}

PHP_RINIT_FUNCTION(rinit)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(rshutdown)
{
	if (yp_coprocfinilize() < 0)
		return FAILURE;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(mshutdown)
{
	UNREGISTER_INI_ENTRIES();
}

static int yp_mvarpathadd(struct yp_dstring *s, const zval *v)
{
	if (Z_TYPE_P(v) == IS_LONG || Z_TYPE_P(v) == IS_DOUBLE) {
		zval zv;
		ZVAL_COPY_VALUE(&zv, v);

		convert_to_string(&zv);

		if (yp_dstradds(s, zv.value.str->val) < 0) {
			zval_dtor(&zv);

			yp_seterror(YDB_ERR_ALLOC,
				"Cannot allocate memory.");
			return (-1);
		}

		zval_dtor(&zv);
	}
	else if (Z_TYPE_P(v) == IS_STRING) {
		if (yp_dstraddc(s, '"') < 0) {
			yp_seterror(YDB_ERR_ALLOC,
				"Cannot allocate memory.");
			return (-1);
		}

		if (yp_dstradds(s, v->value.str->val) < 0) {
			yp_seterror(YDB_ERR_ALLOC,
				"Cannot allocate memory.");
			return (-1);
		}

		if (yp_dstraddc(s, '"') < 0) {
			yp_seterror(YDB_ERR_ALLOC,
				"Cannot allocate memory.");
			return (-1);
		}
	}
	else {
		yp_seterror(YDB_ERR_WRONGARGS,
		"M array indexes should be string, \
number, or array of strings and numbers");
		return (-1);
	}

	return 0;
}

static int yp_mvarpatharrayadd(struct yp_dstring *s, const zval *v)
{
	zval *idx;
	
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(v), idx) {
		if (yp_mvarpathadd(s, idx) < 0)
			return (-1);

		if (yp_dstraddc(s, ',') < 0) {
			yp_seterror(YDB_ERR_ALLOC,
				"Cannot allocate memory.");
			return (-1);
		}
	} ZEND_HASH_FOREACH_END();
	
	// a dirty hack
	s->data[--(s->len)] = '\0';

	return 0;
}

static int yp_mvarpath(const zval *vargs, int vargscount,
	struct yp_dstring *s)
{
	int i;

	if (Z_TYPE_P(vargs + 0) != IS_STRING) {
		yp_seterror(YDB_ERR_WRONGARGS,
			"M array name should be a string.");
		return (-1);
	}

	if (yp_dstradds(s, vargs[0].value.str->val) < 0) {
		yp_seterror(YDB_ERR_ALLOC,
			"Cannot allocate memory.");
		return (-1);
	}
	
	if (yp_dstraddc(s, '(') < 0) {
		yp_seterror(YDB_ERR_ALLOC,
			"Cannot allocate memory.");
		return (-1);
	}

	for (i = 1; i < vargscount; ++i) {
		if (Z_TYPE_P(vargs + i) == IS_ARRAY) {
			if (yp_mvarpatharrayadd(s, vargs + i) < 0)
				return (-1);
		}
		else {
			if (yp_mvarpathadd(s, vargs + i) < 0)
				return (-1);
		}

		if (i != vargscount - 1) {
			if (yp_dstraddc(s, ',') < 0) {
				yp_seterror(YDB_ERR_ALLOC,
					"Cannot allocate memory.");
				return (-1);
			}
		}
	}

	if (yp_dstraddc(s, ')') < 0) {
		yp_seterror(YDB_ERR_ALLOC, "Cannot allocate memory.");
		return (-1);
	}
		
	return 0;
}

PHP_FUNCTION(ydb_init)
{
	const char **env;
	zval *vargs;
	int vargscount;
	int i;

	
	vargs = NULL;
	vargscount = 0;

	// read arguments to env
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"*", &vargs, &vargscount) != SUCCESS) {
		yp_seterror(YDB_ERR_WRONGARGS, "Wrong arguments.");
		goto parseparamerror;
	}
	
	if ((env = malloc(sizeof(char *) * ZEND_NUM_ARGS())) == NULL) {
		yp_seterror(YDB_ERR_ALLOC, "Cannot allocate memory.");
		goto envallocerror;
	}

	for (i = 0; i < vargscount; ++i) {
		if (Z_TYPE_P(vargs + i) != IS_STRING) {
			yp_seterror(YDB_ERR_WRONGARGS,
				"All arguments chould be strings.");
			goto vargserror;
		}

		env[i] = vargs[i].value.str->val;
	}

	if (INI_STR("ydbphp.routinepath") == NULL) {
		yp_seterror(YDB_ERR_CONFIG,
			"No routines path found in .ini files.");
		goto configerror;
	}

	if (yp_coprocinit(env, vargscount, INI_STR("ydbphp.routinepath")) < 0)
		goto coprocerror;

	if (env != NULL)
		free(env);

	RETURN_BOOL(1);

coprocerror:
configerror:
vargserror:
envallocerror:
	if (env != NULL)
		free(env);
	
parseparamerror:
	RETURN_BOOL(0);
}

PHP_FUNCTION(ydb_finilize)
{
	if (yp_coprocfinilize() < 0)
		RETURN_BOOL(0);

	RETURN_BOOL(1);
}

PHP_FUNCTION(ydb_set)
{
	zval *vargs;
	int vargscount;
	char *val;
	size_t vallen;
	
	struct yp_dstring s;
	const char *params[2];

	vargs = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"+s", &vargs, &vargscount, &val, &vallen) != SUCCESS) {
		yp_seterror(YDB_ERR_WRONGARGS, "Wrong arguments.");
		goto parseparamerror;
	}

	yp_dstrinit(&s);	
	if (yp_mvarpath(vargs, vargscount, &s) < 0)
		goto mvpatherror;


	params[0] = s.data;
	params[1] = val;

	if (yp_coprocreq(NULL, "ydb_set", params, 2) < 0)
		goto coprocerror;
	
	yp_dstrdestroy(&s);

	RETURN_BOOL(1);

coprocerror:
mvpatherror:
	yp_dstrdestroy(&s);

parseparamerror:
	RETURN_BOOL(0);
}

PHP_FUNCTION(ydb_get)
{
	zval *vargs;
	int vargscount;
	
	struct yp_dstring s;
	const char *params[2];

	zend_string *zs;
	char *res;

	vargs = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"+", &vargs, &vargscount) != SUCCESS) {
		yp_seterror(YDB_ERR_WRONGARGS, "Wrong arguments.");
		goto parseparamerror;
	}

	yp_dstrinit(&s);	
	if (yp_mvarpath(vargs, vargscount, &s) < 0)
		goto mvpatherror;


	params[0] = s.data;

	if (yp_coprocreq(&res, "ydb_get", params, 1) < 0)
		goto coprocerror;

	zs = zend_string_init(res, strlen(res), 0);

	free(res);
	yp_dstrdestroy(&s);

	RETURN_STR(zs);

coprocerror:
mvpatherror:
	yp_dstrdestroy(&s);

parseparamerror:
	RETURN_BOOL(0);
}

PHP_FUNCTION(ydb_gettyped)
{
	zval *vargs;
	int vargscount;
	
	struct yp_dstring s;
	const char *params[2];

	zend_string *zs;
	char *res;

	int t;
	long integer;
	double floating;

	vargs = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"+", &vargs, &vargscount) != SUCCESS) {
		yp_seterror(YDB_ERR_WRONGARGS, "Wrong arguments.");
		goto parseparamerror;
	}

	yp_dstrinit(&s);	
	if (yp_mvarpath(vargs, vargscount, &s) < 0)
		goto mvpatherror;

	params[0] = s.data;

	if (yp_coprocreq(&res, "ydb_get", params, 1) < 0)
		goto coprocerror;

	t = is_numeric_string(res, strlen(res), &integer, &floating, 0);
	
	yp_dstrdestroy(&s);

	if (strcasecmp(res, "true") == 0) {
		free(res);
		RETURN_BOOL(1);
	}
	else if (strcasecmp(res, "false") == 0) {
		free(res);
		RETURN_BOOL(0);
	}
	else if (t == IS_LONG) {
		free(res);
		RETURN_LONG(integer);
	}
	else if (t == IS_DOUBLE) {
		free(res);
		RETURN_DOUBLE(floating);
	} else {
		zs = zend_string_init(res, strlen(res), 0);

		free(res);
		RETURN_STR(zs);
	}

coprocerror:
mvpatherror:
	yp_dstrdestroy(&s);

parseparamerror:
	RETURN_BOOL(0);
}

PHP_FUNCTION(ydb_kill)
{
	zval *vargs;
	int vargscount;
	
	struct yp_dstring s;
	const char *params[2];

	zend_string *zs;
	char *res;

	vargs = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"+", &vargs, &vargscount) != SUCCESS) {
		yp_seterror(YDB_ERR_WRONGARGS, "Wrong arguments.");
		goto parseparamerror;
	}

	yp_dstrinit(&s);	
	if (yp_mvarpath(vargs, vargscount, &s) < 0)
		goto mvpatherror;


	params[0] = s.data;

	if (yp_coprocreq(NULL, "ydb_kill", params, 1) < 0)
		goto coprocerror;

	yp_dstrdestroy(&s);

	RETURN_BOOL(1);

coprocerror:
mvpatherror:
	yp_dstrdestroy(&s);

parseparamerror:
	RETURN_BOOL(0);
}

PHP_FUNCTION(ydb_data)
{
	zval *vargs;
	int vargscount;
	
	struct yp_dstring s;
	const char *params[2];

	zend_string *zs;
	char *res;

	vargs = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"+", &vargs, &vargscount) != SUCCESS) {
		yp_seterror(YDB_ERR_WRONGARGS, "Wrong arguments.");
		goto parseparamerror;
	}

	yp_dstrinit(&s);	
	if (yp_mvarpath(vargs, vargscount, &s) < 0)
		goto mvpatherror;


	params[0] = s.data;

	if (yp_coprocreq(&res, "ydb_data", params, 1) < 0)
		goto coprocerror;

	zs = zend_string_init(res, strlen(res), 0);

	free(res);
	yp_dstrdestroy(&s);

	RETURN_STR(zs);

coprocerror:
mvpatherror:
	yp_dstrdestroy(&s);	

parseparamerror:
	RETURN_BOOL(0);
}

PHP_FUNCTION(ydb_order)
{
	zval *vargs;
	int vargscount;
	
	struct yp_dstring s;
	const char *params[2];
	char sdir[1024];

	zval *dir;
	zval ztmp;

	zend_string *zs;
	char *res;

	vargs = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"+z", &vargs, &vargscount, &dir) != SUCCESS) {
		yp_seterror(YDB_ERR_WRONGARGS, "Wrong arguments.");
		goto parseparamerror;
	}

	if (Z_TYPE_P(dir) != IS_LONG) {
		yp_seterror(YDB_ERR_WRONGARGS,
			"Last argument should be a long integer.");
		goto dirparamerror;
	}
	
	yp_dstrinit(&s);	
	if (yp_mvarpath(vargs, vargscount, &s) < 0)
		goto mvpatherror;


	params[0] = s.data;

	ZVAL_COPY_VALUE(&ztmp, dir);
	convert_to_string(&ztmp);

	params[1] = ztmp.value.str->val;

	if (yp_coprocreq(&res, "ydb_order", params, 2) < 0)
		goto coprocerror;

	if (*res == '\0') {
		yp_seterror(YDB_ERR_LASTCLD, "Last index reached.");
		
		goto lastindexerror;
	}

	
	zs = zend_string_init(res, strlen(res), 0);

	free(res);
	zval_dtor(&ztmp);
	yp_dstrdestroy(&s);
	
	RETURN_STR(zs);

lastindexerror:
	free(res);

coprocerror:
	zval_dtor(&ztmp);

mvpatherror:
	yp_dstrdestroy(&s);	

dirparamerror:
parseparamerror:
	RETURN_BOOL(0);
}

PHP_FUNCTION(ydb_error)
{
	RETURN_LONG(yp_errcode);
}

PHP_FUNCTION(ydb_strerror)
{
	zend_string *zs;

	zs = zend_string_init(yp_errmessage, strlen(yp_errmessage), 0);

	RETURN_STR(zs);
}

PHP_FUNCTION(ydb_zsinfo)
{
	char s[YP_MAX_ERRMESSAGE];
	char *p, *err, *id, *sev, *text;	
	zval *zid, *zsev, *ztext;

	// get argument by reference, error handling
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
		"zzz", &zid, &zsev, &ztext) != SUCCESS)
		goto error;
	
	strncpy(s, yp_errmessage, YP_MAX_ERRMESSAGE);

	// ignore error code
	if (strtok(s, ",") == NULL)
		goto error;

	// ignore error position
	if (strtok(NULL, ",") == NULL)
		goto error;
	
	// get error
	if ((err = p = strtok(NULL, ",")) == NULL)
		goto error;
	
	// get text message
	text = p + strlen(p) + 1;
	
	// working with error
	if (*err != '%')
		goto error;
	
	++err;

	// error in format %<FAC>-<SEV>-<ID>
	if (strtok(err, "-") == NULL)
		goto error;
	if ((sev = strtok(NULL, "-")) == NULL)
		goto error;
	if ((id = strtok(NULL, "-")) == NULL)
		goto error;

	// working with message
	while (*text == ' ' || *text == '\n' || *text == '\t')
		++text;

	ZVAL_DEREF(zid);
	convert_to_string(zid);
	ZVAL_STRING(zid, id);

	ZVAL_DEREF(zsev);
	convert_to_string(zsev);
	if (strcmp(sev, "E") == 0) {
		ZVAL_LONG(zsev, YDB_SEVERITY_ERROR);
	}
	else if (strcmp(sev, "F") == 0) {
		ZVAL_LONG(zsev, YDB_SEVERITY_FATAL);
	}
	else if (strcmp(sev, "I") == 0) {
		ZVAL_LONG(zsev, YDB_SEVERITY_INFORMATIONAL);
	}
	else if (strcmp(sev, "S") == 0) {
		ZVAL_LONG(zsev, YDB_SEVERITY_SUCCESS);
	}
	else if (strcmp(sev, "W") == 0) {
		ZVAL_LONG(zsev, YDB_SEVERITY_WARNING);
	}
	else
		goto error;

	ZVAL_DEREF(ztext);
	convert_to_string(ztext);
	ZVAL_STRING(ztext, text);
	
	RETURN_BOOL(1);

error:
	RETURN_BOOL(0);
}
