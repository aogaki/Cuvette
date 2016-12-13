#include "BIActionInitialization.hpp"
#include "BIPrimaryGeneratorAction.hpp"
#include "BIRunAction.hpp"
#include "BIEventAction.hpp"


BIActionInitialization::BIActionInitialization(BeamType beamType, G4bool gridFlag,
                                               G4bool quarterFlag, G4bool shmFlag)
   : G4VUserActionInitialization()
{
   fBeamType = beamType;
   fForGrid = gridFlag;
   fUseQuarter = quarterFlag;
   fUseShm = shmFlag;
}

BIActionInitialization::~BIActionInitialization()
{}

void BIActionInitialization::BuildForMaster() const
{
   SetUserAction(new BIRunAction(fForGrid, fUseShm));
}

void BIActionInitialization::Build() const
{
   SetUserAction(new BIPrimaryGeneratorAction(fBeamType, fForGrid, fUseQuarter));
   SetUserAction(new BIRunAction(fForGrid, fUseShm));
   SetUserAction(new BIEventAction(fForGrid));
}
