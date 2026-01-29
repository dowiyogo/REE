#!/bin/bash

# Nombre del proyecto
PROJECT_NAME="Simulacion_TierrasRaras"

echo "=== Creando estructura del proyecto: $PROJECT_NAME ==="

# 1. Crear directorios
mkdir -p $PROJECT_NAME/include
mkdir -p $PROJECT_NAME/src
mkdir -p $PROJECT_NAME/build

cd $PROJECT_NAME

# ==========================================================
# 2. GENERAR ARCHIVO CMakeLists.txt
# ==========================================================
cat <<EOF > CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project($PROJECT_NAME)

# Buscar Geant4
find_package(Geant4 REQUIRED ui_all vis_all)

# Configurar compilación
include(\${Geant4_USE_FILE})
include_directories(\${PROJECT_SOURCE_DIR}/include)

# Buscar todos los archivos fuente y cabeceras
file(GLOB sources \${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers \${PROJECT_SOURCE_DIR}/include/*.hh)

# Crear el ejecutable
add_executable($PROJECT_NAME main.cc \${sources} \${headers})
target_link_libraries($PROJECT_NAME \${Geant4_LIBRARIES})

# Copiar macros al directorio de construcción
set(MACROS init_vis.mac run.mac)
foreach(macro \${MACROS})
  configure_file(\${PROJECT_SOURCE_DIR}/\${macro} \${PROJECT_BINARY_DIR}/\${macro} COPYONLY)
endforeach()

echo "CMakeLists.txt generado."
EOF

# ==========================================================
# 3. GENERAR main.cc
# ==========================================================
cat <<EOF > main.cc
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
EOF
echo "main.cc generado."

# ==========================================================
# 4. GENERAR HEADERS (.hh)
# ==========================================================

# --- DetectorConstruction.hh ---
cat <<EOF > include/DetectorConstruction.hh
#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "G4VPhysicalVolume.hh"

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    virtual ~DetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    
    G4LogicalVolume* GetScoringVolume() const { return fScoringVolume; }

  protected:
    G4LogicalVolume* fScoringVolume;
};
#endif
EOF

# --- PhysicsList.hh ---
cat <<EOF > include/PhysicsList.hh
#ifndef PhysicsList_h
#define PhysicsList_h 1

#include "G4VUserPhysicsList.hh"

class PhysicsList: public G4VUserPhysicsList
{
  public:
    PhysicsList();
    virtual ~PhysicsList();

  protected:
    virtual void ConstructParticle();
    virtual void ConstructProcess();
    virtual void SetCuts();
};
#endif
EOF

# --- RunAction.hh ---
cat <<EOF > include/RunAction.hh
#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4Run.hh"

class RunAction : public G4UserRunAction
{
  public:
    RunAction();
    virtual ~RunAction();

    virtual void BeginOfRunAction(const G4Run*);
    virtual void   EndOfRunAction(const G4Run*);
};
#endif
EOF

# --- EventAction.hh ---
cat <<EOF > include/EventAction.hh
#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "G4Event.hh"

class EventAction : public G4UserEventAction
{
  public:
    EventAction();
    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event*);
    virtual void   EndOfEventAction(const G4Event*);
};
#endif
EOF

# --- PrimaryGeneratorAction.hh (Modificado para ser ActionInitialization) ---
# Usaremos el estandar moderno ActionInitialization para instanciar Run/Event Actions
cat <<EOF > include/PrimaryGeneratorAction.hh
#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserActionInitialization.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"

// Clase Generadora real
class PrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGenerator();
    virtual ~PrimaryGenerator();
    virtual void GeneratePrimaries(G4Event*);
  
  private:
    G4ParticleGun* fParticleGun;
};

// Clase Inicializadora (La que llama main.cc)
class PrimaryGeneratorAction : public G4VUserActionInitialization
{
  public:
    PrimaryGeneratorAction();
    virtual ~PrimaryGeneratorAction();
    virtual void Build() const;
    virtual void BuildForMaster() const;
};
#endif
EOF

# ==========================================================
# 5. GENERAR SOURCES (.cc)
# ==========================================================

# --- DetectorConstruction.cc ---
cat <<EOF > src/DetectorConstruction.cc
#include "DetectorConstruction.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(), fScoringVolume(0)
{ }

DetectorConstruction::~DetectorConstruction()
{ }

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  // AQUI PEGARAS TU CODIGO DE LABR3 Y APATITO
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* world_mat = nist->FindOrBuildMaterial("G4_AIR");
  G4Box* solidWorld = new G4Box("World", 0.5*m, 0.5*m, 0.5*m);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, world_mat, "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0, true);
  return physWorld;
}
EOF

# --- PhysicsList.cc ---
cat <<EOF > src/PhysicsList.cc
#include "PhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"

PhysicsList::PhysicsList() : G4VUserPhysicsList()
{
  RegisterPhysics(new G4EmStandardPhysics_option4());
  RegisterPhysics(new G4DecayPhysics());
  RegisterPhysics(new G4RadioactiveDecayPhysics());
}

PhysicsList::~PhysicsList() { }
void PhysicsList::ConstructParticle() { G4VUserPhysicsList::ConstructParticle(); }
void PhysicsList::ConstructProcess() { G4VUserPhysicsList::ConstructProcess(); }
void PhysicsList::SetCuts() { SetCutsWithDefaultValue(1.0*mm); }
EOF

# --- RunAction.cc ---
cat <<EOF > src/RunAction.cc
#include "RunAction.hh"
#include "g4root.hh"

RunAction::RunAction() : G4UserRunAction()
{
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->CreateNtuple("Coincidencia", "Datos Tierras Raras");
  analysisManager->CreateNtupleDColumn("Energy");
  analysisManager->FinishNtuple();
}

RunAction::~RunAction() { delete G4AnalysisManager::Instance(); }

void RunAction::BeginOfRunAction(const G4Run*)
{
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->OpenFile("Salida_TierrasRaras.root");
}

void RunAction::EndOfRunAction(const G4Run*)
{
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
}
EOF

# --- EventAction.cc ---
cat <<EOF > src/EventAction.cc
#include "EventAction.hh"
#include "RunAction.hh"
#include "g4root.hh"

EventAction::EventAction() : G4UserEventAction() { }
EventAction::~EventAction() { }
void EventAction::BeginOfEventAction(const G4Event*) { }
void EventAction::EndOfEventAction(const G4Event*) { }
EOF

# --- PrimaryGeneratorAction.cc ---
cat <<EOF > src/PrimaryGeneratorAction.cc
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "G4SystemOfUnits.hh"

// --- Inicializador ---
PrimaryGeneratorAction::PrimaryGeneratorAction() : G4VUserActionInitialization() {}
PrimaryGeneratorAction::~PrimaryGeneratorAction() {}

void PrimaryGeneratorAction::BuildForMaster() const {
  SetUserAction(new RunAction());
}

void PrimaryGeneratorAction::Build() const {
  SetUserAction(new PrimaryGenerator());
  SetUserAction(new RunAction());
  SetUserAction(new EventAction());
}

// --- Generador Real ---
PrimaryGenerator::PrimaryGenerator() : G4VUserPrimaryGeneratorAction()
{
  fParticleGun = new G4ParticleGun(1);
  fParticleGun->SetParticleEnergy(511*keV);
}

PrimaryGenerator::~PrimaryGenerator() { delete fParticleGun; }

void PrimaryGenerator::GeneratePrimaries(G4Event* anEvent)
{
  fParticleGun->GeneratePrimaryVertex(anEvent);
}
EOF

# ==========================================================
# 6. GENERAR MACROS (.mac)
# ==========================================================
cat <<EOF > init_vis.mac
/control/verbose 2
/vis/open OGL 600x600-0+0
/vis/drawVolume
/vis/viewer/set/viewpointThetaPhi 90 180
/vis/scene/add/trajectories smooth
/vis/scene/endOfEventAction accumulate
EOF

cat <<EOF > run.mac
/run/initialize
/run/beamOn 1000
EOF

echo "=== Estructura creada exitosamente en $PROJECT_NAME ==="
echo "Pasos siguientes:"
echo "1. cd $PROJECT_NAME/build"
echo "2. cmake .."
echo "3. make"
echo "4. ./$PROJECT_NAME"
