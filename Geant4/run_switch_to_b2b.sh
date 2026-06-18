#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Geant4/b2b"

cp "$FILEPATH"/*.cc ../src/
rm ../src/exampleB2b.cc
cp "$FILEPATH"/exampleB2b.cc ..
cp "$FILEPATH"/*.hh ../include/
cp "$FILEPATH"/run_exampleb2b.sh .
