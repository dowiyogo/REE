#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

// --- SECCIÓN DE DECLARACIONES ANTICIPADAS (AQUÍ ESTABA EL ERROR 1) ---
// Esto arregla el error "G4Material does not name a type"
class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;           // <--- Necesario para fApatiteWithREE
class G4GenericMessenger;   // <--- Necesario para fMessenger

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    virtual ~DetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    
    // --- NUEVA FUNCIÓN (AQUÍ ESTABA EL ERROR 2) ---
    // Debes declarar la función aquí para que el .cc sepa que pertenece a la clase
    void SetREEConcentration(G4double fraction);
    // NUEVO: Función para que el SteppingAction sepa cuál es el detector
    G4LogicalVolume* GetScoringVolume() const { return fLogicDetector; }

  private:
    void DefineMaterials();

    // Volúmenes lógicos que queremos manipular
    G4LogicalVolume* fLogicSample; 

    // --- NUEVAS VARIABLES (AQUÍ ESTABA EL ERROR 3) ---
    G4Material* fApatiteWithREE; // El material dinámico
    G4GenericMessenger* fMessenger;      // El comunicador con la macro
    G4double            fREEFraction;    // La variable de concentración
    // NUEVO: Variable para guardar el detector
    G4LogicalVolume* fLogicDetector;
};

#endif