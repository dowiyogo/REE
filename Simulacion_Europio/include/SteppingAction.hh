#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class DetectorConstruction;

class SteppingAction : public G4UserSteppingAction
{
  public:
    SteppingAction(const DetectorConstruction* detector);
    virtual ~SteppingAction();

    virtual void UserSteppingAction(const G4Step*);

  private:
    const DetectorConstruction* fDetConstruction;
};

#endif