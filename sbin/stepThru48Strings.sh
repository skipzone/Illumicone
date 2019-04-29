#! /bin/bash
for i in {1..48}; do timeout 1s stringTester ~/activeConfig.json $i 128 128 128; done
