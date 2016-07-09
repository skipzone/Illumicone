/*****************************************************************
 *                                                               *
 * Illumicone Widget Radio Functions                             *
 *                                                               *
 * Platform:  Arduino Uno, Pro, Pro Mini                         *
 *                                                               *
 * by Ross Butler, July 2016                                 )'( *
 *                                                               *
 *****************************************************************/

#pragma once

void configureRadio(
    RF24&         radio,
    const char*   writePipeAddress,
    uint8_t       txRetryDelayMultiplier,
    uint8_t       txMaxRetries,
    rf24_pa_dbm_e rfPowerLevel);


