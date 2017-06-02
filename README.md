# Groundstation Tracking Scheduler

This software aims to automate the computation of a tracking schedule for a satellite groundstation, using a list of spacecraft with associated unique priority levels.

The Spacecraft, Track, Observation and Schedule data is stored in a PostgreSQL database. This allows external applications to modify input parameters and view computed output data through a generic PostgreSQL interface.

## Usage

Configuration parameters are set in *config.ini*, copy the template from *config.ini.example*

`./gss`

The application will:
 * Update TLEs if last updated > 15 minutes ago.
 * Recompute Satellite Tracks and Observations if new TLE or updated > 24 hours ago 
 * Recompute Schedule if any Tracks have been updated since the last Schedule computation.

### Compile

`make`

Dependencies: libpq5 libpq-dev libcurl4-openssl-dev

### Database Setup

**init_database.sql**
 * *Example* database setup script - please modify before use.

**example_spacecraft.sql**
 * Populates the Spacecraft table with initial data for ISS & AO-73

## Example Client

The 'example_client' folder contains an example database client application that, using computed schedule data, can be used as a realtime input for an AZ/EL Tracking System.

### Compile

`make`

Dependencies: libpq5 libpq-dev

## Authors

Phil Crump <phil@philcrump.co.uk>
