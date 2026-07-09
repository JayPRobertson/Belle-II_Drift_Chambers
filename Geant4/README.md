# Geant4 Drift Chamber

Files for a basic drift chamber geometry built using a modified example in [v11.4.1 of Geant4](https://github.com/Geant4/geant4/tree/v11.4.1). It is based off of the [2010 Belle II Technical Design Report](https://docs.belle2.org/files/4270/BELLE2-REPORT-2016-001/1/BELLE2-REPORT-2016-001.pdf) (pp. 202–208).

## Directory Descriptions
`b2b` - A basic drift chamber geometry. Does not include wires, gas layers, or the ability to provide a geometry. Ouputs data as csv files.

`b2b_kalman` - A basic drift chamber geometry (built off of `b2b_superlayers`). Includes gas mixture as a set of layers rather than a solid volume and reads geometry from a json file. Ouputs some data as a csv file and other data printed to terminal.

`b2b_layered` - A more advanced drift chamber geometry. Includes gas mixture as a set of layers rather than a solid volume. Includes an option to read in a more detailed geometry from a json file (sub-directory `b2b_superlayers`). Outputs data as csv files.

`b2b_root` - A more advanced drift chamber geometry (built off of `b2b_superlayers`). Includes gas mixture as a set of layers rather than a solid volume and reads geometry from a json file. Ouputs data as a root file.


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

Make sure to adjust the filepath in `run_exampleb2b.sh` before running.

The initial momentum of the generated particles along each track are generated randomly. The seed for the random geenrator can be optionally set by passing it as a command line argument, e.g. `./run_exampleb2b.sh 12345`

Three `run_switch_to_<geometry name>.sh` files have been provided to easily switch between the different b2b geometries in this directory. To use one, copy it to the `build` directory and run it using `./run_switch_to_<geometry name>.sh`

## Output

Outputs three csv files of collected data. Filepath of output is set as `Python/csv/hits/`and can be modified in `run_exampleb2b.sh`.

- `entry_exit_data.csv` - the actual and expected point of entry and exit from the gas volume; for calculating multiple scattering
- `event_action_data.csv` - hit energy and hit positions
- `init_step_data.csv` - initial energy and position, and dEdx over volume

The `b2b_layered` and `b2b_superlayers` geometries also output additional files specified in their directories.

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
