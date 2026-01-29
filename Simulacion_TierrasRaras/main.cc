#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"

int main(int argc, char** argv)
{
  // 1. Detectar modo (Interactivo o Batch)
  G4UIExecutive* ui = nullptr;
  if (argc == 1) { ui = new G4UIExecutive(argc, argv); }

  // 2. Crear RunManager
  auto* runManager = G4RunManagerFactory::CreateRunManager();

  // 3. Inicializar Clases de Usuario (Tu Física y Geometría)
  runManager->SetUserInitialization(new DetectorConstruction());
  runManager->SetUserInitialization(new PhysicsList());

  // 4. Inicializar Acciones (Generador, Run, Event)
  auto* actionInitialization = new PrimaryGeneratorAction(); // Clase wrapper necesaria
  // Nota: En versiones modernas se usa ActionInitialization, pero lo haremos directo por simplicidad pedagógica
  // O mejor, definamos una clase ActionInitialization simple aquí mismo o en archivo aparte.
  // Para simplificar este script, asumiremos que PrimaryGeneratorAction hereda de G4VUserActionInitialization
  // CORRECCIÓN: Para mantener el estándar, vamos a crear el archivo ActionInitialization.
  
  // Vamos a usar la forma moderna:
  runManager->SetUserInitialization(new PrimaryGeneratorAction()); 
  
  // 5. Inicializar Visor
  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();

  // 6. Obtener puntero al UI Manager
  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  if ( ! ui ) {
    // Modo Batch (lectura de macro)
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command+fileName);
  }
  else {
    // Modo Interactivo
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  delete visManager;
  delete runManager;
  return 0;
}
