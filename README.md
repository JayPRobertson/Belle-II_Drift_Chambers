# Drift Chamber Simulation

The Belle II collaboration is currently working to replace and upgrade their Central Drift Chamber. This software was developed to conduct some preliminary modelling and data collection in Garfield++ and Geant4 in preparation for this upgrade. 

This simulation creates a Geant4 geometry modelled off of the current Belle II Central Drift Chamber (CDC). The specifications for the detector are read from `geometry.json`, which currently specifies various properties based on the [2010 Belle II Technical Design Report](https://docs.belle2.org/files/4270/BELLE2-REPORT-2016-001/1/BELLE2-REPORT-2016-001.pdf) (pp. 202–208). 

<details>
<summary>&nbsp<span style="font-size: 20px; font-weight: bold;">Toolkit Installations</span></summary>
<br>

This simulation utilizes various toolkits that must be installed before attempting to run. The following are instructions for installing the necessary versions.

### ROOT
[ROOT](https://root.cern/) is a C++ data collection and analysis framework. It can be installed using CERN's [ROOT Installation Guide](https://root.cern/install/).

### Geant4
[Geant4](https://geant4.web.cern.ch/) is a toolkit for simulating particles through matter. This simulation is built to utilize the [v11.4.1 of Geant4](https://github.com/Geant4/geant4/tree/v11.4.1), which can be installed by following the [Geant4 Installation Guide](https://geant4.web.cern.ch/documentation/dev/ig_html/InstallationGuide/).

### Garfield++
[Garfield++](https://garfieldpp.docs.cern.ch/) is a toolkit for gaseous particle physics detectors. This simulation requires the [5.0 release](https://gitlab.cern.ch/garfield/garfieldpp/-/tree/5.0?ref_type=tags), which can be installed as follows:
```
git clone --branch 5.0 https://gitlab.cern.ch/garfield/garfieldpp.git $GARFIELD_HOME
cd garfieldpp
mkdir install
mkdir build
cd build
cmake -DGARFIELD_WITH_GSL=ON -DCMAKE_INSTALL_PREFIX=../install/ -DWITH_EXAMPLES=OFF  ../
make
make install
source ../install/share/Garfield/setupGarfield.sh
```
In this version, the path to `libomp` is broken. This can be fixed by directly copying the file from your installation location to the correct library. Example command:
```
cp /opt/homebrew/Cellar/libomp/22.1.5/libomp.dylib ../install/lib/
```
### JSON Parser
This simulation uses the [nlohmann C++ json parser](https://github.com/nlohmann/json) to read in its geometry. On macOS, this can be installed using: ```brew install nlohmann-json```. 

</details>

<details>
<summary>&nbsp<span style="font-size: 20px; font-weight: bold;">Running Files</span></summary>
<br>

Once the directory is replaced, set up the environmental variables for Garfield++, Geant4, and ROOT. An example of this may look like:
```
source ~/garfieldpp/install/share/Garfield/setupGarfield.sh
source ~/geant4/source/bin/geant4.sh
source /opt/homebrew/Cellar/root/6.38.04_1/bin/thisroot.sh
```
Following this, the simulation can be run using the Geant4 visualization UI:

```
cd DriftChamberSim/build/
cmake ../
chmod +x run_DCSim.sh
./run_DCSim.sh
```

### Random Seed
The initial momentum of each simulated particle is generated randomly. The seed for the random generator can be optionally set by passing it as a command line argument, e.g. `./run_DCSim.sh 12345`

### Gas Files
To run the Garfield++ section of the simulation, a `.gas` file is needed describing the properties of the gas mixture specified in `geometry.json`. Some sample files have been provided in `gas_files` and additional gas files can be made by modifying and running Garfield++'s [GasFile example](https://gitlab.cern.ch/garfield/garfieldpp/-/blob/master/Examples/GasFile/generate.C). This process takes approximately 2 hours to complete. 

### Warning
After the trajectories have loaded in the GUI, do NOT close the GUI window until the GUI menu info has printed to the terminal. This is the section of the output that begins with:
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

</details>

<details>
<summary>&nbsp<span style="font-size: 20px; font-weight: bold;">Output</span></summary>
<br>

The simulation outputs three data files. `run_DCSim.sh` provides an option to save these outputs to a filepath provided at the top of that file.

- `drift_times_lookup.csv` - a lookup table of sample drift times (in ns) of electrons originating at various positions in a drift cell; each cell of the csv file correponds to a 1mm x 1mm section of the drift cell
- `particle_and_track_data.root` - data about each particle sent through the drift chamber during the simulation, including the particle's initial position and momentum, as well as the times, energies, and positions of the particle as it passed through each layer in the cell. Includes a separate branch for delta rays.
- `libDriftChamberlib.so` - record of the necessary ROOT libraries

Sample data output can be found in `SampleData`.

</details>

<details>
<summary>&nbsp<span style="font-size: 20px; font-weight: bold;">Analysis</span></summary>
<br>

`KalmanFit` is a tool used to estimate the trajectory of particles using a [Kalman fitting](https://en.wikipedia.org/wiki/Kalman_filter) algorithm built using [GenFit2](https://github.com/GenFit/GenFit), a framework for track reconstruction. The project Makefile generates an executable for this tool that can be run using `./kalman`. All data will be output to the terminal (a sample output can be found in `SampleData/kalmanFit_output.txt`)

To inspect the simulation data, a ROOT TBrowser can be opened using the following:
```
root particle_and_track_data.root
gSystem->Load("libDriftChamberlib.so");
TBrowser b
```

</details>

<details>
<summary>&nbsp<span style="font-size: 20px; font-weight: bold;">Licenses and Disclaimers</span></summary>
<br>

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

</details>