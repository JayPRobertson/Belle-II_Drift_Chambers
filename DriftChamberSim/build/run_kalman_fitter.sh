#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/output"

./kalman

# Copy data to storage location
cp cluster_info.csv "$FILEPATH/"
cp kalman_output.root "$FILEPATH/"