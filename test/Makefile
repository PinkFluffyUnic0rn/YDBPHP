all:
	gcc -Wall coproc_main.c yp_coproc.c rpc/yp_rpc.c rpc/yp_dstring.c \
		`pkg-config --libs --cflags yottadb` -o coproc
	gcc coproc_main.c yp_rpc.c yp_dstring.c -o yp_rpc
