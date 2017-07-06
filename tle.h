#ifndef __TLE_DOWNLOAD_H__
#define __TLE_DOWNLOAD_H__

void tle_update(PGconn *conn, char *spacetrack_user, char *spacetrack_password);

bool tle_download(int catalog_id, char *spacetrack_user, char *spacetrack_password, char *tle_0, char *tle_1);

#endif /* __TLE_DOWNLOAD_H__ */
