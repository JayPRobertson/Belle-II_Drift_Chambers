#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Geant4/b2b_superlayers"

./run_switch_to_layered.sh

cp "$FILEPATH"/*.cc ../src/
cp "$FILEPATH"/*.hh ../include/

cp "$FILEPATH/geometry.json" .
cp "$FILEPATH/run_exampleb2b.sh" .