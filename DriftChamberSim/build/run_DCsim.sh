#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/"

# Command-line args
FILENAME="$0"
SEED="$1"

# Remove old files
rm *.csv
rm *.root

make
echo "i,numWires,r1,r2" > layer_radius.csv

cp ../gas_files/*.gas .
./DriftTimes

if [ "$SEED" ]; then
   echo "============ Random generator seed detected ============"
   ./DriftChamberSim "$SEED" 
else
   ./DriftChamberSim
fi

ln -s libDriftChamberlib.dylib libDriftChamberlib.so

# Copy data to storage location
cp particle_and_track_data.root "$FILEPATH/root/"
cp libDriftChamberlib.so "$FILEPATH/root/"
cp drift_times_lookup.csv "$FILEPATH/csv/drifting/"

# Clean directory
rm *.so
rm *.gas