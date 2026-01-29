#include "RunAction.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh" // Necesario para obtener el ID del Run

RunAction::RunAction() : G4UserRunAction()
{
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("root");
  analysisManager->SetVerboseLevel(1);
  
  // ACTIVAR FUSIÓN DE HILOS
  analysisManager->SetNtupleMerging(true); 

  analysisManager->CreateNtuple("Coincidencia", "Datos Tierras Raras");
  analysisManager->CreateNtupleDColumn("Energy");
  analysisManager->FinishNtuple();
}

RunAction::~RunAction()
{
  // --- CORRECCIÓN DEL SEGMENTATION FAULT ---
  // NO borres la instancia aquí. Geant4 maneja el ciclo de vida del Singleton.
  // delete G4AnalysisManager::Instance(); <--- ESTA LINEA CAUSABA EL CRASH
}

void RunAction::BeginOfRunAction(const G4Run* run)
{
  auto analysisManager = G4AnalysisManager::Instance();
  
  // --- CORRECCIÓN DE SOBRESCRITURA ---
  // Obtenemos el ID de la corrida (0 para Na22, 1 para Am241)
  G4int runID = run->GetRunID();
  
  // Convertimos a texto para crear un nombre único
  G4String fileName = "Salida_TierrasRaras_Run" + std::to_string(runID);
  
  // Esto creará: Salida_TierrasRaras_Run0.root Y Salida_TierrasRaras_Run1.root
  analysisManager->OpenFile(fileName);
}

void RunAction::EndOfRunAction(const G4Run*)
{
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
}