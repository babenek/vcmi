#ifndef TOWN_OBJECTIVE_H
#define TOWN_OBJECTIVE_H
//town objectives
//recruitHero
//buildBuilding
//recruitCreatures
//upgradeCreatures

#include "AIObjective.h"
#include "HypotheticalGameState.h"

namespace geniusai {
class TownObjective: public AIObjective {
public:
  TownObjective(const HypotheticalGameState& hgs,
                Type,
                HypotheticalGameState::TownModel*,
                si16 Which,
                CGeniusAI* AI);

  HypotheticalGameState hgs;
  HypotheticalGameState::TownModel* whichTown;
  si16 which;				// which hero, which building, which creature.

  bool operator<(const TownObjective& other) const;
  void fulfill(CGeniusAI&, HypotheticalGameState& hgs);
  HypotheticalGameState pretend(const HypotheticalGameState& hgs) {
    return hgs;
  };
  float getValue() const;
  void print() const;
private:
  mutable float _value;
  mutable float _cost;
};
}

#endif // TOWN_OBJECTIVE_H