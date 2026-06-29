# Outputting data as a ROOT file

## Running Files
This code is a supplemental to the main body of code in `b2b`. Replace select files in `b2b` directory with those in `b2b_layered` and run as normal. It uses the `b2b_superlayers` detector geometry, but outputs data differently. 

For more information on running this geometry, see the README of the `b2b_layered/b2b_superlayers` directory.

## Output
Outputs one ROOT file of collected data and one .so file. The ROOT file does not collect all the data that the csv files output by the `b2b` and `b2b_layered` examples do, but instead a more minimal version. Filepath of output is set as `Python/root/`and can be modified in `run_exampleb2b.sh`.

- `particle_and_track_data.root` - includes the initial position and momentum of each particle, and a collection of times, energies, and positions of track segments for each particle track
- `libexampleB2blib.so` - specifies the necessary root libraries

To inspect this data, a ROOT TBrowser can be opened using the following:
```
root particle_and_track_data.root
gSystem->Load("libexampleB2blib.so");
TBrowser b
```

Sample data output can be found in `Python/root`.