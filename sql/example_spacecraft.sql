\c "gs-scheduler";
INSERT INTO spacecraft 
  (name, priority, frequency_downlink, frequency_uplink, tle_source_type, tle_source_url, tle_spacecraft_uri)
  VALUES 
  (
    'ISS', 1, 145.8, 145.3, 'spacetrack', 'https://www.space-track.org/basicspacedata/query/class/tle_latest/NORAD_CAT_ID/25544/ORDINAL/1/EPOCH/%3Enow-30/format/3le', '0 ISS (ZARYA)'
  )
  , ('Funcube-1', 2, 145.85, 437.230, 'http', 'https://www.celestrak.com/NORAD/elements/amateur.txt', 'FUNCUBE-1 (AO-73)');
