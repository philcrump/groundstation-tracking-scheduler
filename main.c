#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <libpq-fe.h>

#include "config.h"
#include "tle.h"
#include "schedule.h"
#include "track.h"
#include "libpredict/predict.h"

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

int main(int argc, char *argv[])
{
    fprintf(stdout,
        "gs-scheduler "BUILD_VERSION" (built "BUILD_DATE")\n"
        "Phil Crump 2018\n"
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

    printf("Updating TLEs from celestrak..\n");
    tle_update_http(conn);
    
    printf("Updating TLEs from spacetrack..\n");
    tle_update_spacetrack(conn, config.spacetrack_user, config.spacetrack_passwd);

    printf("Updating Tracks..\n");
    track_update_all(conn); 

    printf("Updating Schedule..\n");
    schedule_update_all(conn);
    
    PQfinish(conn);
    
    return 0;
}
