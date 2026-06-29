# Outputting data as a ROOT file

## Running files
After copying these files to the B2b directory of Geant4's examples, this program is run in identical manner to the one specified previously:
```
cd geant4/examples/basic/B2/b2b/build
cmake ../
chmod +x run_exampleb2b.sh
./run_exampleb2b.sh
```

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