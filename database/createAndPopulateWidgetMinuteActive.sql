/*
    ----------------------------------------------------------------------------
    This script creates and populates the widget_minute_active table.

    The widget_minute_active_table contains a row for each minute that a widget
    was active.

    Ross Butler  November 2016
    ----------------------------------------------------------------------------

    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/


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

