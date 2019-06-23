#! /bin/bash
for i in {1..48}; do timeout 1s stringTester -c $ILLUMICONE_CONFIG/activeConfig.json $i 128 128 128; done
