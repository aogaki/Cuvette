#include <stdio.h>
#include <string>
#include <fstream>

#include "G4SystemOfUnits.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Element.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4RotationMatrix.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4CutTubs.hh"
#include "G4GenericMessenger.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4UImanager.hh"
#include "G4UnionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4IntersectionSolid.hh"

#include "BIDetectorConstruction.hpp"
#include "BICommonSD.hpp"
#include "BISmallSD.hpp"

#ifdef NOTCPP11
#include <sstream>
G4double stol(std::string buf)
{
   std::istringstream is;
   is.str(buf);
   G4double retVal;
   is >> retVal;
   return retVal;
}

G4String itos(const G4int val)
{
  std::ostringstream oss;
  oss << val;
  return oss.str();
}

#endif

BIDetectorConstruction::BIDetectorConstruction(G4bool forGrid, G4bool useTile)
   : G4VUserDetectorConstruction(),
     fWorldLV(nullptr),
     fVacuumMat(nullptr),
     fAirMat(nullptr),
     fCuvetteMat(nullptr),
     fWaterMat(nullptr),
     fCoverMat(nullptr),
     fTissueMat(nullptr)
{
   fCut = false;
   fCheckOverlap = true;

   fForGrid = forGrid;
   fUseTileAtt = useTile;
   
   DefineMaterial();
}

BIDetectorConstruction::~BIDetectorConstruction()
{}

void BIDetectorConstruction::DefineMaterial()
{
   G4NistManager *manager = G4NistManager::Instance();

   // NIST database materials
   fVacuumMat = manager->FindOrBuildMaterial("G4_Galactic");
   fAirMat = manager->FindOrBuildMaterial("G4_AIR");
   fWaterMat = manager->FindOrBuildMaterial("G4_WATER");
   fCoverMat = manager->FindOrBuildMaterial("G4_TEFLON");
   fCuvetteMat = manager->BuildMaterialWithNewDensity("G4_OPTICAL_GLASS", "G4_GLASS_PLATE",
                                                   2.55*g/cm3);
   
   // This shuld be changed (i.e. fLangMat, fHeartMat, or etc.)
   fTissueMat = manager->BuildMaterialWithNewDensity("G4_WATER_MODIFIED","G4_WATER",
                                                     1.1*g/cm/cm/cm);

}

G4VPhysicalVolume *BIDetectorConstruction::Construct()
{
   // world volume
   G4double worldDx = 2.*m;
   G4double worldDy = 2.*m;
   G4double worldDz = 2.*m;

   G4Box *worldS = new G4Box("World", worldDx / 2., worldDy / 2., worldDz / 2.);
   fWorldLV = new G4LogicalVolume(worldS, fVacuumMat, "World");

   G4VisAttributes *visAttributes = new G4VisAttributes(G4Colour::White());
   visAttributes->SetVisibility(false);
   fWorldLV->SetVisAttributes(visAttributes);
   fVisAttributes.push_back(visAttributes);

   G4VPhysicalVolume *worldPV
      = new G4PVPlacement(nullptr, G4ThreeVector(), fWorldLV, "World", 0,
                          false, 0, fCheckOverlap);

   // geometry parameters for cuvette
   G4double innerW = 10.*mm; // X direction
   G4double innerL = 10.*mm; // Z
   G4double innerH = 46.*mm; // Y
   G4double cuvetteT = 1.25*mm; 
   G4double coverT = cuvetteT * 2;
   
   G4double cuvetteW = innerW + cuvetteT * 2;
   G4double cuvetteL = innerL + cuvetteT * 2;
   G4double cuvetteH = innerH + cuvetteT + coverT; // one side is replaced by cover
   
   G4Box *cuvetteS = new G4Box("Cuvette", cuvetteW / 2., cuvetteH / 2., cuvetteL / 2.);
   G4LogicalVolume *cuvetteLV = new G4LogicalVolume(cuvetteS, fCuvetteMat, "Cuvette");
   visAttributes = new G4VisAttributes(G4Colour::White());
   cuvetteLV->SetVisAttributes(visAttributes);
   fVisAttributes.push_back(visAttributes);

   // inside the cuvette
   // filled by water.  (more high density is better?)
   G4Box *waterS = new G4Box("Water", innerW / 2., innerH / 2., innerL / 2.);
   G4LogicalVolume *waterLV = new G4LogicalVolume(waterS, fWaterMat, "Water");
   visAttributes = new G4VisAttributes(G4Colour::Blue());
   waterLV->SetVisAttributes(visAttributes);
   fVisAttributes.push_back(visAttributes);

   // tissues
   G4double tissueDia = 8.*mm;
   G4double tissueT = 200.*um;
   G4Tubs *tissueS = new G4Tubs("Tissue", 0., tissueDia / 2., tissueT / 2., 0., 360.*deg);
   G4LogicalVolume *tissueLV = new G4LogicalVolume(tissueS, fTissueMat, "Tissue");
   visAttributes = new G4VisAttributes(G4Colour::Magenta());
   tissueLV->SetVisAttributes(visAttributes);
   fVisAttributes.push_back(visAttributes);

   G4ThreeVector tissuePos = G4ThreeVector(0., 0., 0.);
   new G4PVPlacement(nullptr, tissuePos, tissueLV, "Tissue", cuvetteLV,
                     false, 0, fCheckOverlap);
   
   
   G4ThreeVector waterPos = G4ThreeVector(0., -(coverT - cuvetteT) / 2., 0.);
   new G4PVPlacement(nullptr, waterPos, waterLV, "Water", cuvetteLV,
                     false, 0, fCheckOverlap);

   // cover of cuvette
   G4Box *coverS = new G4Box("Cover", cuvetteW / 2., coverT / 2., cuvetteL / 2.);
   G4LogicalVolume *coverLV = new G4LogicalVolume(coverS, fCoverMat, "Cover");
   visAttributes = new G4VisAttributes(G4Colour::Green());
   coverLV->SetVisAttributes(visAttributes);
   fVisAttributes.push_back(visAttributes);
   
   G4ThreeVector coverPos = G4ThreeVector(0., (cuvetteH - coverT) / 2., 0.);
   new G4PVPlacement(nullptr, coverPos, coverLV, "Cover", cuvetteLV,
                     false, 0, fCheckOverlap);

   

   
   G4ThreeVector cuvettePos = G4ThreeVector(0., 0., 0.);
   new G4PVPlacement(nullptr, cuvettePos, cuvetteLV, "Cuvette", fWorldLV,
                     false, 0, fCheckOverlap);
   
   
   return worldPV;
}

void BIDetectorConstruction::ConstructSDandField()
{
   if(fForGrid){
      G4VSensitiveDetector *SmallSD = new BICommonSD("Small",
                                                     "SmallHitsCollection");
      SetSensitiveDetector("Cell", SmallSD);
   }
   else{
      G4VSensitiveDetector *CommonSD = new BICommonSD("Common",
                                                      "CommonHitsCollection");
 
      G4LogicalVolumeStore *lvStore = G4LogicalVolumeStore::GetInstance();
      std::vector<G4LogicalVolume*>::const_iterator it;
      for(it = lvStore->begin(); it != lvStore->end(); it++){
         if((*it)->GetName() != "World")
            SetSensitiveDetector((*it)->GetName(), CommonSD);
      }
   }

}
