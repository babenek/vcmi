#ifndef HYPOTHETICAL_GAME_STATE_H
#define HYPOTHETICAL_GAME_STATE_H

#include <vector>
#include <set>

#include "../../hch/CHeroHandler.h"
#include "../../hch/CBuildingHandler.h"
#include "../../hch/CObjectHandler.h"
#include "AIObjectContainer.h"

namespace geniusai {

class CGeniusAI; // Predeclaration of main class.

class HypotheticalGameState {
public:
  struct HeroModel {
    HeroModel(void);
    HeroModel(const CGHeroInstance* h);
    int3 pos;
    int3 previouslyVisited_pos;
    int3 interestingPos;
    bool finished;
    si16 remainingMovement;
    const CGHeroInstance* h;
  };

  struct TownModel {
    TownModel(const CGTownInstance* t);
    const CGTownInstance* t;
    std::vector< std::pair< ui32, std::vector<ui32> > > creaturesToRecruit;
    CCreatureSet creaturesInGarrison;				//type, num
    bool hasBuilt;
  };

  HypotheticalGameState() {}; // TODO: No definitions in .h.
  HypotheticalGameState(CGeniusAI& ai);

  void update(CGeniusAI& ai);

  CGeniusAI* AI;
  std::vector<const CGHeroInstance*> AvailableHeroesToBuy;
  std::vector<si16> resourceAmounts;
  std::vector<HeroModel> heroModels;
  std::vector<TownModel> townModels;
  std::set<AIObjectContainer> knownVisitableObjects;
};
}
#endif