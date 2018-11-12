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
#include "pgutil.h"

#define SQL_BATCH_BUFFER_SIZE   1048576

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
    printf(" * * Up to date\n");
    PQclear(res); 
    return;
  }
  for(int i=0; i<rows; i++)
  {
    char tle_line[2][150];
    char *tle_lines[2];
    tle_lines[0] = &tle_line[0][0];
    tle_lines[1] = &tle_line[1][0];

    int spacecraft_id = atoi(PQgetvalue(res, i, 0));
    printf(" * %s\n",PQgetvalue(res, i, 1));
    strncpy(tle_lines[0],PQgetvalue(res, i, 2),150);
    strncpy(tle_lines[1],PQgetvalue(res, i, 3),150);

    predict_orbital_elements_t tle;
    struct predict_sgp4 sgp;
    struct predict_sdp4 sdp;
    struct predict_position orbit;
    predict_observer_t observer;
    struct predict_observation observation;

    if(!predict_parse_tle(&tle, tle_lines[0], tle_lines[1], &sgp, &sdp))
    {
      fprintf(stderr, " * * Error parsing TLE\n");
      PQclear(res); 
      return;
    }

    predict_create_observer(&observer, "Self", config.latitude*3.14159/180.0, config.longitude*3.14159/180.0, config.altitude);

    uint64_t curr_timestamp_s = timestamp_s();
    uint64_t calc_timestamp_s;
    predict_julian_date_t calc_datetime;
    PGresult *calc_res;

    char line_sql_buffer[250];
    int line_sql_length;

    char batch_sql_buffer[SQL_BATCH_BUFFER_SIZE];
    int batch_sql_length;

    printf(" * * Calculating positions..\n");

    char *position_sql_start = "INSERT into positions VALUES";
    int position_sql_start_length = 28;
    char *position_sql_end = " ON CONFLICT (spacecraft, time) DO UPDATE SET position = excluded.position, velocity = excluded.velocity;";
    int position_sql_end_length = 105;

    strcpy(batch_sql_buffer, position_sql_start);
    batch_sql_length = position_sql_start_length;

    pg_transaction_begin(conn);

    for(int j=0; j<(60*60*24*7); j++)
    {
      calc_timestamp_s = curr_timestamp_s + j;
      calc_datetime = julian_from_timestamp_ms(calc_timestamp_s * 1000);

      predict_orbit(&tle, &orbit, calc_datetime);
      line_sql_length = sprintf(
          line_sql_buffer
          ,"((TIMESTAMP 'epoch' + INTERVAL '%ld seconds'),%d,(%f,%f,%f),(%f,%f,%f)), "
          , calc_timestamp_s
          , spacecraft_id
          , orbit.position[0], orbit.position[1], orbit.position[2]
          , orbit.velocity[0], orbit.velocity[1], orbit.velocity[2]
      );
      if(batch_sql_length + line_sql_length + 105 + 2 + 1 >= SQL_BATCH_BUFFER_SIZE) // TODO check length calcs
      {
        /* Cut trailing comma + space off */
        batch_sql_buffer[batch_sql_length - 2] = '\0';

        bufconcat(batch_sql_buffer, batch_sql_length, position_sql_end, position_sql_end_length);

        calc_res = PQexec(conn,batch_sql_buffer);
        if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
        {
            printf("Error: Position batch insert failed! : %s\n", batch_sql_buffer);
        }
        PQclear(calc_res);

        strcpy(batch_sql_buffer, position_sql_start);
        batch_sql_length = position_sql_start_length;
      }
      else
      {
        batch_sql_length = bufconcat(batch_sql_buffer, batch_sql_length, line_sql_buffer, line_sql_length);
      }
    }

    if(batch_sql_length > position_sql_start_length)
    {
      /* Cut trailing comma + space off */
      batch_sql_buffer[batch_sql_length - 2] = '\0';

      bufconcat(batch_sql_buffer, batch_sql_length, position_sql_end, position_sql_end_length);

      calc_res = PQexec(conn,batch_sql_buffer);
      if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
      {
          printf("Error: Final Position batch insert failed! : %s\n", batch_sql_buffer);
      }
      PQclear(calc_res);
    }

    printf(" * * * Done.\n");
    printf(" * * Calculating observations..\n");

    char *observation_sql_start = "INSERT into observations VALUES";
    int observation_sql_start_length = 31;
    char *observation_sql_end = " ON CONFLICT (spacecraft, time) DO UPDATE SET azimuth = excluded.azimuth, azimuth_rate = excluded.azimuth_rate, elevation = excluded.elevation, elevation_rate = excluded.elevation_rate;";
    int observation_sql_end_length = 185;

    strcpy(batch_sql_buffer, observation_sql_start);
    batch_sql_length = observation_sql_start_length;

    for(int j=0; j<(60*60*24*7); j++)
    {
      calc_timestamp_s = curr_timestamp_s + j;
      calc_datetime = julian_from_timestamp_ms(calc_timestamp_s * 1000);

      predict_orbit(&tle, &orbit, calc_datetime);
      predict_observe_orbit(&observer, &orbit, &observation);

      line_sql_length = sprintf(
            line_sql_buffer
            , "((TIMESTAMP 'epoch' + INTERVAL '%ld seconds'),%d, %f,%f, %f,%f, %f), "
            , calc_timestamp_s
            , spacecraft_id
            , observation.azimuth, observation.azimuth_rate
            , observation.elevation, observation.elevation_rate
            , (observation.range_rate*1000)
      );
      if(batch_sql_length + line_sql_length + 185 + 2 + 1 >= SQL_BATCH_BUFFER_SIZE) // TODO check length calcs
      {
        /* Cut trailing comma + space off */
        batch_sql_buffer[batch_sql_length - 2] = '\0';

        bufconcat(batch_sql_buffer, batch_sql_length, observation_sql_end, observation_sql_end_length);

        calc_res = PQexec(conn,batch_sql_buffer);
        if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
        {
            printf("Error: Observation batch insert failed! : %s\n", batch_sql_buffer);
        }
        PQclear(calc_res);

        strcpy(batch_sql_buffer, observation_sql_start);
        batch_sql_length = observation_sql_start_length;
      }
      else
      {
        batch_sql_length = bufconcat(batch_sql_buffer, batch_sql_length, line_sql_buffer, line_sql_length);
      }
    }

    if(batch_sql_length > observation_sql_start_length)
    {
      /* Cut trailing comma + space off */
      batch_sql_buffer[batch_sql_length - 2] = '\0';

      bufconcat(batch_sql_buffer, batch_sql_length, observation_sql_end, observation_sql_end_length);

      calc_res = PQexec(conn,batch_sql_buffer);
      if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
      {
          printf("Error: Final Observation batch insert failed! : %s\n", batch_sql_buffer);
      }
      PQclear(calc_res);
    }

    printf(" * * * Done.\n");

    calc_res = PQexec(conn, "DELETE FROM positions WHERE time < NOW();");
    PQclear(calc_res);

    calc_res = PQexec(conn, "DELETE FROM observations WHERE time < NOW();");
    PQclear(calc_res);

    char sql_stmt[200];
    sprintf(sql_stmt
        , "UPDATE spacecraft SET track_updated = NOW(), track_epoch = tle_epoch WHERE id=%d;"
        , spacecraft_id
    );
    calc_res = PQexec(conn, sql_stmt);
    if(PQresultStatus(calc_res) != PGRES_COMMAND_OK)
    {
      printf("Error: Spacecraft Track Update failed! : %s\n", sql_stmt);
    }

    pg_transaction_commit(conn);
  }

  PQclear(res);
}    
