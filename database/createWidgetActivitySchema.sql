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
  `isActive` TINYINT(1) NOT NULL,
  `channel` TINYINT(3) UNSIGNED NOT NULL,
  `payload_type_id` TINYINT(3) UNSIGNED NOT NULL,
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
CREATE TABLE IF NOT EXISTS `widget_activity`.`pattern_controller_packet` (
  `widget_packet_id` BIGINT(20) UNSIGNED NOT NULL,
  `sequence` TINYINT(3) UNSIGNED NOT NULL,
  `channel` TINYINT(3) UNSIGNED NOT NULL,
  `considered_active_by_pattern` TINYINT(1) NOT NULL,
  `position` MEDIUMINT(5) NOT NULL,
  `velocity` MEDIUMINT(5) NOT NULL,
  PRIMARY KEY (`widget_packet_id`, `sequence`),
  CONSTRAINT `pattern_controller_packet_widget_packet`
    FOREIGN KEY (`widget_packet_id`)
    REFERENCES `widget_activity`.`widget_packet` (`widget_packet_id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
DEFAULT CHARACTER SET = latin1;


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
