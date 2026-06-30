#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/csv/superlayers"

# Command-line args
FILENAME="$0"
SEED="$1"

rm -f event_action_data.csv layered_edep_data.csv layer_radius.csv init_step_data.csv entry_exit_data.csv

echo "event_id,num_trajectory,num_hits,energies,hitx,hity,hitz" > event_action_data_layered.csv

echo "index,entry_x,entry_y,entry_z,exit_x,exit_y,exit_z,edep" > layered_edep_data.csv

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

cp event_action_data_layered.csv "$FILEPATH/"
cp layered_edep_data.csv "$FILEPATH/"
cp layer_radius.csv "$FILEPATH/"
cp init_step_data.csv "$FILEPATH/"
cp entry_exit_data.csv "$FILEPATH/"
