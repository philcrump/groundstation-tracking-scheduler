#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <libpq-fe.h>

#include "../config.h"

void _print_usage(void)
{
    printf(
        "\n"
        "Usage: gss [options]\n"
        "\n"
        "  -V, --version          Print version string and exit.\n"
        "\n"
    );
}

void print_next_point(PGconn *conn)
{
    PGresult *res = PQexec(conn
        , "SELECT schedule.time, spacecraft.name, round(degrees(observations.azimuth)), round(degrees(observations.elevation)), spacecraft.frequency_downlink-(spacecraft.frequency_downlink*(observations.relative_velocity/3e8)) AS downlink, spacecraft.frequency_uplink-(spacecraft.frequency_uplink*(observations.relative_velocity/3e8)) AS uplink FROM (SELECT * FROM schedule WHERE time > NOW() ORDER BY time LIMIT 1) AS schedule INNER JOIN observations ON schedule.spacecraft=observations.spacecraft AND schedule.time=observations.time INNER JOIN spacecraft ON schedule.spacecraft=spacecraft.id;");
    if(PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Error: Next Point Select query failed!\n");
        printf("%s\n", PQresStatus(PQresultStatus(res)));
        printf("%s\n", PQresultErrorMessage(res));
        return;
    }

    int rows = PQntuples(res);
    if(rows == 0)
    {
        printf("Error: No Next point found\n");
        PQclear(res);
        return;
    }
    printf("%s: %d, %d, [%s] Downlink: %.6f MHz, Uplink: %.6f MHz\n"
        , PQgetvalue(res,0,0)
        , atoi(PQgetvalue(res,0,2))
        , atoi(PQgetvalue(res,0,3))
        , PQgetvalue(res,0,1)
        , atof(PQgetvalue(res,0,4))
        , atof(PQgetvalue(res,0,5))
    );
    PQclear(res);
}

int main(int argc, char *argv[])
{
    fprintf(stdout,
        "gs-scheduler "BUILD_VERSION" (built "BUILD_DATE")\n"
        "Phil Crump 2017\n"
    );
    
    int opt, c;
    static const struct option long_options[] = {
        { "version",     no_argument,       0, 'V' },
        { "config",      required_argument, 0, 'c' },
        { 0,             0,                 0,  0  }
    };

    char config_filename[100];
    strncpy(config_filename, "config.ini", 100);

    while((c = getopt_long(argc, argv, "Vc:", long_options, &opt)) != -1)
    {
        switch(c)
        {
            case 'c':
                strncpy(config_filename, optarg, 100); 
                break;
            case '?':
            default:
                _print_usage();
                return 0;
        }
    }

    printf("Loading config from file: %s\n", config_filename);
    if(!config_load(&config, config_filename))
    {
        printf("Error loading config from file! Exiting..\n");
        return 1;
    }

    PGconn *conn = PQconnectdb(config.db_conn_string);

    if (PQstatus(conn) == CONNECTION_BAD) {
        
        fprintf(stderr, "Connection to database failed: %s\n",
            PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    printf("Connected to Server (server: %d, client: %d)\n"
        , PQserverVersion(conn), PQlibVersion()
    );

    while(1)
    {
        print_next_point(conn);
        sleep(1);
    }
 
    PQfinish(conn);
    
    return 0;
}
