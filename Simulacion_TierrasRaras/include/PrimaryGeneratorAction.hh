#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserActionInitialization.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4GeneralParticleSource.hh" // <--- CAMBIO IMPORTANTE: Antes era G4ParticleGun.hh

// ================================================================
// Clase Generadora Real (La que dispara)
// ================================================================
class PrimaryGenerator : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGenerator();
    virtual ~PrimaryGenerator();
    virtual void GeneratePrimaries(G4Event*);
  
  private:
    G4GeneralParticleSource* fParticleGun; // <--- CAMBIO IMPORTANTE: Tipo actualizado
};

// ================================================================
// Clase Inicializadora (La que conecta todo)
// ================================================================
class PrimaryGeneratorAction : public G4VUserActionInitialization
{
  public:
    PrimaryGeneratorAction();
    virtual ~PrimaryGeneratorAction();
    virtual void Build() const;
    virtual void BuildForMaster() const;
};

#endif