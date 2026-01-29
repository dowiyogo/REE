#ifndef PhysicsList_h
#define PhysicsList_h 1

#include "G4VModularPhysicsList.hh" // <--- CAMBIO IMPORTANTE

class PhysicsList: public G4VModularPhysicsList // <--- CAMBIO DE HERENCIA
{
  public:
    PhysicsList();
    virtual ~PhysicsList();

  public:
    virtual void SetCuts();
};

#endif