#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh" 

RunAction::RunAction()
: G4UserRunAction()
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetVerboseLevel(1);
    analysisManager->SetNtupleMerging(true);
    
    if (G4Threading::IsMasterThread()) {
        G4cout << ">>> [Master] RunAction iniciado. Merging activado." << G4endl;
    }
}

RunAction::~RunAction()
{}

void RunAction::BeginOfRunAction(const G4Run*)
{
    auto analysisManager = G4AnalysisManager::Instance();
    
    // Crear NTuple SOLO si no existe (primera vez o después de Reset)
    if (analysisManager->GetNofNtuples() == 0) {
        analysisManager->CreateNtuple("Scoring", "Datos por Evento");
        analysisManager->CreateNtupleDColumn("Energy");
        analysisManager->FinishNtuple();
    }
    
    // Abrir archivo
    analysisManager->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run*)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();
    analysisManager->CloseFile();
    
    // NO usar Reset() aquí - causa problemas entre runs consecutivos
}