#include <stdio.h>
#include <stdlib.h>

#include "../yp_coproc.h"

int main()
{
	int i;

	for (i = 0; i < 1000000; ++i) {
		const char *env[] = {"gtm_dist=/usr/local/lib/yottadb/r128",
			"gtmgbldir=/home/evgeniy/.yottadb/r1.28_x86_64/g/yottadb.gld",
			"ydb_dir=/home/evgeniy/.yottadb",
			"ydb_rel=r1.28_x86_64"};

		if (yp_coprocinit(env, 4, "/home/evgeniy/ydbphp") < 0) {
			printf("error. errcode = %d, errmessage = \"%s\"\n",
				yp_errcode, yp_errmessage);
		}

		if (yp_coprocfinilize() < 0) {
			printf("error. errcode = %d, errmessage = \"%s\"\n",
				yp_errcode, yp_errmessage);
		}
	}
	/*
	char *res;
	
	const char *env[] = {"gtm_dist=/usr/local/lib/yottadb/r128",
		"gtmgbldir=/home/evgeniy/.yottadb/r1.28_x86_64/g/yottadb.gld",
		"ydb_dir=/home/evgeniy/.yottadb",
		"ydb_rel=r1.28_x86_64"};

	if (yp_coprocinit(env, 4, "/home/evgeniy/ydbphp") < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}

	const char *arg1[] = {"^a(1)", "1213"};
	if (yp_coprocreq(&res, "ydb_set", arg1, 2) < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}

	const char *arg2[] = {"^a(1,2)", "3.14hello"};
	if (yp_coprocreq(&res, "ydb_set", arg2, 2) < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}

	const char *arg22[] = {"^a(1,3)", "0.51"};
	if (yp_coprocreq(&res, "ydb_set", arg22, 2) < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}



	const char *arg3[] = {"^a(1,2)"};
	if (yp_coprocreq(&res, "ydb_get", arg3, 1) < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}
	else
		fprintf(stdout, "%s\n", res);

	const char *arg4[] = {"^a(1,2)"};
	if (yp_coprocreq(&res, "ydb_data", arg4, 1) < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}
	else
		fprintf(stdout, "%s\n", res);
	
	const char *arg5[] = {"^a(1,1)", "1"};
	if (yp_coprocreq(&res, "ydb_order", arg5, 2) < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}
	else
		fprintf(stdout, "%s\n", res);

	const char *arg55[] = {"^a(1,2)", "1"};
	if (yp_coprocreq(&res, "ydb_order", arg55, 2) < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}
	else
		fprintf(stdout, "%s\n", res);

	free(res);

	if (yp_coprocfinilize() < 0) {
		printf("error. errcode = %d, errmessage = \"%s\"\n",
			yp_errcode, yp_errmessage);
	}
	*/

	return 0;
}
