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
   ./DriftChamberSim "$SEED" 
else
   ./DriftChamberSim
fi

ln -s libDriftChamberlib.dylib libDriftChamberlib.so

# Copy data to storage location
cp particle_and_track_data.root "$FILEPATH/"
cp libDriftChamberlib.so "$FILEPATH/"

rm *.so