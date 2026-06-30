#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/csv/layered"

# Command-line args
FILENAME="$0"
SEED="$1"

rm *.csv

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

mv event_action_data_layered.csv "$FILEPATH/"
mv layered_edep_data.csv "$FILEPATH/"
mv layer_radius.csv "$FILEPATH/"
mv init_step_data.csv "$FILEPATH/"
mv entry_exit_data.csv "$FILEPATH/"


