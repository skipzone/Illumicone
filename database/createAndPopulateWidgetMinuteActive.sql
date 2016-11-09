
CREATE TABLE widget_minute_active (
  widget_id TINYINT(3) UNSIGNED NOT NULL,
  minute_active DATETIME NOT NULL,
  PRIMARY KEY (widget_id, minute_active))
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;

INSERT widget_minute_active (widget_id, minute_active)
SELECT DISTINCT
    widget_id,
    FROM_UNIXTIME(CONVERT(UNIX_TIMESTAMP(rcvr_timestamp) / 60, UNSIGNED) * 60)
FROM widget_packet
WHERE considered_active_by_pattern = 1;

