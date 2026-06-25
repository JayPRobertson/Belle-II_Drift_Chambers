#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/root"

rm particle_and_track_data.root *.csv

make
./exampleB2b

cp particle_and_track_data.root "$FILEPATH/"
