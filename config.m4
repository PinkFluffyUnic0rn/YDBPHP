PHP_ARG_ENABLE(ydbphp, whether to enable ydbphp support,
[ --enable-ydbphp   Enable ydbphp support])

if test "$PHP_YDBPHP" = "yes"; then
	LIBNAME=yottadb
	LIBSYMBOL=ydb_init
	YOTTADB_LIBS=$(pkg-config --libs yottadb)
	YOTTADB_INC=$(pkg-config --cflags yottadb)
	YDBPHP_SOURCES="$YDBPHP_SOURCES yp_coproc.c yp_rpc.c yp_dstring.c"

	PHP_CHECK_LIBRARY($LIBNAME, $LIBSYMBOL,
		[
			PHP_EVAL_INCLINE($YOTTADB_INC) 
			PHP_EVAL_LIBLINE($YOTTADB_LIBS, YDBPHP_SHARED_LIBADD) 
		],
		[
			AC_MSG_ERROR([yottadb not found. Check config.log])
		],
		[
			$YOTTADB_LIBS
		]
	)

	AC_DEFINE(HAVE_YDBPHP, 1, [Whether you have ydbphp])
	PHP_NEW_EXTENSION(ydbphp, ydbphp.c $YDBPHP_SOURCES, $ext_shared)
	PHP_SUBST(YDBPHP_SHARED_LIBADD)
	
fi
