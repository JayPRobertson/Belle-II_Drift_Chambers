# Garfield++ Drift Chamber

Files for a basic drift chamber geometry built using a modified example in the [5.0 version of Garfield++](https://gitlab.cern.ch/garfield/garfieldpp/-/tree/5.0?ref_type=tags). 

## File Descriptions
More in-depth descriptions can be found in a comment at the top of each file.

`mdt.C` - Drift chamber with a single drift cell within a gas mix provided by CL arg; plots enviromental data for single track

`mdt_no_clargs.C` - Drift chamber with a single drift cell within a set gas mix; plots enviromental data for single track

`mdt_circle_cell.C` - Drift chamber with rings of drift cells; plots chamber for a single track

`mdt_multicell.C` -  Drift chamber with 3x3 grid of drift cells; plots chamber for one or two tracks

`mdt_rantrack.C` -  Drift chamber with a single drift cell; collects drift data for randomly generates 1000 tracks

`mdt_rantrack.C` -  Drift chamber with either a single, 3x3 grid, or rings of drift cells; collects wire sag and stretch data for a single track

`mdt_two_tracks.C` -  Drift chamber with a single drift cell; plots environmental data for two tracks


## Garfield++ Installation Steps
```
git clone --branch 5.0 https://gitlab.cern.ch/garfield/garfieldpp.git $GARFIELD_HOME
cd garfieldpp
mkdir install
mkdir build
cd build
cmake -DGARFIELD_WITH_GSL=ON -DCMAKE_INSTALL_PREFIX=../install/ -DWITH_EXAMPLES=OFF  ../
make -j <number of cores on your computer>
make install
source ../install/share/Garfield/setupGarfield.sh
```

In this version, the path to libomp is broken. This can be fixed by directly copying the file from your installation location to the correct library. Example command:
```
cp /opt/homebrew/Cellar/libomp/22.1.5/libomp.dylib ../install/lib/
```

## Running files
The provided examples are built off of the [Garfield++ example DriftTube](https://gitlab.cern.ch/garfield/garfieldpp/-/tree/5.0/Examples/DriftTube?ref_type=tags) located in the `garfieldpp/Examples/DriftTube/` directory. In this directory, the basic example can be run using:
```
mkdir build
cd build
cmake ../
make
./mdt
```
To run a modified example from `mdt_files`:
1. Copy all .gas files in `gas_files` to `garfieldpp/Examples/DriftTube/`
2. Copy the contents of the modified example to `garfieldpp/Examples/DriftTube/mdt.C`
3. If necessary, copy the corresponding .sh file from `sh_files` to `garfieldpp/Examples/DriftTube/build`

Some examples have corresponding .sh file in `sh_files`:
- `mdt_rantrack.C`  -> `run_rantrack.sh`
- `mdt_sag_wires.C` -> `run_sag.sh`
- `mdt.C `          -> `run_mdt.sh`

Before running, make sure to set the file permissions with `chmod +x run_<filename>.sh` and to change the filepath inside.

4. Run using either the same method as the basic example or using a shell script with `./run_<filename.sh>`

## Supplementary Material
- [Analysis of collected data](https://github.com/JayPRobertson/Belle-II/blob/main/Python/BelleII_Gas_Analysis.ipynb)