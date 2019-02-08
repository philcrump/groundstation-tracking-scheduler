#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <libpq-fe.h>

#include "config.h"
#include "time.h"
#include "tle.h"
#include "libpredict/predict.h"

#define RADIANS_FROM_DEGREES(x)   (x*(3.1415926536/180.0))

void schedule_update_all(PGconn *conn)
{
    PGresult *res = PQexec(conn, "SELECT id FROM spacecraft WHERE (SELECT updated FROM schedule_meta) < (SELECT MAX(track_updated) FROM spacecraft) ORDER BY priority ASC;");
    if(PQresultStatus(res) != PGRES_TUPLES_OK)
    { 
        printf("Error: Schedule Update Select query failed!\n");
        printf("%s\n", PQresStatus(PQresultStatus(res)));
        printf("%s\n", PQresultErrorMessage(res));
        return;
    }

    int rows = PQntuples(res);
    if(rows == 0)
    {
        printf(" * Up to date\n");
        PQclear(res);
        return;
    }
    printf(" * Running update..\n");

    PGresult *cmd_res = PQexec(conn,"BEGIN;");
    if(PQresultStatus(cmd_res) != PGRES_COMMAND_OK)
    {
        printf("Error: Schedule Transaction Begin failed!\n");
        PQclear(res);
        return;
    }
    PQclear(cmd_res);

    cmd_res = PQexec(conn,"DELETE from schedule;");
    if(PQresultStatus(cmd_res) != PGRES_COMMAND_OK)
    {
        printf("Error: Schedule Clear failed!\n");
        PQclear(res);
        return;
    }
    PQclear(cmd_res);

    char visible_query[200];
    char schedule_insert[200];
    for(int i=0; i<rows; i++)
    {
        int spacecraft_id = atoi(PQgetvalue(res,i,0));
        snprintf(visible_query, 200
            , "SELECT time FROM observations WHERE spacecraft=%d AND elevation>%f ORDER BY TIME ASC;"
            , spacecraft_id
            , RADIANS_FROM_DEGREES(config.minimum_elevation)
        );
        PGresult *spacecraft_visible = PQexec(conn,visible_query);
        if(PQresultStatus(spacecraft_visible) != PGRES_TUPLES_OK)
        {
            printf("Error: Spacecraft Visibility Select query failed!\n");
            continue;
        }
        int visible_rows = PQntuples(spacecraft_visible);
        for(int j=0; j<visible_rows; j++)
        {
            snprintf(schedule_insert,200
                , "INSERT INTO schedule VALUES ('%s', %d, 'tracking') ON CONFLICT DO NOTHING;"
                , PQgetvalue(spacecraft_visible,j,0)
                , spacecraft_id
            );
            cmd_res = PQexec(conn, schedule_insert);
            if(PQresultStatus(cmd_res) != PGRES_COMMAND_OK)
            {
                printf("Error: Schedule Insertion failed! : %s\n"
                    , schedule_insert
                );
            }
            PQclear(cmd_res);
            //if((timestamp - last_row_timestamp) > 1)
            //{
                /* Add return-to-zenith on end of last_row_timestamp
                 , add align-to-track on this timestamp */
            //}
        }
        PQclear(spacecraft_visible);
    }

    cmd_res = PQexec(conn, "UPDATE schedule_meta SET updated=NOW();");
    if(PQresultStatus(cmd_res) != PGRES_COMMAND_OK)
    {
        printf("Error: Schedule meta Update failed!\n");
    }
    PQclear(cmd_res);

    cmd_res = PQexec(conn, "COMMIT;");
    if(PQresultStatus(cmd_res) != PGRES_COMMAND_OK)
    {
        printf("Error: Schedule Transaction Commit failed!\n");
    }
    PQclear(cmd_res);

    PQclear(res);
}
