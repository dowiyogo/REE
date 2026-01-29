#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh" // <--- CORREGIDO

class EventAction; // Declaración anticipada

class SteppingAction : public G4UserSteppingAction
{
  public:
    SteppingAction(EventAction* eventAction);
    virtual ~SteppingAction();

    // Aquí ocurre la magia paso a paso
    virtual void UserSteppingAction(const G4Step*);

  private:
    EventAction* fEventAction;
};

#endif