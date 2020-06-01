#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "yp_rpc.h"

int main(int argc, char *argv[])
{	
	FILE *req, *resp, *err;
	struct yp_response rs;
	struct yp_request rq;
	int i;
		
	if (argc < 4) {
		fprintf(stderr, "Not enough arguments.\n");
		return 1;
	}

	if ((req = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Cannot open file %s.\n", argv[1]);
		return 1;
	}
	if ((err = fopen(argv[2], "r")) == NULL) {
		fprintf(stderr, "Cannot open file %s.\n", argv[2]);
		return 1;
	}
	if ((resp = fopen(argv[3], "r")) == NULL) {
		fprintf(stderr, "Cannot open file %s.\n", argv[3]);
		return 1;
	}

	printf("return code: %d\n", yp_readrequest(req, &rq));
	yp_writerequest(&rq, stdout);

	printf("return code: %d\n", yp_readresponse(resp, &rs));
	yp_writeresponse(&rs, stdout);

	printf("return code: %d\n", yp_readresponse(err, &rs));
	yp_writeresponse(&rs, stdout);

	close(req);
	close(resp);
	close(err);

	return 0;
}
