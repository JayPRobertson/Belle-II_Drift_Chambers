#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Geant4/b2b_root"

cp "$FILEPATH"/*.cc ../src/
cp "$FILEPATH"/*.hh ../include/

cp "$FILEPATH/run_exampleb2b.sh" .
cp "$FILEPATH/geometry.json" .
