DROP DATABASE IF EXISTS "gs-scheduler";
DROP USER IF EXISTS phil;

CREATE DATABASE "gs-scheduler";
CREATE USER phil;

\c "gs-scheduler";

CREATE TYPE tle_source_enum AS ENUM ('http', 'spacetrack');

CREATE TABLE spacecraft
(
  id serial NOT NULL,
  name character varying(250) NOT NULL,
  priority integer NOT NULL,
  tle_source_type tle_source_enum,
  tle_source_url character varying(1024),
  tle_spacecraft_uri character varying(250),
  tle_lines_0 character varying(250),
  tle_lines_1 character varying(250),
  tle_updated timestamp without time zone NOT NULL DEFAULT TIMESTAMP 'epoch',
  tle_epoch timestamp without time zone NOT NULL DEFAULT TIMESTAMP 'epoch',
  track_updated timestamp without time zone NOT NULL DEFAULT TIMESTAMP 'epoch',
  track_epoch timestamp without time zone NOT NULL DEFAULT TIMESTAMP 'epoch',
  CONSTRAINT pkey_spacecraft PRIMARY KEY (id)
);

CREATE TYPE eci_vector AS (
  x double precision,
  y double precision,
  z double precision
);

CREATE TABLE positions
(
  time timestamp without time zone NOT NULL,
  spacecraft integer NOT NULL,
  position eci_vector NOT NULL,
  velocity eci_vector NOT NULL,
  FOREIGN KEY (spacecraft) REFERENCES spacecraft (id) ON DELETE CASCADE,
  UNIQUE (spacecraft, time)
);

CREATE TABLE observations
(
  time timestamp without time zone NOT NULL,
  spacecraft integer NOT NULL,
  azimuth double precision NOT NULL,
  azimuth_rate double precision NOT NULL,
  elevation double precision NOT NULL,
  elevation_rate double precision NOT NULL,
  FOREIGN KEY (spacecraft) REFERENCES spacecraft (id) ON DELETE CASCADE,
  UNIQUE (spacecraft, time)
);

CREATE TYPE schedule_state_enum AS ENUM ('idle', 'aligning', 'tracking', 'returning');

CREATE table schedule
(
  time timestamp without time zone NOT NULL,
  spacecraft integer NOT NULL,
  state schedule_state_enum NOT NULL,
  FOREIGN KEY (spacecraft) REFERENCES spacecraft (id) ON DELETE CASCADE,
  UNIQUE (spacecraft, time)
);

CREATE table schedule_meta
(
  onerow_select bool PRIMARY KEY DEFAULT TRUE,
  updated timestamp without time zone NOT NULL DEFAULT TIMESTAMP 'epoch',
  CONSTRAINT onerow_unique CHECK (onerow_select)
);

INSERT into schedule_meta (updated) VALUES (timestamp 'epoch');

GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO phil;
GRANT USAGE ON ALL SEQUENCES IN SCHEMA public TO phil;
