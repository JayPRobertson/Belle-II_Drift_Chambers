#!/bin/bash

FILEPATH="$HOME/Desktop/Belle II"

rm *.csv

echo "event_id,num_trajectory,num_hits,energies,hitx,hity,hitz" > event_action_data.csv

echo "initx_p,inity_p,initz_p,actx_ent,acty_ent,actz_ent,actx_exit,acty_exit,actz_exit,predx_ent,predy_ent,predz_ent,predx_exit,predy_exit,predz_exit" > entry_exit_data.csv

echo "energies,initx,inity,initz,tot_dist,dEdx,beta_gamma" > init_step_data.csv

make
./exampleB2b

mv event_action_data.csv "$FILEPATH/Python/csv/hits/"
mv init_step_data.csv "$FILEPATH/Python/csv/hits/"
mv entry_exit_data.csv "$FILEPATH/Python/csv/hits/"