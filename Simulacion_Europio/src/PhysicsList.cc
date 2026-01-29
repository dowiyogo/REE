#include "PhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4SystemOfUnits.hh" // <--- FALTABA ESTO PARA LEER 'mm'

PhysicsList::PhysicsList() 
: G4VModularPhysicsList() // <--- CAMBIO A MODULAR
{
  // Ahora sí podemos usar RegisterPhysics porque somos una G4VModularPhysicsList
  RegisterPhysics(new G4EmStandardPhysics_option4());
  RegisterPhysics(new G4DecayPhysics());
  RegisterPhysics(new G4RadioactiveDecayPhysics());
}

PhysicsList::~PhysicsList()
{ 
}

void PhysicsList::SetCuts()
{
  // Definir el rango de corte de producción (Production Cut)
  // Esto dice: "No generes partículas secundarias si viajan menos de 0.1 mm"
  SetDefaultCutValue(0.1*mm); 

  // Aplicar los cortes a las tablas de física
  G4VModularPhysicsList::SetCuts();
}