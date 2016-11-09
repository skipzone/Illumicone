-- The thresholds used below are those that were in effect for Burning Man 2016
-- and Idaho Decompression 2016.  They may not be correct for Element 11 2016.


-- For Eye, set considered_active_by_pattern = 1 when the corresponding
-- position (light intensity) is greater than 200.
UPDATE widget_packet wp
    INNER JOIN position_velocity_payload pvp ON wp.widget_packet_id = pvp.widget_packet_id
SET wp.considered_active_by_pattern = CASE WHEN pvp.position <= 200 THEN 0 ELSE 1 END
WHERE wp.widget_id = 1
      AND wp.considered_active_by_pattern IS NULL;


-- For Shirley's Web, set considered_active_by_pattern = 1 when the
-- corresponding velocity is greater than 20.
UPDATE widget_packet wp
    INNER JOIN position_velocity_payload pvp ON wp.widget_packet_id = pvp.widget_packet_id
SET wp.considered_active_by_pattern = CASE WHEN pvp.velocity <= 20 THEN 0 ELSE 1 END
WHERE wp.widget_id = 2
      AND wp.considered_active_by_pattern IS NULL;


-- For Bells, set considered_active_by_pattern = 1 when the corresponding
-- position (sound intensity) is greater than 170.
UPDATE widget_packet wp
    INNER JOIN position_velocity_payload pvp ON wp.widget_packet_id = pvp.widget_packet_id
SET wp.considered_active_by_pattern = CASE WHEN pvp.position <= 170 THEN 0 ELSE 1 END
WHERE wp.widget_id = 3
      AND wp.considered_active_by_pattern IS NULL;


-- Set considered_active_by_pattern = is_active for Steps.  
UPDATE widget_packet
SET considered_active_by_pattern = is_active
WHERE widget_id = 4
      AND considered_active_by_pattern IS NULL;


-- Rain Stick


-- Set considered_active_by_pattern = is_active for position/velocity payloads
-- from TriObelisk.  Set it to false for all other (i.e., stress-test) payloads.
UPDATE widget_packet
SET considered_active_by_pattern = CASE payload_type_id WHEN 1 THEN is_active ELSE 0 END
WHERE widget_id = 6
      AND considered_active_by_pattern IS NULL


-- Box Theramin


-- For Plunger, set considered_active_by_pattern = 1 when the corresponding
-- position (sound intensity) is 700 or higher.
UPDATE widget_packet wp
    INNER JOIN position_velocity_payload pvp ON wp.widget_packet_id = pvp.widget_packet_id
SET wp.considered_active_by_pattern = CASE WHEN pvp.position < 700 THEN 0 ELSE 1 END
WHERE wp.widget_id = 8
      AND wp.considered_active_by_pattern IS NULL;


-- Set considered_active_by_pattern = is_active for position/velocity payloads
-- from FourPlay.  Set it to false for all other (i.e., stress-test) payloads.
UPDATE widget_packet
SET considered_active_by_pattern = CASE payload_type_id WHEN 1 THEN is_active ELSE 0 END
WHERE widget_id = 9
      AND considered_active_by_pattern IS NULL
