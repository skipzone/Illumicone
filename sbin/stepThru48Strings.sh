#! /bin/bash
for i in {1..48}; do timeout 1s ~/devl/Illumicone/src/stringTester ~/devl/Illumicone/config/activeConfig.json $i 128 128 128; done
