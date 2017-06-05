#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <libpq-fe.h>

#include "config.h"
#include "tle.h"
#include "time.h"
#include "libpredict/predict.h"

void track_update_all(PGconn *conn)
{
    PGresult *res = PQexec(conn, "SELECT id,name,tle_lines_0,tle_lines_1 FROM spacecraft WHERE tle_epoch != timestamp 'epoch' AND track_epoch < tle_epoch AND track_updated < NOW()-INTERVAL '1 day' ORDER BY id ASC;");
    if(PQresultStatus(res) != PGRES_TUPLES_OK)
    {   
        printf("Error: Track Update Select query failed!\n");
        return;
    }
    
    int rows = PQntuples(res);
    if(rows == 0)
    {
        printf(" * Up to date\n");
        PQclear(res);
        return;
    }
    for(int i=0; i<rows; i++) {
        char tle_line[2][150];
        char *tle_lines[2];
        tle_lines[0] = &tle_line[0][0];
        tle_lines[1] = &tle_line[1][0];

	int spacecraft_id = atoi(PQgetvalue(res, i, 0));
        printf(" * %s\n",PQgetvalue(res, i, 1));
        strncpy(tle_lines[0],PQgetvalue(res, i, 2),150);
        strncpy(tle_lines[1],PQgetvalue(res, i, 3),150);

        predict_orbital_elements_t *tle = predict_parse_tle((char **)tle_lines);
	struct predict_orbit orbit;
        predict_observer_t *observer;
        observer = predict_create_observer("Self", config.latitude*3.14159/180.0, config.longitude*3.14159/180.0, config.altitude);
        struct predict_observation observation;

	uint64_t curr_timestamp_ms = timestamp_ms();
        uint64_t calc_timestamp_ms;
	predict_julian_date_t calc_datetime;
        PGresult *calc_res;

        /* Begin SQL Transaction */
        calc_res = PQexec(conn,"BEGIN");
        if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
        {
            printf("Error: Transaction Begin failed!\n");
        }
        PQclear(calc_res);

        char sql_stmt[450];
        for(int j=0; j<(60*60*24*7); j++)
        {
            calc_timestamp_ms = curr_timestamp_ms + (1000 * j);
            calc_datetime = julian_from_timestamp_ms(calc_timestamp_ms);

            predict_orbit(tle, &orbit, calc_datetime);
            sprintf( sql_stmt
                , "INSERT into positions VALUES ((TIMESTAMP 'epoch' + INTERVAL '%ld seconds'),%d,(%f,%f,%f),(%f,%f,%f)) ON CONFLICT (spacecraft, time) DO UPDATE SET position = excluded.position, velocity = excluded.velocity;"
                 , calc_timestamp_ms/1000
                 , spacecraft_id
                 , orbit.position[0], orbit.position[1], orbit.position[2]
                 , orbit.velocity[0], orbit.velocity[1], orbit.velocity[2]
            );
            calc_res = PQexec(conn,sql_stmt);
            if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
            {
                printf("Error: Position insert failed! : %s\n", sql_stmt);
            }
            PQclear(calc_res);
	
            predict_observe_orbit(observer, &orbit, &observation);
	    sprintf( sql_stmt
                , "INSERT into observations VALUES ((TIMESTAMP 'epoch' + INTERVAL '%ld seconds'),%d, %f,%f, %f,%f, %f) ON CONFLICT (spacecraft, time) DO UPDATE SET azimuth = excluded.azimuth, azimuth_rate = excluded.azimuth_rate, elevation = excluded.elevation, elevation_rate = excluded.elevation_rate;"
                 , calc_timestamp_ms/1000
                 , spacecraft_id
                 , observation.azimuth, observation.azimuth_rate
                 , observation.elevation, observation.elevation_rate
                 , (observation.range_rate*1000)
            );
            //printf("%s\n",sql_stmt);
            calc_res = PQexec(conn,sql_stmt);
            if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
            {
                printf("Error: Observation insert failed! : %s\n", sql_stmt);
            }
            PQclear(calc_res);
            
        }
        calc_res = PQexec(conn, "DELETE FROM positions WHERE time < NOW();");
        PQclear(calc_res);

        calc_res = PQexec(conn, "DELETE FROM observations WHERE time < NOW();");
        PQclear(calc_res);

        sprintf(sql_stmt
            , "UPDATE spacecraft SET track_updated = NOW(), track_epoch = tle_epoch WHERE id=%d;"
            , spacecraft_id
        );
        calc_res = PQexec(conn, sql_stmt);
        if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
        {
            printf("Error: Spacecraft Track Update failed! : %s\n", sql_stmt);
        }

        calc_res = PQexec(conn,"COMMIT");
        if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
        {
            printf("Error: Transaction Commit failed!\n");
        }
        PQclear(calc_res);

    }

    PQclear(res);
}    
