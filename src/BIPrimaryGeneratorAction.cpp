#include <fstream>
#include <math.h>
#include <time.h>

#include <TFile.h>
#include <TH2.h>
#include <TF1.h>
#include <TRandom3.h>

#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "Randomize.hh"
#include "G4Threading.hh"
#include "G4AutoLock.hh"
#include "g4root.hh"
#include "G4GenericMessenger.hh"

#include "BIPrimaryGeneratorAction.hpp"


G4int nEveInPGA = 0; // Global variable change to local? 
G4Mutex mutexInPGA = G4MUTEX_INITIALIZER;
/*
// For using ROOT classes.
// When using ROOT classes, it have to be thread local or mutex locked.
// I don't know _G4MT_TLS_ is truely needed or not.
// https://indico.cern.ch/event/226961/material-old/0/0?contribId=0
// In case without, this code looks like working well...
G4ThreadLocal TF1 *fEneFnc_G4MT_TLS_ = 0;
*/
BIPrimaryGeneratorAction::BIPrimaryGeneratorAction(BeamType beamType, G4bool gridFlag, G4bool quarterFlag)
   : G4VUserPrimaryGeneratorAction(),
     fParticleGun(nullptr),
     fInputFile(nullptr),
     fHisSource(nullptr),
     fEneFnc(nullptr),
     fAngFnc(nullptr)
{
   fBeamType = beamType;

   fForGrid = gridFlag;

   fUseQuarter = quarterFlag;
   if(fUseQuarter) fPhiLimit = 0.5 * CLHEP::pi;
   else fPhiLimit = 2. * CLHEP::pi;
   
   fDx = (458. - 78.) / 15.;
   fDy = (332 - 152) / (log10(60.) - log10(20.));

   G4AutoLock lock(&mutexInPGA);
   
   Int_t seed = G4UniformRand() * 1000000;
   G4cout << "Seed of PGA = " << seed << G4endl;
   gRandom->SetSeed(seed);
   
   G4int nPar = 1;
   fParticleGun = new G4ParticleGun(nPar);

   fZPosition = -300.*mm;
   //fZPosition = -160.*mm; // Minimum distance for new beam

   fParVec = G4ThreeVector(0., 0., 1.);
   
   G4ParticleTable *parTable = G4ParticleTable::GetParticleTable();
   G4ParticleDefinition *proton = parTable->FindParticle("proton");
   fParticleGun->SetParticleDefinition(proton);
   fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., fZPosition));
   fParticleGun->SetParticleMomentumDirection(fParVec);
   fParticleGun->SetParticleEnergy(fEnergy);

   fInputFile = new TFile("randomSource.root", "OPEN");
   fHisSource = (TH2D*)fInputFile->Get("HisMap");
   fHisSource->SetName("fHisSource");
   
   DefineCommands();

// Pointer of Function is not good for readable code?
// And also, use if statement does not make program slow.
   if(fBeamType == kFirstBeam){
      fEneFnc = new TF1("fncEne", "exp([0]*x)", 0., 100.);
      fEneFnc->SetParameter(0, -4.77205e-02);
      
      GunFuncPointer = &BIPrimaryGeneratorAction::FirstBeamGun;
   }
   else if(fBeamType == kSecondBeam)
      GunFuncPointer = &BIPrimaryGeneratorAction::SecondBeamGun;
   
   else if(fBeamType == kThirdBeam){
      fEneFnc = new TF1("fncEne", "exp([0]*x)", 0., 100.);
      fEneFnc->SetParameter(0, -4.77205e-02);
      
      fAngFnc = new TF1("fAngFnc", "exp([0]*x)", 0., 20.);
      fAngFnc->SetParameter(0, -8.98131e-02);

      GunFuncPointer = &BIPrimaryGeneratorAction::ThirdBeamGun;
   }
   else{
      G4cout << "Beam type is wrong.  Please check it." << G4endl;
      exit(0);
   }
}

