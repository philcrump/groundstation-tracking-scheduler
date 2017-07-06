\c "gs-scheduler";
INSERT INTO spacecraft 
  (name, priority, frequencies, tle_catalog_id)
  VALUES 
   (
    'ISS', 1, array[145.8, 145.3], 25544
  )
  ,(
  	'Funcube-1', 2, array[145.85, 437.230], 39444
  );