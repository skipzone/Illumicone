#! /bin/bash
for i in {1..24}; do timeout 1s $ILLUMICONE_BIN/stringTester -c $ILLUMICONE_CONFIG/activeConfig.json $i 128 128 128; done
