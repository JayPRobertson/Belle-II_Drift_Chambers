# Layered Gas Volume Drift Chamber

One way to measure energy deposition through a volume is to divide up the volume into layers and detect when a particle enters and exits that layer. 

## Running Files

This code is a supplemental to the main body of code in `b2b`. Replace select files in `b2b` directory with those in `b2b_layered` and run as normal. There is a modified version of this layered geometry in `b2b_superlayers` that adds the ability to edit superlayers of drift cells, and a shell script file that will switch to the geometry. The `b2b_kalman` utilizes this `b2b_superlayers` geometry and can be used to estimate the trajectory of particles using a [Kalman fitting](https://en.wikipedia.org/wiki/Kalman_filter) algorithm.

Make sure you have the nlohmann C++ json parser installed before attempting to run this version. On macOS, this can be done using: ```brew install nlohmann-json```.

This code accepts a file called `geometry.json`, which describes the geometry of the desired detector. Modify this file to modify the detector construction.

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

- `entry_exit_data.csv` - the actual and expected point of entry and exit from the gas layers; for calculating multiple scattering
- `event_action_data_layered.csv` - hit energy and hit positions
- `init_step_data.csv` - initial energy and position, and dEdx over gas layer volumes
- `layered_edep_data.csv` - energy deposition within layers
- `layer_radius.csv` - inner and outer radius of each gas layer
