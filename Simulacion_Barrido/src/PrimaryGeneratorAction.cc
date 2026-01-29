#include "PrimaryGeneratorAction.hh"

#include "G4Event.hh"
#include "G4GeneralParticleSource.hh" // Usamos GPS
#include "G4SystemOfUnits.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"

// --- CONSTRUCTOR ---
// Nota: Aquí estaba tu error, decías "PrimaryGenerator::" en lugar de "PrimaryGeneratorAction::"
PrimaryGeneratorAction::PrimaryGeneratorAction()
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(nullptr)
{
    // Crear la fuente (GPS)
    fParticleGun = new G4GeneralParticleSource();

    // Configuración por defecto (opcional, por seguridad)
    // El resto se controla desde la macro .mac
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle("gamma");
    fParticleGun->SetParticleDefinition(particle);
}

// --- DESTRUCTOR ---
PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

// --- GENERATE PRIMARIES ---
// Esta es la única función que realmente importa aquí
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
    // Le decimos al GPS que dispare un vértice en este evento
    fParticleGun->GeneratePrimaryVertex(anEvent);
}