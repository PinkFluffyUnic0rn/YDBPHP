<?php

for ($i = 0; $i < 100000; ++$i) {
	ydb_init(
		"gtm_dist=/usr/local/lib/yottadb/r128",
		"gtmgbldir=/home/evgeniy/.yottadb/r1.28_x86_64/g/yottadb.gld",
		"ydb_dir=/home/evgeniy/.yottadb",
		"ydb_rel=r1.28_x86_64",
		"gtmroutines=/home/evgeniy/ydbtest"
	);
	
	ydb_data("^a", 24, "asdf");

	ydb_set("^a", 24, "asdf", "hello");
	ydb_set("^a", 24, "asdf", 1, "412");
	ydb_set("^a", 24, "asdf", 2, "1.4422");
	ydb_set("^a", 24, "asdf", 3, "True");
	ydb_set("^a", 24, "asdf", 4, "False");

	ydb_get("^a", array(24, "asdf"), 1);

	ydb_get("^a", 24, "asdf");
	ydb_data("^a", 24, "asdf");

	ydb_gettyped("^a", 24, "asdf");
	ydb_gettyped("^a", 24, "asdf", 1);
	ydb_gettyped("^a", 24, "asdf", 2);
	ydb_gettyped("^a", 24, "asdf", 3);
	ydb_gettyped("^a", 24, "asdf", 4);

	ydb_order("^a", 24, "asdf", 1, 1);
	ydb_order("^a", 24, "asdf", 2, 1);
	ydb_order("^a", 24, "asdf", 3, 1);
	ydb_order("^a", 24, "asdf", 4, 1);


	ydb_kill("^a", 24, "asdf");
	ydb_data("^a", 24, "asdf");

	ydb_get("^a", 24, "asdf");
	
	$id = $sev = $text = "1";
	ydb_error();
	ydb_strerror();
	ydb_zsinfo($id, $sev, $text);

	ydb_gettyped("^a", 24, "asdf");

	ydb_finilize();
}

printf("initilizing: %s\n\n", ydb_init(
	"gtm_dist=/usr/local/lib/yottadb/r128",
	"gtmgbldir=/home/evgeniy/.yottadb/r1.28_x86_64/g/yottadb.gld",
	"ydb_dir=/home/evgeniy/.yottadb",
	"ydb_rel=r1.28_x86_64",
	"gtmroutines=/home/evgeniy/ydbtest"
));

printf("data ^a(24,\"asdf\") : %s\n\n", ydb_data("^a", 24, "asdf"));

printf("setting ^a(24,\"asdf\")=\"hello\" : %s\n", ydb_set("^a", 24, "asdf", "hello"));
printf("setting ^a(24,\"asdf\",1)=\"412\" : %s\n", ydb_set("^a", 24, "asdf", 1, "412"));
printf("setting ^a(24,\"asdf\",2)=\"1.4422\" : %s\n", ydb_set("^a", 24, "asdf", 2, "1.4422"));
printf("setting ^a(24,\"asdf\",3)=\"True\" : %s\n\n", ydb_set("^a", 24, "asdf", 3, "True"));
printf("setting ^a(24,\"asdf\",4)=\"False\" : %s\n\n", ydb_set("^a", 24, "asdf", 4, "False"));

printf("getting with array ^a(24,\"asdf\",1) : %s\n", ydb_get("^a", array(24, "asdf"), 1));

printf("getting ^a(24,\"asdf\") : %s\n", ydb_get("^a", 24, "asdf"));
printf("data ^a(24,\"asdf\") : %s\n\n", ydb_data("^a", 24, "asdf"));

printf("getting typed ^a(24,\"asdf\") : ");
var_dump(ydb_gettyped("^a", 24, "asdf"));
printf("getting typed ^a(24,\"asdf\", 1) : ");
var_dump(ydb_gettyped("^a", 24, "asdf", 1));
printf("getting typed ^a(24,\"asdf\", 2) : ");
var_dump(ydb_gettyped("^a", 24, "asdf", 2));
printf("getting typed ^a(24,\"asdf\", 3) : ");
var_dump(ydb_gettyped("^a", 24, "asdf", 3));
printf("getting typed ^a(24,\"asdf\", 4) : ");
var_dump(ydb_gettyped("^a", 24, "asdf", 4));

printf("order ^a(24,\"asdf\", 1) : %s\n", ydb_order("^a", 24, "asdf", 1, 1));
printf("order ^a(24,\"asdf\", 2) : %s\n", ydb_order("^a", 24, "asdf", 2, 1));
printf("order ^a(24,\"asdf\", 3) : %s\n", ydb_order("^a", 24, "asdf", 3, 1));
printf("order ^a(24,\"asdf\", 4) : %s\n\n", ydb_order("^a", 24, "asdf", 4, 1));

printf("ydb_error: %d\n", ydb_error());
printf("ydb_strerror: %s\n\n", ydb_strerror());

printf("killing ^a(24,\"asdf\") : %s\n", ydb_kill("^a", 24, "asdf"));
printf("data ^a(24,\"asdf\") : %s\n\n", ydb_data("^a", 24, "asdf"));

printf("getting ^a(24,\"asdf\") : ");
if (ydb_get("^a", 24, "asdf") === FALSE)
	printf("got FALSE\n");
else
	printf("\n");

$id = $sev = $text = "1";
printf("ydb_error: %d\n", ydb_error());
printf("ydb_strerror: %s\n", ydb_strerror());
printf("ydb_zsinfo: %s\n", ydb_zsinfo($id, $sev, $text));
printf("id: %s\nsev: %s\ntext: %s\n\n", $id, $sev, $text);

printf("YDB_SEVERITY_ERROR: %d
YDB_SEVERITY_FATAL: %d
YDB_SEVERITY_INFORMATIONAL: %d
YDB_SEVERITY_SUCCESS: %d
YDB_SEVERITY_WARNING: %d\n\n",
	YDB_SEVERITY_ERROR,
	YDB_SEVERITY_FATAL,
	YDB_SEVERITY_INFORMATIONAL,
	YDB_SEVERITY_SUCCESS,
	YDB_SEVERITY_WARNING);

printf("getting typed ^a(24,\"asdf\") : ");
if (ydb_gettyped("^a", 24, "asdf") === NULL)
	printf("got NULL\n\n");
else
	printf("\n\n");

printf("finilizing: %s\n", ydb_finilize());

?>
