# Geant4 Drift Chamber

Files for a basic drift chamber geometry built using a modified example in [v11.4.1 of Geant4](https://github.com/Geant4/geant4/tree/v11.4.1).

This simulation creates a Geant4 geometry modelled off of the current Belle II Central Drift Chamber (CDC). This geometry is read from a `geometry.json`, which specifies various properties based on the [2010 Belle II Technical Design Report](https://docs.belle2.org/files/4270/BELLE2-REPORT-2016-001/1/BELLE2-REPORT-2016-001.pdf) (pp. 202–208). 

## Running Files
This geometry was built using the [B2b example](https://github.com/Geant4/geant4/tree/v11.4.1/examples/basic/B2/B2b) provided by Geant4 as a framework. Rather than adjusting the makefiles and namespace, it runs off of the b2b files with adjusted src, .mac, and exampleB2b.cc files. As such, this modified geometry can be easily run if the `geant4/examples/basic/B2/b2b` directory is replaced with the contents of the provided b2b file.

One the directory is replaced, set up the environmental variables for Geant4 (`geant4.sh`) and ROOT (`thisroot.sh`). This can be done by executing the files `geant4.sh` and `thisroot.sh` respectively. An example of this may look like:
```
source ~/geant4/source/bin/geant4.sh
source /opt/homebrew/Cellar/root/6.38.04_1/bin/thisroot.sh
```
Following this, the simulation can be run using the Geant4 visualization UI:

```
cd geant4/examples/basic/B2/b2b/build
cmake ../
chmod +x run_exampleb2b.sh
./run_exampleb2b.sh
```

Make sure to adjust the filepath in `run_exampleb2b.sh` and ensure you have the nlohmann C++ json parser installed before attempting to run. On macOS, this can be done using: ```brew install nlohmann-json```.

The initial momentum of the generated particles along each track are generated randomly. The seed for the random geenrator can be optionally set by passing it as a command line argument, e.g. `./run_exampleb2b.sh 12345`

**WARNING**: After the trajectories have loaded in the GUI, do NOT close the GUI window until the GUI menu info has printed to the terminal. This will start with:
```
#
# This file permits to customize, with commands,
# the menu bar of the G4UIXm, G4UIQt, G4UIWin32 sessions.
# It has no effect with G4UIterminal.
#
# File menu :
.
.
.
```
If the window is closed early, a segmentation fault will occur and no data will be saved.


## Output
Outputs one ROOT file of collected data and one .so file. The ROOT file does not collect all the data that the csv files output by the `b2b` and `b2b_layered` examples do, but instead a more minimal version. Filepath of output is set as `Python/root/`and can be modified in `run_exampleb2b.sh`.

- `particle_and_track_data.root` - includes the initial position and momentum of each particle, and a collection of times, energies, and positions of track segments for each particle track. Includes a separate branch for delta rays.
- `libexampleB2blib.so` - specifies the necessary root libraries

To inspect this data, a ROOT TBrowser can be opened using the following:
```
root particle_and_track_data.root
gSystem->Load("libexampleB2blib.so");
TBrowser b
```

Sample data output can be found in `Python/root`.

## Analysis
`KalmanFit` is an tool used to estimate the trajectory of particles using a [Kalman fitting](https://en.wikipedia.org/wiki/Kalman_filter) algorithm built using [GenFit2](https://github.com/GenFit/GenFit), a framework for track reconstruction. The project Makefile generates an executable for this tool that can be run using `./kalman`. All data will be output to the terminal.

## Supplementary Material
- [Analysis of collected data](https://github.com/JayPRobertson/Belle-II/blob/main/Python/geant4_data_analysis.ipynb)

## Geant4 License and Disclaimer
The following disclaimer was removed from all file headers, but its validity still holds for any use of the software found in this directory:

```
The Geant4 software is copyright of the Copyright Holders of the Geant4 Collaboration.
It is provided under the terms and conditions of the Geant4 Software License, included
in the file LICENSE and available at http://cern.ch/geant4/license. These include a list
of copyright holders.

Neither the authors of this software system, nor their employing institutes, nor the
agencies providing financial support for this work make any representation or warranty,
express or implied, regarding this software system or assume any liability for its use.
Please see the license in the file LICENSE and URL above for the full disclaimer and the
limitation of liability.

This code implementation is the result of the scientific and technical work of the GEANT4
collaboration. By using, copying, modifying or distributing the software (or any work
based on the software) you agree to acknowledge its use in resulting scientific
publications, and indicate your acceptance of all terms of the Geant4 Software license.
```
Learn more about the Geant4 license at:  http://cern.ch/geant4/license .