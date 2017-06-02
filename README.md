# Groundstation Tracking Scheduler

This software aims to automate the computation of a tracking schedule for a satellite groundstation.

The Spacecraft, Track, Observation and Schedule data is stored in a PostgreSQL database. This allows external applications to modify input parameters and view computed output data through their own database interface.

## Usage

`./gss`

The application will:
 * Update TLEs if last updated > 15 minutes ago.
 * Recompute Satellite Tracks and Observations if new TLE or updated > 24 hours ago 
 * Recompute Schedule if any Tracks have been updated since the last Schedule computation.

### Compile

`make`

Dependencies: libpq5 libpq-dev libcurl4-openssl-dev

### Database Setup

The 'sql/' folder contains a database setup script (note this will drop an existing database), and an example spacecraft data insertion script (populates ISS & AO-73)

## Example Client

The 'example_client' folder contains an example database client application that, using computed schedule data, can be used as a direct realtime input for an AZ/EL Tracking System.

### Compile

`make`

Dependencies: libpq5 libpq-dev

## Authors

Phil Crump <phil@philcrump.co.uk>
