#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/root"

# Command-line args
FILENAME="$0"
SEED="$1"

rm *.csv
rm *.root

make
echo "i,numWires,r1,r2" > layer_radius.csv

if [ "$SEED" ]; then
   echo "============ Random generator seed detected ============"
   ./exampleB2b "$SEED" 
else
   ./exampleB2b
fi

ln -s libexampleB2blib.dylib libexampleB2blib.so

# Copy data to storage location
#cp particle_and_track_data.root "$FILEPATH/"
#cp libexampleB2blib.so "$FILEPATH/"

rm *.so