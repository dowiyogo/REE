#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh" // <--- CORREGIDO

class EventAction : public G4UserEventAction
{
  public:
    EventAction();
    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);

    // Función para acumular energía (llamada por SteppingAction)
    void AddEdep(G4double edep) { fEdep += edep; }

  private:
    G4double fEdep; // Variable para sumar energía total del evento
};

#endif