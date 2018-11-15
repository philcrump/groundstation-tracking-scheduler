#include <libpq-fe.h>

/* Begin SQL Transaction */
void pg_transaction_begin(PGconn *conn)
{
  PGresult *res;
  res = PQexec(conn,"BEGIN");
  if(PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    printf("Error: Transaction Begin failed!\n");
  }
  PQclear(res);
}

/* Commit SQL Transaction */
void pg_transaction_commit(PGconn *conn)
{
  PGresult *res;
  res = PQexec(conn,"COMMIT");
  if(PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    printf("Error: Transaction Commit failed!\n");
  }
  PQclear(res);
}