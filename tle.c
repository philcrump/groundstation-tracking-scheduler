#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <libpq-fe.h> 
#include <curl/curl.h>

#include "tle.h"
#include "libpredict/predict.h"

static const char newline[2] = "\n";

void tle_update(PGconn *conn, char *spacetrack_user, char *spacetrack_password)
{
    PGresult *res = PQexec(conn, "SELECT id,name,tle_catalog_id FROM spacecraft WHERE (tle_epoch = timestamp 'epoch' OR tle_updated < NOW()-INTERVAL '15 minutes');");
    if(PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        printf("Error: TLE Update Select failed!\n");
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

        printf(" * Spacecraft: %s\n",PQgetvalue(res, i, 1));

        int row_id = atoi(PQgetvalue(res, i, 0));
        int catalog_id = atoi(PQgetvalue(res, i, 2));

        if(!tle_download(catalog_id, spacetrack_user, spacetrack_password, tle_line[0], tle_line[1]))
        {
          fprintf(stderr, " * * Error: TLE Download failed!\n");
          continue;
        }

        predict_orbital_elements_t *tle;
        tle = predict_parse_tle((char **)tle_lines);

        char sql_stmt[200];
        snprintf(sql_stmt,200
            , "UPDATE spacecraft SET tle_lines_0 = $1, tle_lines_1 = $2, tle_updated = NOW(), tle_epoch = (make_timestamp(%d, 1,1,0,0,0) + interval '%f days') WHERE id=%d;"
            , (2000+tle->epoch_year)
            , (tle->epoch_day-1)
            , row_id
        );
        PGresult *sres = PQexecParams(conn, sql_stmt, 2, NULL, (const char * const*)tle_lines, NULL, NULL, 0);
        if(PQresultStatus(sres) != PGRES_COMMAND_OK)
        {
            fprintf(stderr, " * * Error: TLE Database Update failed! : %s\n", sql_stmt);
        }
        PQclear(sres);
    }

    PQclear(res);
}

typedef struct curl_buffer {
  char *memory;
  size_t size;
} curl_buffer;
 
static size_t curl_buffer_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  curl_buffer *mem = (curl_buffer *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

static bool tle_split(char* input, char *tle_0, char *tle_1)
{
  char *line_ptr;

  line_ptr = strtok(input, newline);
  if(line_ptr == NULL)
  	return false;

  strcpy(tle_0, line_ptr);
  
  line_ptr = strtok(NULL, newline);
  if(line_ptr == NULL)
  	return false;

  strcpy(tle_1, line_ptr);
  return true;
}

static bool tle_extract(char* input, int catalog_id, char *tle_0, char *tle_1)
{
  char catalog_id_string[6];
  snprintf(catalog_id_string, 6, "%05d", catalog_id);

  char *tle_line = strtok(input, newline);

  while (tle_line != NULL)
  {
  	/* Note that either TLE line will match, but we assume we find the top one first! */
    if(0 == strncmp(tle_line+2, catalog_id_string, 5))
    {
      /* Found spacecraft */
      return tle_split(input, tle_0, tle_1);
    }
    tle_line = strtok(NULL, newline);
  }
  /* Failed to find spacecraft */
  return false;
}

bool tle_download(int catalog_id, char *spacetrack_user, char *spacetrack_password, char *tle_0, char *tle_1)
{
  CURL *curl_handle;
  CURLcode res;
  char spacetrack_query[300];
  char postData[400];

  snprintf(spacetrack_query, 300
  	, "https://www.space-track.org/basicspacedata/query/class/tle_latest/NORAD_CAT_ID/%d/ORDINAL/1/EPOCH/%%3Enow-30/format/tle"
  	, catalog_id
  );

  snprintf(postData, 400
  	, "identity=%s&password=%s&query=%s"
  	, spacetrack_user, spacetrack_password
  	, spacetrack_query
  );
 
  curl_buffer tle_buffer;
 
  tle_buffer.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  tle_buffer.size = 0;    /* no data at this point */ 
 
  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "gss/0.1");
  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
   
  curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.space-track.org/ajaxauth/login");

  curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postData);

  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_buffer_cb);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&tle_buffer);
 
  res = curl_easy_perform(curl_handle);

  curl_easy_cleanup(curl_handle);
  curl_global_cleanup();
 
  if(res != CURLE_OK)
  {
    fprintf(stderr, " * * * Error: Curl failed with error: %s\n",
      curl_easy_strerror(res));

    free(tle_buffer.memory);
    return false;
  }

  /* Note that checking the HTTP code doesn't catch an incorrect password with Celestrak! */
  long http_code = 0;
  curl_easy_getinfo (curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
  if(http_code != 200)
  {
    fprintf(stderr, " * * * Error: Download failed with HTTP status: %ld\n",
      http_code);

    free(tle_buffer.memory);
    return false;
  }

  tle_buffer.memory[tle_buffer.size]='\0';
    fprintf(stderr,"%s",tle_buffer.memory);
  if(!tle_extract(tle_buffer.memory, catalog_id, tle_0, tle_1))
  {
    fprintf(stderr, " * * * Error: Extracting TLE from HTTP response failed\n");
    fprintf(stderr, "        Note: SpaceTrack User/Password may be incorrect!\n");

    free(tle_buffer.memory);
    return false;
  }
 
  free(tle_buffer.memory);
  return true;
}

