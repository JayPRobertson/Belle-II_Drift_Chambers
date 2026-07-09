# Utilizing a Kalman Fitter to Estimate Trajectories

While Geant4 tracks the trajectories of simulated particles, they can also be estimated using a [Kalman fitter](https://en.wikipedia.org/wiki/Kalman_filter). This is accolmmplished using [GenFit2](https://github.com/GenFit/GenFit), a framework for track reconstruction.

## Running Files

This code is a supplemental to the main body of code in `b2b` and uses the `b2b_layered/b2b_superlayers` model. Replace select files in `b2b` directory with those in `b2b_kalman` and run as normal. Note that this geometry uses a different CMake file as it requires the installation of GenFit.

Make sure you have the nlohmann C++ json parser installed before attempting to run this version. On macOS, this can be done using: ```brew install nlohmann-json```.

This code accepts a file called `geometry.json`, which describes the geometry of the desired detector. Modify this file to modify the detector construction. 

## Output

- `entry_exit_data.csv` - the actual and expected point of entry and exit from the gas layers; for calculating multiple scattering
- `init_step_data.csv` - initial energy and position, and dEdx over gas layer volumes
- `layer_radius.csv` - inner and outer radius of each gas layer

There is additional information printed to the terminal about each the fitting of each track in the form:
```
Track = <1 = primary particle>, hits = <number of hits>, fitted = <1 if kalman fitting successful, else 0>, converged = <1 if kalman fitting converged, else 0>, chi2 = <χ² value>, ndf = <number of degrees of freedom>
Momentum = <fitted momentum> GeV
Position = <fitted x> <fitted y> <fitted z> cm
```
The Kalman fitted trajectories are displayed in the Geant4 GUI in yellow, while Geant4's tracked trajectories are displayed in red.


