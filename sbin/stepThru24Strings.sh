#! /bin/bash
for i in {1..24}; do timeout 1s stringTester ~/activeConfig.json $i 128 128 128; done
