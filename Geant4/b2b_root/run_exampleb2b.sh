#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/root"

make
./exampleB2b

mv particle_and_track_data.root "$FILEPATH/particle_and_track_data.root"

ln -s libexampleB2blib.dylib libexampleB2blib.so
mv libexampleB2blib.so "$FILEPATH/"

rm *.csv