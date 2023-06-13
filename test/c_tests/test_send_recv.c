#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

void exit_nicely(PGconn *conn) {
	PQfinish(conn);
	exit(1);
}

int main() {
	/* Connect to database specified by the PGDATABASE environment variable */
	const char	*conninfo = "host=localhost port=5432";
	PGconn		*conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK) {
		fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
		exit_nicely(conn);
	}

	/* Create extension */
	PQexec(conn, "CREATE EXTENSION IF NOT EXISTS hashset");

	/* Create temporary table */
	PQexec(conn, "CREATE TABLE IF NOT EXISTS test_hashset_send_recv (hashset_col hashset)");

	/* Enable binary output */
	PQexec(conn, "SET bytea_output = 'escape'");

	/* Insert dummy data */
	const char *insert_command = "INSERT INTO test_hashset_send_recv (hashset_col) VALUES ('{1,2,3}'::hashset)";
	PGresult *res = PQexec(conn, insert_command);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	/* Fetch the data in binary format */
	const char *select_command = "SELECT hashset_col FROM test_hashset_send_recv";
	int resultFormat = 1;  /* 0 = text, 1 = binary */
	res = PQexecParams(conn, select_command, 0, NULL, NULL, NULL, NULL, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	/* Store binary data for later use */
	const char *binary_data = PQgetvalue(res, 0, 0);
	int binary_data_length = PQgetlength(res, 0, 0);
	PQclear(res);

	/* Re-insert the binary data */
	const char *insert_binary_command = "INSERT INTO test_hashset_send_recv (hashset_col) VALUES ($1)";
	const char *paramValues[1] = {binary_data};
	int paramLengths[1] = {binary_data_length};
	int paramFormats[1] = {1}; /* binary format */
	res = PQexecParams(conn, insert_binary_command, 1, NULL, paramValues, paramLengths, paramFormats, 0);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		fprintf(stderr, "INSERT failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	/* Check the data */
	const char *check_command = "SELECT COUNT(DISTINCT hashset_col::text) AS unique_count, COUNT(*) FROM test_hashset_send_recv";
	res = PQexec(conn, check_command);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "SELECT failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}

	/* Print the results */
	printf("unique_count: %s\n", PQgetvalue(res, 0, 0));
	printf("count: %s\n", PQgetvalue(res, 0, 1));
	PQclear(res);

	/* Disconnect */
	PQfinish(conn);

	return 0;
}
