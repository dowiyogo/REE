#include "SteppingAction.hh"
#include "DetectorConstruction.hh"
#include "EventAction.hh"  // <--- NECESARIO para ver AddEdep
#include "G4Step.hh"
#include "G4RunManager.hh"
#include "G4EventManager.hh" // <--- NECESARIO para encontrar el EventAction actual
#include "G4SystemOfUnits.hh"

SteppingAction::SteppingAction(const DetectorConstruction* detector)
: G4UserSteppingAction(),
  fDetConstruction(detector)
{}

SteppingAction::~SteppingAction()
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    G4Track* track = step->GetTrack();
    
    // Si la partícula acaba de ser creada por decaimiento radiactivo
    if (track->GetCreatorProcess() && 
        track->GetCreatorProcess()->GetProcessName() == "RadioactiveDecay" &&
        track->GetCurrentStepNumber() == 1) {
        
        // ¡TRUCO! Reseteamos el reloj para que el detector la vea AHORA.
        track->SetGlobalTime(0.0); 
        track->SetLocalTime(0.0);
    }
        
    // 1. Obtener el volumen donde ocurre el paso
    G4LogicalVolume* volume = step->GetPreStepPoint()->GetTouchableHandle()
                                  ->GetVolume()->GetLogicalVolume();

    // 2. Obtener el volumen sensible (Detector)
    // (Usamos tu lógica existente para recuperar el puntero si se perdió)
    const DetectorConstruction* detectorConstruction = fDetConstruction;
    if (!detectorConstruction) {
        detectorConstruction = static_cast<const DetectorConstruction*>
            (G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    }
    G4LogicalVolume* fScoringVolume = detectorConstruction->GetScoringVolume();

    // 3. Verificación: ¿Estamos en el detector?
    if (volume != fScoringVolume) return;

    // 4. CAMBIO CLAVE: Obtener Energía DEPOSITADA en este paso
    // (No la cinética en el borde, sino la que realmente se queda en el cristal)
    G4double edep = step->GetTotalEnergyDeposit();

    if (edep > 0.) {
        // 5. CAMBIO CLAVE: Pasarle la energía al EventAction
        // Recuperamos el EventAction actual a través del EventManager
        EventAction* eventAction = (EventAction*) G4EventManager::GetEventManager()->GetUserEventAction();
        
        if (eventAction) {
            eventAction->AddEdep(edep); // ¡Acumulamos!
        }
    }
    
    // NOTA: Aquí borramos el analysisManager->FillH1(0, edep)
    // porque ahora el EventAction se encarga de guardar al FINAL del evento.
}