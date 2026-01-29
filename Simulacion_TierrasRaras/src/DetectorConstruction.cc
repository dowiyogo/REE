#include "DetectorConstruction.hh"
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Color.hh"

DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(),
  fScoringVolume(0)
{ }

DetectorConstruction::~DetectorConstruction()
{ }

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  // =============================================================
  // 1. DEFINICIÓN DE MATERIALES
  // =============================================================
  G4NistManager* nist = G4NistManager::Instance();

  // --- Elementos Químicos ---
  G4Element* elLa = new G4Element("Lantano", "La", 57., 138.905*g/mole);
  G4Element* elBr = new G4Element("Bromo",    "Br", 35., 79.904*g/mole);
  G4Element* elCe = new G4Element("Cerio",    "Ce", 58., 140.116*g/mole);
  G4Element* elCa = nist->FindOrBuildElement("Ca");
  G4Element* elP  = nist->FindOrBuildElement("P");
  G4Element* elO  = nist->FindOrBuildElement("O");
  G4Element* elF  = nist->FindOrBuildElement("F");

  // --- Material Detector: LaBr3(Ce) ---
  // Densidad: 5.08 g/cm3.
  G4Material* matLaBr3 = new G4Material("LaBr3", 5.08*g/cm3, 2);
  matLaBr3->AddElement(elLa, 1);
  matLaBr3->AddElement(elBr, 3);
  
  // --- Material Muestra Base: Apatito (Fluorapatito) Ca5(PO4)3F ---
  // Densidad: 3.19 g/cm3
  G4Material* matApatitoPuro = new G4Material("ApatitoBase", 3.19*g/cm3, 4);
  matApatitoPuro->AddElement(elCa, 5);
  matApatitoPuro->AddElement(elP, 3);
  matApatitoPuro->AddElement(elO, 12);
  matApatitoPuro->AddElement(elF, 1);

  // --- Material Muestra con Tierras Raras (Mix) ---
  // Simulación de una muestra con 1% de Cerio
  G4double densityMix = 3.20*g/cm3; 
  G4Material* matMuestra = new G4Material("Apatito_con_Ce", densityMix, 2);
  G4double fractionCe = 0.01; // 1% de Cerio (10,000 ppm)
  matMuestra->AddMaterial(matApatitoPuro, (1.0 - fractionCe));
  matMuestra->AddElement(elCe, fractionCe);

  // --- Material Aire ---
  G4Material* world_mat = nist->FindOrBuildMaterial("G4_AIR");

  // =============================================================
  // 2. VOLÚMENES Y GEOMETRÍA
  // =============================================================

  // --- A. El Mundo (Laboratorio) ---
  G4Box* solidWorld = new G4Box("World", 0.5*m, 0.5*m, 0.5*m);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, world_mat, "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0, true);

  // --- Parámetros de Geometría ---
  G4double distSourceToDet = 10.0*cm;    // Distancia Fuente -> Detector
  G4double distSourceToSample = 5.0*cm;  // Distancia Fuente -> Muestra

  // --- B. Detectores LaBr3 (Cilindros) ---
  // Dimensiones: 1.5 pulgadas de diámetro y largo
  G4double det_rad = 1.9*cm; 
  G4double det_len = 3.8*cm; 

  G4Tubs* solidDet = new G4Tubs("Det_Solid", 0, det_rad, det_len/2.0, 0, 360*deg);
  G4LogicalVolume* logicDet = new G4LogicalVolume(solidDet, matLaBr3, "Det_LV");

  // Estética: Pintar los detectores de Azul
  G4VisAttributes* visDet = new G4VisAttributes(G4Color(0.0, 0.0, 1.0)); // Azul
  visDet->SetForceSolid(true);
  logicDet->SetVisAttributes(visDet);

  // COLOCACIÓN FÍSICA:
  // Detector 1 (Tag - Izquierda) en Z = -10 cm
  new G4PVPlacement(0, G4ThreeVector(0, 0, -distSourceToDet), logicDet, "Detector_Tag", logicWorld, false, 0, true);

  // Detector 2 (Medida - Derecha) en Z = +10 cm
  new G4PVPlacement(0, G4ThreeVector(0, 0, +distSourceToDet), logicDet, "Detector_Measure", logicWorld, false, 1, true);

  // --- C. Muestra (Cilindro de Apatito) ---
  // Dimensiones: Disco de 6 cm diámetro, 2 cm espesor
  G4double sample_thick = 2.0*cm; 
  G4double sample_rad = 3.0*cm;

  G4Tubs* solidSample = new G4Tubs("Sample_Solid", 0, sample_rad, sample_thick/2.0, 0, 360*deg);
  G4LogicalVolume* logicSample = new G4LogicalVolume(solidSample, matMuestra, "Sample_LV");
  
  // Estética: Pintar la muestra de Amarillo
  G4VisAttributes* visSample = new G4VisAttributes(G4Color(1.0, 1.0, 0.0)); // Amarillo
  visSample->SetForceSolid(true);
  logicSample->SetVisAttributes(visSample);

  // COLOCACIÓN FÍSICA:
  // Muestra en Z = +5 cm (Entre la fuente y el Detector 2)
  new G4PVPlacement(0, G4ThreeVector(0, 0, distSourceToSample), logicSample, "Sample_Phys", logicWorld, false, 0, true);

  // Definir cuál volumen registra datos (Scoring) -> Detector 2
  fScoringVolume = logicDet;

  return physWorld;
}