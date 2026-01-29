#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "DetectorConstruction.hh"

ActionInitialization::ActionInitialization()
 : G4VUserActionInitialization()
{}

ActionInitialization::~ActionInitialization()
{}

// --- ESTO ARREGLA EL PROBLEMA DEL ARCHIVO VACÍO ---
void ActionInitialization::BuildForMaster() const
{
    // El Master necesita RunAction para gestionar el archivo final
    SetUserAction(new RunAction());
}

void ActionInitialization::Build() const
{
    SetUserAction(new PrimaryGeneratorAction);
    SetUserAction(new RunAction());  // Workers también necesitan uno
    
    EventAction* eventAction = new EventAction();
    SetUserAction(eventAction);
    
    SetUserAction(new SteppingAction(nullptr)); 
}