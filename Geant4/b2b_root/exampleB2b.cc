#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "FTFP_BERT.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"

int main(int argc, char** argv){
  // Detect interactive mode (if no arguments) and define UI session
  G4UIExecutive* ui = nullptr;
  
  if (argc == 1) {
    ui = new G4UIExecutive(argc, argv);
  }
  
  // Construct the default run manager
  auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);
  
  // Set mandatory initialization classes
  runManager->SetUserInitialization(new B2b::DetectorConstruction());

  auto physicsList = new FTFP_BERT;
  physicsList->RegisterPhysics(new G4StepLimiterPhysics());

  runManager->SetUserInitialization(physicsList);
  runManager->SetUserInitialization(new B2::ActionInitialization());
  runManager->Initialize();
  
  // Initialize visualization with the default graphics system
  auto visManager = new G4VisExecutive(argc, argv, "OGL", "Quiet");
  visManager->Initialize();

  // Get the pointer to the User Interface manager
  auto UImanager = G4UImanager::GetUIpointer();

  // Process macro or start UI session
  if (!ui) {
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command + fileName);
  }
  else {
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    if (ui->IsGUI()) {
      UImanager->ApplyCommand("/control/execute gui.mac");
    }
    ui->SessionStart();
    delete ui;
  }
  
  delete visManager;
  delete runManager;
  
  return 0;
}

