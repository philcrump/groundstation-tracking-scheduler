#ifndef __PGUTIL_H__
#define __PGUTIL_H__

#include <stdint.h>

int bufconcat(char *a, int a_length, char *b, int b_length);

void pg_transaction_begin(PGconn *conn);
void pg_transaction_commit(PGconn *conn);

#endif /* __PGUTIL_H__ */
