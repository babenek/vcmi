#include "StdAfx.h"
#include "HypotheticalGameState.h"

#include "CGeniusAI.h"

using std::vector;

namespace geniusai {
HypotheticalGameState::HeroModel::HeroModel(void)
{
}

HypotheticalGameState::HeroModel::HeroModel(const CGHeroInstance* h)
  : h(h), finished(false)
{
  pos = h->getPosition(false);
  remainingMovement = h->movement;
}

HypotheticalGameState::TownModel::TownModel(const CGTownInstance* t)
  : t(t)
{
  hasBuilt = (t->builded > 0) ? true : false;
  creaturesToRecruit = t->creatures;
  creaturesInGarrison = t->army;
}

HypotheticalGameState::HypotheticalGameState(CGeniusAI& ai)
  : knownVisitableObjects(ai.knownVisitableObjects)
{
  AI = &ai;
  std::vector<const CGHeroInstance*> heroes = ai.call_back_->getHeroesInfo();	
  for (std::vector<const CGHeroInstance*>::iterator i = heroes.begin();
       i != heroes.end();
       i++)
    heroModels.push_back(HeroModel(*i));

  vector<const CGTownInstance*> towns = ai.call_back_->getTownsInfo();	
  for (vector <const CGTownInstance*>::iterator i = towns.begin();
    i != towns.end();
    i++) {
      if ( (*i)->tempOwner == ai.call_back_->getMyColor() )
        townModels.push_back(TownModel(*i));
  }

  if (ai.call_back_->howManyTowns() != 0) {
    AvailableHeroesToBuy = 
      ai.call_back_->getAvailableHeroes(ai.call_back_->getTownInfo(0,0));
  }

  for (si16 i = 0; i < 8; i++)
    resourceAmounts.push_back(ai.call_back_->getResourceAmount(i));
}

void HypotheticalGameState::update(CGeniusAI& ai)
{
  AI = &ai;
  knownVisitableObjects = ai.knownVisitableObjects;
  vector<HeroModel> oldModels = heroModels;
  heroModels.clear();

  vector<const CGHeroInstance*> heroes = ai.call_back_->getHeroesInfo();
  for (vector<const CGHeroInstance*>::iterator i = heroes.begin();
       i != heroes.end();
       i++)
    heroModels.push_back(HeroModel(*i));

  for (si16 i = 0; i < oldModels.size(); i++) {
    for (si16 j = 0; j < heroModels.size(); j++) {
      if (oldModels[i].h->subID == heroModels[j].h->subID) {
        heroModels[j].finished = oldModels[i].finished;
        heroModels[j].previouslyVisited_pos = oldModels[i].previouslyVisited_pos;
      }
    }
  }
  townModels.clear();
  std::vector<const CGTownInstance*> towns = ai.call_back_->getTownsInfo();	
  for (std::vector<const CGTownInstance*>::iterator i = towns.begin();
    i != towns.end();
    i++) {
      if ( (*i)->tempOwner == ai.call_back_->getMyColor() )
        townModels.push_back(TownModel(*i));
  }

  if (ai.call_back_->howManyTowns() != 0) {
    AvailableHeroesToBuy =
      ai.call_back_->getAvailableHeroes(ai.call_back_->getTownInfo(0,0));
  }
  resourceAmounts.clear();
  for (si16 i = 0; i < 8; i++)
    resourceAmounts.push_back(ai.call_back_->getResourceAmount(i));
}

}