BIPrimaryGeneratorAction::~BIPrimaryGeneratorAction()
{
   G4AutoLock lock(&mutexInPGA); // when closing file, this is needed
   if(fEneFnc != nullptr) {delete fEneFnc; fEneFnc = nullptr;}
   if(fAngFnc != nullptr) {delete fAngFnc; fAngFnc = nullptr;}
   if(fHisSource != nullptr) {delete fHisSource; fHisSource = nullptr;}
   if(fParticleGun != nullptr) {delete fParticleGun; fParticleGun = nullptr;}
   if(fInputFile->IsOpen()) fInputFile->Close();
}

void BIPrimaryGeneratorAction::GeneratePrimaries(G4Event *event)
{
   (this->*GunFuncPointer)();
   
   //fParticleGun->SetParticleMomentumDirection(fParVec);
   //fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., fZPosition));
   fParticleGun->SetParticleEnergy(fEnergy);
   fParticleGun->GeneratePrimaryVertex(event);

   if(!fForGrid){
      G4AnalysisManager *anaMan = G4AnalysisManager::Instance();
      anaMan->FillNtupleIColumn(1, 0, 11);
      anaMan->FillNtupleDColumn(1, 1, fEnergy);
      anaMan->FillNtupleDColumn(1, 2, fParVec.x());
      anaMan->FillNtupleDColumn(1, 3, fParVec.y());
      anaMan->FillNtupleDColumn(1, 4, fParVec.z());
      anaMan->AddNtupleRow(1);
   }
   
   G4AutoLock lock(&mutexInPGA);
   if (nEveInPGA++ % 10000 == 0)
      G4cout << nEveInPGA - 1 << " events done" << G4endl;
}

void BIPrimaryGeneratorAction::FirstBeamGun()
{
   fEnergy = fEneFnc->GetRandom() * MeV;

   G4double x = (G4UniformRand() * 12.5 - 6.25)*mm;
   G4double y = (G4UniformRand() * 48.75 - 24.375)*mm;

   fParticleGun->SetParticlePosition(G4ThreeVector(x, y, fZPosition));
}

void BIPrimaryGeneratorAction::SecondBeamGun()
{
   G4double x, y;
   fHisSource->GetRandom2(x, y);

   fEnergy = pow(10., (y - 152.) / fDy) * 20 * MeV;

   G4double theta = CLHEP::pi * ((x - 78.) / fDx) / 180.;
   G4double phi = G4UniformRand() * fPhiLimit;
   G4double vx = sin(theta) * cos(phi);
   G4double vy = sin(theta) * sin(phi);
   G4double vz = cos(theta);
   fParVec = G4ThreeVector(vx, vy, vz);
   fParticleGun->SetParticleMomentumDirection(fParVec);
}

void BIPrimaryGeneratorAction::ThirdBeamGun()
{
   fEnergy = fEneFnc->GetRandom() * MeV;
   
   G4double theta;
   if(fEnergy > 30.*MeV)
      theta = fAngFnc->GetRandom() * deg;
   else
      theta = acos(1. - G4UniformRand() * (1. - cos(20.*deg)));
   G4double phi = G4UniformRand() * fPhiLimit;
   G4double vx = sin(theta) * cos(phi);
   G4double vy = sin(theta) * sin(phi);
   G4double vz = cos(theta);
   fParVec = G4ThreeVector(vx, vy, vz);
   fParticleGun->SetParticleMomentumDirection(fParVec);
}

void BIPrimaryGeneratorAction::DefineCommands()
{
   fMessenger = new G4GenericMessenger(this, "/BI/Primary/", 
                                       "Beam control");

   // z position
   G4GenericMessenger::Command &sourceZCmd
      = fMessenger->DeclareMethodWithUnit("Z", "mm",
                                          &BIPrimaryGeneratorAction::SetSourceZ, 
                                          "Set the length of source.");

   sourceZCmd.SetParameterName("z", true);
   sourceZCmd.SetRange("z>=-1000. && z<1000.");
   sourceZCmd.SetDefaultValue("-300");
}

void BIPrimaryGeneratorAction::SetSourceZ(G4double z)
{
   fZPosition = z;
}
