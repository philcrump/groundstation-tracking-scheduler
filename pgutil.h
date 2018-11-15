#ifndef __PGUTIL_H__
#define __PGUTIL_H__

void pg_transaction_begin(PGconn *conn);
void pg_transaction_commit(PGconn *conn);

#endif /* __PGUTIL_H__ */
