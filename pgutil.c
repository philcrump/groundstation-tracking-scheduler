#include <libpq-fe.h>

/* Fast concatenation of char buffers (warning: no bounds checking) */
/*  - Returns new length of buffer 'a' */
int bufconcat(char *a, int a_length, char *b, int b_length)
{
  int i;
  for(i=0; i < b_length; i++)
  {
    a[a_length + i] = b[i];
  }
  return (a_length + b_length);
}

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