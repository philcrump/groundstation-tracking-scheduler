# Groundstation Tracking Scheduler [![Build Status](https://travis-ci.org/philcrump/groundstation-tracking-scheduler.svg?branch=master)](https://travis-ci.org/philcrump/groundstation-tracking-scheduler)

This software aims to automate the computation of a tracking schedule for a satellite groundstation, using a list of spacecraft with associated unique priority levels.

The Spacecraft, Track, Observation and Schedule data is stored in a PostgreSQL database. This allows external applications to modify input parameters and view computed output data through a generic PostgreSQL interface.

## Usage

Configuration parameters are set in *config.ini*, copy the template from *config.ini.example*

`./gss`

The application will:
 * Update TLE for a spacecraft if TLE last updated > 15 minutes ago.
 * Recompute Satellite Tracks and Observations if new TLE or Track last updated > 24 hours ago.
 * Recompute Schedule if any Tracks have been updated since the last Schedule computation.

### Compile

`make`

Dependencies: libpq-dev libcurl4-openssl-dev

### Database Setup

#### Ubuntu PostgreSQL Repository

`echo "deb http://apt.postgresql.org/pub/repos/apt/ $(lsb_release -cs)-pgdg main" | sudo tee /etc/apt/sources.list.d/postgres.list
wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -
sudo apt-get update`

`sudo apt-get install postgres-11`

Scripts in *sql/*

`sudo -u postgres psql -U postgres -f - < sql/<script>.sql`

**init_database.sql**
 * _Example_ database setup script - please modify before use.

**example_spacecraft.sql**
 * Populates the Spacecraft table with initial data for ISS & AO-73

## Example Client

The *example_client/* folder contains an example database client application that, using computed schedule data, can be used as a realtime input for an AZ/EL Tracking System.

### Compile

`make`

Dependencies: libpq-dev libcurl4-openssl-dev

## Authors

Phil Crump <phil@philcrump.co.uk>
