/*
    ----------------------------------------------------------------------------
    This script creates the widget_activity relational database.

    Ross Butler  October 2016
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


-- MySQL Workbench Forward Engineering

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='TRADITIONAL,ALLOW_INVALID_DATES';

-- -----------------------------------------------------
-- Schema widget_activity
-- -----------------------------------------------------
CREATE SCHEMA IF NOT EXISTS `widget_activity` DEFAULT CHARACTER SET latin1 ;
USE `widget_activity` ;


-- -----------------------------------------------------
-- Table `widget_activity`.`widget`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `widget_activity`.`widget` (
  `widget_id` TINYINT(3) UNSIGNED NOT NULL,
  `name` VARCHAR(255) NOT NULL,
  PRIMARY KEY (`widget_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;

INSERT widget VALUES (0, 'reserved');
INSERT widget VALUES (1, 'Eye');
INSERT widget VALUES (2, 'Shirleys Web');
INSERT widget VALUES (3, 'Bells');
INSERT widget VALUES (4, 'Steps');
INSERT widget VALUES (5, 'Rain Stick');
INSERT widget VALUES (6, 'TriObelisk');
INSERT widget VALUES (7, 'Box Theramin');
INSERT widget VALUES (8, 'Plunger');
INSERT widget VALUES (9, 'FourPlay');
INSERT widget VALUES (10, 'unassigned');
INSERT widget VALUES (11, 'unassigned');
INSERT widget VALUES (12, 'unassigned');
INSERT widget VALUES (13, 'unassigned');
INSERT widget VALUES (14, 'unassigned');
INSERT widget VALUES (15, 'unassigned');


-- -----------------------------------------------------
-- Table `widget_activity`.`payload_type`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `widget_activity`.`payload_type` (
  `payload_type_id` TINYINT(3) UNSIGNED NOT NULL,
  `payload_type_desc` VARCHAR(255) NOT NULL,
  PRIMARY KEY (`payload_type_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;

INSERT payload_type VALUES (0, 'stress test');
INSERT payload_type VALUES (1, 'position/velocity');
INSERT payload_type VALUES (2, 'measurement vector');
INSERT payload_type VALUES (3, 'unsupported');
INSERT payload_type VALUES (4, 'unsupported');
INSERT payload_type VALUES (5, 'custom');


-- -----------------------------------------------------
-- Table `widget_activity`.`widget_packet`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `widget_activity`.`widget_packet` (
  `widget_packet_id` BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `rcvr_timestamp` DATETIME(3) NOT NULL,
  `widget_id` TINYINT(3) UNSIGNED NOT NULL,
  `is_active` TINYINT(1) NOT NULL,
  `channel` TINYINT(3) UNSIGNED NOT NULL,
  `payload_type_id` TINYINT(3) UNSIGNED NOT NULL,
  `considered_active_by_pattern` TINYINT(1) NOT NULL,
  PRIMARY KEY (`widget_packet_id`),
  INDEX `time_id` (`rcvr_timestamp` ASC, `widget_id` ASC),
  INDEX `id_time` (`widget_id` ASC, `rcvr_timestamp` ASC),
  INDEX `fk_payload_type_idx` (`payload_type_id` ASC),
  CONSTRAINT `fk_payload_type`
    FOREIGN KEY (`payload_type_id`)
    REFERENCES `widget_activity`.`payload_type` (`payload_type_id`))
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `widget_activity`.`custom_payload`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `widget_activity`.`custom_payload` (
  `widget_packet_id` BIGINT(20) UNSIGNED NOT NULL,
  `payload_length` TINYINT(3) UNSIGNED NOT NULL,
  `data_bytes` BINARY(31) NULL DEFAULT NULL,
  PRIMARY KEY (`widget_packet_id`),
  CONSTRAINT `fk_custom_payload_widget_packet`
    FOREIGN KEY (`widget_packet_id`)
    REFERENCES `widget_activity`.`widget_packet` (`widget_packet_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `widget_activity`.`measurement_vector_payload`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `widget_activity`.`measurement_vector_payload` (
  `widget_packet_id` BIGINT(20) UNSIGNED NOT NULL,
  `measurement_idx` TINYINT(3) UNSIGNED NOT NULL,
  `measurement` MEDIUMINT(5) NOT NULL,
  PRIMARY KEY (`widget_packet_id`, `measurement_idx`),
  CONSTRAINT `fk_measurement_vector_payload_widget_packet`
    FOREIGN KEY (`widget_packet_id`)
    REFERENCES `widget_activity`.`widget_packet` (`widget_packet_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `widget_activity`.`pattern_controller_packet`
-- -----------------------------------------------------
-- We really don't need this table.  Instead, considered_active_by_pattern
-- should be in widget_packet.
-- CREATE TABLE IF NOT EXISTS `widget_activity`.`pattern_controller_packet` (
--   `widget_packet_id` BIGINT(20) UNSIGNED NOT NULL,
--   `sequence` TINYINT(3) UNSIGNED NOT NULL,
--   `channel` TINYINT(3) UNSIGNED NOT NULL,
--   `considered_active_by_pattern` TINYINT(1) NOT NULL,
--   `position` MEDIUMINT(5) NOT NULL,
--   `velocity` MEDIUMINT(5) NOT NULL,
--   PRIMARY KEY (`widget_packet_id`, `sequence`),
--   CONSTRAINT `pattern_controller_packet_widget_packet`
--     FOREIGN KEY (`widget_packet_id`)
--     REFERENCES `widget_activity`.`widget_packet` (`widget_packet_id`)
--     ON DELETE CASCADE
--     ON UPDATE CASCADE)
-- ENGINE = InnoDB
-- DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `widget_activity`.`position_velocity_payload`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `widget_activity`.`position_velocity_payload` (
  `widget_packet_id` BIGINT(20) UNSIGNED NOT NULL,
  `position` MEDIUMINT(5) NULL DEFAULT NULL,
  `velocity` MEDIUMINT(5) NULL DEFAULT NULL,
  PRIMARY KEY (`widget_packet_id`),
  CONSTRAINT `position_velocity_payload_widget_packet`
    FOREIGN KEY (`widget_packet_id`)
    REFERENCES `widget_activity`.`widget_packet` (`widget_packet_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;


-- -----------------------------------------------------
-- Table `widget_activity`.`stress_test_payload`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `widget_activity`.`stress_test_payload` (
  `widget_packet_id` BIGINT(20) UNSIGNED NOT NULL,
  `payload_count` INT(10) UNSIGNED NOT NULL,
  `tx_failure_count` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`widget_packet_id`),
  CONSTRAINT `fk_stress_test_payload_widget_packet`
    FOREIGN KEY (`widget_packet_id`)
    REFERENCES `widget_activity`.`widget_packet` (`widget_packet_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;


SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
