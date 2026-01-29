#include "EventAction.hh"
#include "RunAction.hh"
#include "G4AnalysisManager.hh"
#include "G4Event.hh"

EventAction::EventAction()
: G4UserEventAction(),
  fEdep(0.)
{}

EventAction::~EventAction()
{}

void EventAction::BeginOfEventAction(const G4Event*)
{
  fEdep = 0.;
}

void EventAction::EndOfEventAction(const G4Event*)
{
  auto analysisManager = G4AnalysisManager::Instance();

  // OPTIMIZACIÓN: Solo guardar si hubo impacto real (> 0).
  // Con 1 Millón de eventos, cada hilo verá al menos unos 300 impactos,
  // así que no habrá archivos vacíos y no fallará.
  if (fEdep > 0.) { 
      analysisManager->FillNtupleDColumn(0, fEdep);
      analysisManager->AddNtupleRow();
  }
}