#ifndef __TLE_DOWNLOAD_H__
#define __TLE_DOWNLOAD_H__

void tle_update_http(PGconn *conn);
void tle_update_spacetrack(PGconn *conn, char *user, char *password);

int tle_download_http(char *url, char *craft_uri, char *tle_0, char *tle_1);
int tle_download_spacetrack(char *url, char *user, char *password, char *craft_uri, char *tle_0, char *tle_1);

#endif /* __TLE_DOWNLOAD_H__ */
