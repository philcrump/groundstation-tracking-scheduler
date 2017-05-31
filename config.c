#include <unistd.h>
#include <string.h>

#include "config.h"
#include "minIni.h"

bool config_load(config_t *config, char *filename)
{
    if(access(filename, F_OK) < 0)
    {
        return false;
    }

    ini_gets(
        "Database", "host", "unix",
        config->db_host, sizeofarray(config->db_host),
        filename
    );
    ini_gets(
        "Database", "database", "gss",
        config->db_database, sizeofarray(config->db_database),
        filename
    );
    ini_gets(
        "Database", "user", "",
        config->db_user, sizeofarray(config->db_user),
        filename
    );
    ini_gets(
        "Database", "password", "",
        config->db_passwd, sizeofarray(config->db_passwd),
        filename
    );

    ini_gets("Spacetrack", "user", "",
        config->spacetrack_user, sizeofarray(config->spacetrack_user),
        filename);
    ini_gets("Spacetrack", "password", "",
        config->spacetrack_passwd, sizeofarray(config->spacetrack_passwd),
        filename
    );

    config->latitude = ini_getf(
        "Tracking","latitude", 0.0,
        filename
    );
    config->longitude = ini_getf(
        "Tracking","longitude", 0.0, 
        filename
    );
    config->altitude = ini_getf(
        "Tracking","altitude", 0.0,
        filename
    );

    config->azimuth_endstop = ini_getf(
        "Tracking","azimuth_endstop", 0.0,
        filename
    );
    config->azimuth_rate = ini_getf(
        "Tracking","azimuth_rate", 1.0,
        filename
    );
    config->elevation_rate = ini_getf(
        "Tracking","elevation_rate", 1.0,
        filename
    );
    config->minimum_interval = ini_getf(
        "Tracking","minimum_interval", 0.5,
        filename
    );
    config->minimum_period = ini_getf(
        "Tracking","minimum_period", 10.0,
        filename
    );

    config->db_conn_string[0] = '\0';
    if(strcmp(config->db_host, "unix") != 0)
    {
        snprintf(config->db_conn_string,
            sizeofarray(config->db_conn_string),
            "host=%s dbname=%s user=%s password=%s",
            config->db_host,
            config->db_database,
            config->db_user,
            config->db_passwd
        );
    }
    else
    {
        snprintf(config->db_conn_string,
            sizeofarray(config->db_conn_string),
            "dbname=%s user=%s",
            config->db_database,
            config->db_user
        );
    }

    return true;
}
