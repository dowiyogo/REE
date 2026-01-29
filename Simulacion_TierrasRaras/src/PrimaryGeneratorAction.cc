#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "G4GeneralParticleSource.hh" // <--- CAMBIO: Usamos GPS
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "SteppingAction.hh" // <--- AGREGAR ESTO

// --- Inicialización ---
PrimaryGeneratorAction::PrimaryGeneratorAction()
: G4VUserActionInitialization()
{}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{}

void PrimaryGeneratorAction::BuildForMaster() const
{
  SetUserAction(new RunAction());
}

void PrimaryGeneratorAction::Build() const
{
  SetUserAction(new PrimaryGenerator());
  SetUserAction(new RunAction());
  
  // CUIDADO AQUÍ: El SteppingAction necesita puntero al EventAction
  EventAction* eventAction = new EventAction();
  SetUserAction(eventAction);
  
  SetUserAction(new SteppingAction(eventAction)); // <--- CONEXIÓN FINAL
}

// --- Generador Real ---
PrimaryGenerator::PrimaryGenerator()
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(0)
{
  // CAMBIO: Instanciamos G4GeneralParticleSource en vez de G4ParticleGun
  fParticleGun = new G4GeneralParticleSource();
}

PrimaryGenerator::~PrimaryGenerator()
{
  delete fParticleGun;
}

void PrimaryGenerator::GeneratePrimaries(G4Event* anEvent)
{
  fParticleGun->GeneratePrimaryVertex(anEvent);
}