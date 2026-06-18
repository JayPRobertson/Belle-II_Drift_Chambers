#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II/Python/csv/superlayers"

rm -f event_action_data.csv layered_edep_data.csv layer_radius.csv init_step_data.csv entry_exit_data.csv

echo "event_id,num_trajectory,num_hits,energies,hitx,hity,hitz" > event_action_data_layered.csv

echo "index,entry_x,entry_y,entry_z,exit_x,exit_y,exit_z,edep" > layered_edep_data.csv

echo "r1,r2" > layer_radius.csv

echo "initx_p,inity_p,initz_p,actx_ent,acty_ent,actz_ent,actx_exit,acty_exit,actz_exit,predx_ent,predy_ent,predz_ent,predx_exit,predy_exit,predz_exit" > entry_exit_data.csv

echo "energies,initx,inity,initz,tot_dist,dEdx,beta_gamma" > init_step_data.csv

make
./exampleB2b

cp event_action_data_layered.csv "$FILEPATH/event_action_data_layered_nowires.csv"
cp layered_edep_data.csv "$FILEPATH/layered_edep_data_nowires.csv"
cp init_step_data.csv "$FILEPATH/init_step_data_nowires.csv"
cp entry_exit_data.csv "$FILEPATH/entry_exit_data_nowires.csv"
