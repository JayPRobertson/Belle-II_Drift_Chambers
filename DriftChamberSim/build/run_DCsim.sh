#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/output"

# Command-line args
FILENAME="$0"
SEED="$1"

# Remove old files
rm *.csv
rm *.root

make
echo "i,numWires,wireLength,type,r1,r2" > layer_radius.csv

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
cp particle_and_track_data.root "$FILEPATH/"
cp libDriftChamberlib.so "$FILEPATH/"
cp drift_times_lookup.csv "$FILEPATH/"
cp diffusion_lookup.csv "$FILEPATH/"
cp layer_radius.csv "$FILEPATH/"

# Clean directory
rm *.so
rm *.gas