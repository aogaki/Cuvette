#include <time.h>

#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#include "G4Threading.hh"
#else
#include "G4RunManager.hh"
#endif

#include "G4SystemOfUnits.hh"
#include "G4UImanager.hh"
#include "FTFP_BERT.hh"
#include "QGSP_BERT_HP.hh"
#include "QGSP_BIC_HP.hh"
#include "Shielding.hh"

#ifdef G4VIS_USE
#include "G4VisExecutive.hh"
#include "G4TrajectoryParticleFilter.hh"
#endif

#ifdef G4UI_USE
#include "G4UIExecutive.hh"
#endif

// Remove?
#include "Randomize.hh"

#include "BIDNAPhysicsList.hpp"
#include "BIDetectorConstruction.hpp"
#include "BIActionInitialization.hpp"
#include "BIPrimaryGeneratorAction.hpp"


namespace {
   void PrintUsage()
   {
      G4cerr << " Usage: " << G4endl;
      G4cerr << " ./LG [-m macro filename]\n"
             << " -a Show all trajectory (default show only ploton)\n"
             << " -q Using only quarter region of the plate\n"
             << " -b [beam profile number] Choose beam profile\n"
             << " -s Using /dev/shm for temporary output\n"
             << " -h cutting and showing the geometry\n"
             << "    Can I use some option of Geant4 itself?\n"
             << " --grid For Grid system, output is only a few parameters\n"
             << " --tile Using tile attenuator\n"
             << G4endl;
   }
}

unsigned int GetRandomSeed()
{
   // Using /dev/urandom for generating random number.
   // If it is not, I have to think solution.
   unsigned int seed;
   std::ifstream file("/dev/urandom", std::ios::binary);
   if (file.is_open()) {
      char *memblock;
      int size = sizeof(int);
      memblock = new char[size];
      file.read(memblock, size);
      file.close();
      seed = *reinterpret_cast<int *>(memblock);
      delete[] memblock;
   } else {
      seed = 0;
   }

   return seed;
}

int main(int argc, char **argv)
{
   G4String macro = "";
   G4bool showAll = false;
   BeamType beamType = kFirstBeam;
   G4bool forGrid = false;
   G4bool useQuarter = false;
   G4bool useShm = false;
   for (G4int i = 1; i < argc; i++) {
      if (G4String(argv[i]) == "-m") macro = argv[++i];
      else if (G4String(argv[i]) == "-a") showAll = true;
      else if (G4String(argv[i]) == "-q") useQuarter = true;
      else if (G4String(argv[i]) == "-s") useShm = true;
      else if (G4String(argv[i]) == "-b"){
         G4String type = *argv[++i];
         if(type == "1") beamType = kFirstBeam;
         else if(type == "2") beamType = kSecondBeam;
         else if(type == "3") beamType = kThirdBeam;
         else if(type == "0") beamType = kElectronTest;
         else{
            G4cout << "Beam type is wrong" << G4endl;
            PrintUsage();
            return 1;
         }
      }
      else if (G4String(argv[i]) == "--grid") forGrid = true;
      else {
         PrintUsage();
         return 1;
      }
   }

   G4cout << "Beam type is " << beamType << G4endl;

   if (forGrid) {
      G4cout << "Small output mode" << G4endl;
   }

   // Remove?
   // Choose the Random engine
   CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine);
   unsigned int seed = GetRandomSeed();
   if (seed == 0) seed = time(0);
   G4cout << "\nseed = " << seed << G4endl;
   CLHEP::HepRandom::setTheSeed(seed);
   G4Random::setTheSeed(seed);

   // Construct the default run manager
   //

#ifdef G4MULTITHREADED
   G4MTRunManager *runManager = new G4MTRunManager();
   runManager->SetNumberOfThreads(G4Threading::G4GetNumberOfCores());
   //runManager->SetNumberOfThreads(1);
#else
   G4RunManager *runManager = new G4RunManager();
#endif

   // Set mandatory initialization classes
   //
   // Detector construction
   runManager->SetUserInitialization(new BIDetectorConstruction(forGrid));

   // Physics list
   //G4VModularPhysicsList *physicsList = new FTFP_BERT;
   //G4VModularPhysicsList *physicsList = new QGSP_BERT_HP;
   //G4VModularPhysicsList *physicsList = new QGSP_BIC_HP;
   G4VModularPhysicsList *physicsList = new Shielding;
   //G4VModularPhysicsList *physicsList = new BIDNAPhysicsList;
   physicsList->SetVerboseLevel(0);
   //physicsList->SetCutValue(1.*um, "proton");
   physicsList->SetDefaultCutValue(10.*um);
   physicsList->SetCuts();
   runManager->SetUserInitialization(physicsList);

   // Primary generator action and User action intialization
   runManager->SetUserInitialization(new BIActionInitialization(beamType, forGrid, useQuarter, useShm));

   // Initialize G4 kernel
   //
   runManager->Initialize();

#ifdef G4VIS_USE
   // Initialize visualization
   G4VisManager *visManager = new G4VisExecutive;
   visManager->Initialize();

   if (!showAll) { //Show only proton
      G4TrajectoryParticleFilter *filterp = new G4TrajectoryParticleFilter;
      filterp->Add("proton");
      visManager->RegisterModel(filterp);
   }
#endif

   // Get the pointer to the User Interface manager
   G4UImanager *UImanager = G4UImanager::GetUIpointer();

   if (macro != "") {
      // batch mode
      G4String command = "/control/execute ";
      UImanager->ApplyCommand(command + macro);
   } else {
      // interactive mode : define UI session
#ifdef G4UI_USE
      G4UIExecutive *ui = new G4UIExecutive(argc, argv);
#ifdef G4VIS_USE
      UImanager->ApplyCommand("/control/execute init_vis.mac");
#else
      UImanager->ApplyCommand("/control/execute init.mac");
#endif
      if (ui->IsGUI()) {
         UImanager->ApplyCommand("/control/execute icons.mac");
      }
      ui->SessionStart();
      delete ui;
#endif
   }

   // Job termination
   // Free the store: user actions, physics_list and detector_description are
   // owned and deleted by the run manager, so they should not be deleted
   // in the main() program !

#ifdef G4VIS_USE
   delete visManager;
#endif

   delete runManager;

   return 0;
}
