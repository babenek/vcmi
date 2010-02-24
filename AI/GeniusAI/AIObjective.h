#ifndef AI_OBJECTIVE_H
#define AI_OBJECTIVE_H

namespace geniusai {
// Predeclarations.
class HypotheticalGameState;
class CGeniusAI;

class AIObjective {
public: 
  enum Type {
    // Hero objectives.
    visit,				// done TODO: upon visit friendly hero, trade
    attack,				// done
    //flee,
    dismissUnits,
    dismissYourself,
    rearangeTroops,
    finishTurn, // uses up remaining motion to get somewhere interesting.
    // Town objectives.
    recruitHero,
    buildBuilding,
    recruitCreatures,
    upgradeCreatures
  };

  CGeniusAI* AI;
  Type type;
  virtual void fulfill(CGeniusAI&, HypotheticalGameState& hgs) = 0;
  virtual HypotheticalGameState pretend(const HypotheticalGameState&) = 0;
  virtual void print() const = 0;
  virtual float getValue() const = 0;	//how much is it worth to the AI to achieve
};
}

#endif