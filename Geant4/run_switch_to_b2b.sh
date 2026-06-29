#!/bin/bash

# This run_switch file assumes all .cc and .hh files are together in one directory
# For an example of this, see the files in b2b_layered directory

FILEPATH="$HOME/Desktop/Belle II/Geant4/b2b"

cp "$FILEPATH"/*.cc ../src/
rm ../src/exampleB2b.cc
cp "$FILEPATH"/exampleB2b.cc ..
cp "$FILEPATH"/*.hh ../include/
cp "$FILEPATH"/run_exampleb2b.sh .
