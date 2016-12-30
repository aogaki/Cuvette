#ifndef BIDetectorConstruction_h
#define BIDetectorConstruction_h 1

#ifdef NOTCPP11
#define nullptr NULL
#endif

#include <vector>

#include "G4VUserDetectorConstruction.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VSolid.hh"
#include "G4Material.hh"
#include "G4VisAttributes.hh"
#include "G4LogicalVolume.hh"
#include "G4GenericMessenger.hh"


class BIDetectorConstruction : public G4VUserDetectorConstruction
{
public:
   BIDetectorConstruction(G4bool forGrid);
   virtual ~BIDetectorConstruction();

   virtual G4VPhysicalVolume *Construct();
   virtual void ConstructSDandField();

private:
   G4bool fCut;
   G4bool fForGrid;
   
   G4LogicalVolume *fWorldLV;
   G4bool fCheckOverlap;

   // Materials
   void DefineMaterial();
   G4Material *fVacuumMat;
   G4Material *fAirMat;
   G4Material *fCuvetteMat;
   G4Material *fWaterMat;
   G4Material *fCoverMat;
   G4Material *fTissueMat;

   G4Material *fQuartzMat;
   G4Material *fPlasticMat;
   G4Material *fGlassMat;
   
   // Geometries
   G4LogicalVolume *ConstructCuvette();
   std::vector<G4VisAttributes *> fVisAttributes;
   
   // Commands
   void DefineCommands();
   void SetRotationY(G4double rotY); // in degree
   G4GenericMessenger *fMessenger;
   G4VPhysicalVolume *fCuvettePV;
};

#endif
