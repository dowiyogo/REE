#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh" // <--- Usamos la nueva clase

int main(int argc, char** argv)
{
  // 1. Detectar modo (Interactivo o Batch)
  G4UIExecutive* ui = nullptr;
  if (argc == 1) { ui = new G4UIExecutive(argc, argv); }

  // 2. Crear RunManager
  // Si compilaste con MT, esto crea un G4MTRunManager automáticamente
  auto* runManager = G4RunManagerFactory::CreateRunManager();
  
  // Opcional: Forzar número de hilos si quieres (ej. 16)
  runManager->SetNumberOfThreads(16); 

  // 3. Inicializar Clases Obligatorias
  runManager->SetUserInitialization(new DetectorConstruction());
  runManager->SetUserInitialization(new PhysicsList());
  
  // 4. Inicializar Acciones (Aquí conectamos el ActionInitialization que creamos)
  runManager->SetUserInitialization(new ActionInitialization());

  // 5. Inicializar Visor
  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();

  // 6. Interfaz de Usuario
  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  if ( ! ui ) {
    // Modo Batch (ejecutar macro y salir)
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command+fileName);
  }
  else {
    // Modo Interactivo (abrir ventana gráfica)
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  // 7. Limpieza
  delete visManager;
  delete runManager;
  
  return 0;
}