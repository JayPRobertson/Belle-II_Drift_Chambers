#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/root"

# Command-line args
FILENAME="$0"
SEED="$1"

make

if [ "$SEED" ]; then
   echo "============ Random generator seed detected ============"
   ./exampleB2b "$SEED" 
else
   ./exampleB2b
fi

mv particle_and_track_data.root "$FILEPATH/particle_and_track_data.root"

ln -s libexampleB2blib.dylib libexampleB2blib.so
cp libexampleB2blib.so "$FILEPATH/"

rm *.csv
rm *.so