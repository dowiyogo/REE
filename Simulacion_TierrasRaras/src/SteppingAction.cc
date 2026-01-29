#include "SteppingAction.hh"
#include "EventAction.hh"
#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(), fEventAction(eventAction)
{}

SteppingAction::~SteppingAction() {}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  // 1. Obtener volumen
  G4LogicalVolume* volume = step->GetPreStepPoint()->GetTouchableHandle()
                                ->GetVolume()->GetLogicalVolume();

  // 2. Verificar si es el detector (Comparación rápida)
  if (volume->GetName() != "Det_LV") return;

  // 3. Obtener energía
  G4double edep = step->GetTotalEnergyDeposit();

  // 4. Acumular solo si es > 0
  if (edep > 0.) {
      fEventAction->AddEdep(edep);
  }
}