#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/csv/superlayers"

# Command-line args
FILENAME="$0"
SEED="$1"

rm -f layer_radius.csv init_step_data.csv entry_exit_data.csv

echo "r1,r2" > layer_radius.csv
echo "initx_p,inity_p,initz_p,actx_ent,acty_ent,actz_ent,actx_exit,acty_exit,actz_exit,predx_ent,predy_ent,predz_ent,predx_exit,predy_exit,predz_exit" > entry_exit_data.csv
echo "energies,initx,inity,initz,tot_dist,dEdx,beta_gamma" > init_step_data.csv

make

if [ "$SEED" ]; then
   echo "============ Random generator seed detected ============"
   ./exampleB2b "$SEED" 
else
   ./exampleB2b
fi

cp layer_radius.csv "$FILEPATH/"
cp init_step_data.csv "$FILEPATH/"
cp entry_exit_data.csv "$FILEPATH/"
