#include "StdAfx.h"
#include "TownObjective.h"

#include "../../lib/VCMI_Lib.h" // VCL->*
#include "../../lib/CGameState.h" // struct UpgradeInfo
#include "CGeniusAI.h"

using std::vector;
using std::cout;
using std::endl;

namespace geniusai {
TownObjective::TownObjective(const HypotheticalGameState& hgs,
                             Type t,
                             HypotheticalGameState::TownModel* tn,
                             si16 Which,
                             CGeniusAI * ai)
                             : whichTown(tn), which(Which), hgs(hgs)
{
  AI = ai;
  type = t;
  _value = -1;
}

float TownObjective::getValue() const
{
  if (_value >= 0)
    return _value - _cost;

  // TODO: Include a constant stating the meaning of 8 (number of resources).
  vector<si16> resourceCosts(8,0);
  CBuilding* b        = NULL;
  CCreature* creature = NULL;
  float cost          = 0; // TODO: Needed?
  si16 ID              = 0;
  si16 newID           = 0;
  si16 howMany         = 0;
  ui32 creatures_max  = 0;

  switch (type) {
    case recruitHero:
      resourceCosts[6] = 2500;  // TODO: Define somehow the meaning of gold etc.
      break;

    case buildBuilding:
      b = VLC->buildh->buildings[whichTown->t->subID][which];
      for (si16 i = 0;
        (b != NULL) && (i < b->resources.size());
        i++)
        resourceCosts[i] = b->resources[i];
      break;

    case recruitCreatures:
      // Buy upgraded if possible.
      ID = whichTown->creaturesToRecruit[which].second.back();
      creature = &VLC->creh->creatures[ID];
      howMany = whichTown->creaturesToRecruit[which].first;
      creatures_max = 0; // Max creatures you can recruit of this type.

      for (si16 i = 0; i < creature->cost.size(); i++) {
        if (creature->cost[i] != 0)
          creatures_max = hgs.resourceAmounts[i]/creature->cost[i];
        else
          creatures_max = INT_MAX; // TODO: Will have to rewrite it.
        // TODO: Buy the best units (the least I can buy)?
        amin(howMany, creatures_max);
      }
      // The cost of recruiting the stack of creatures.
      for (si16 i = 0;
        creature && ( i < creature->cost.size() ); // TODO: Creature what??
        i++)
        resourceCosts[i] = creature->cost[i]*howMany;
      break;

    case upgradeCreatures:
      UpgradeInfo ui = AI->call_back_->getUpgradeInfo(whichTown->t,which);
      ID = whichTown->creaturesInGarrison.slots[which].first;
      howMany = whichTown->creaturesInGarrison.slots[which].second;

      newID = ui.newID.back();
      si16 upgrade_serial = ui.newID.size() - 1;
      // TODO: Rewrite the integers in the CGameState class into boost 
      // equivalent.
      for (std::set< std::pair<int, int> >::iterator
        j = ui.cost[upgrade_serial].begin();
        j != ui.cost[upgrade_serial].end();
      j++)
        resourceCosts[j->first] = j->second*howMany;
      break;
  }

  _cost = AI->priorities_->getCost(resourceCosts, NULL, 0);
  _value = AI->priorities_->getValue(*this);
  return _value - _cost;
}

bool TownObjective::operator<(const TownObjective& other) const
{
  if (type != other.type)
    return type < other.type;
  else if (which != other.which)
    return which < other.which;
  else if (whichTown->t->id != other.whichTown->t->id)
    return whichTown->t->id < other.whichTown->t->id;
  else
    return false;
}

void TownObjective::fulfill(CGeniusAI& cg,
                                       HypotheticalGameState& hgs)
{
  cg.call_back_->waitTillRealize = true;
  CBuilding* b;
  const CCreature* creature;
  HypotheticalGameState::HeroModel hm;
  si16 ID;
  si16 howMany;
  si16 newID;
  si16 hSlot;

  switch (type) {
  case recruitHero:
    cg.call_back_->recruitHero(whichTown->t, hgs.AvailableHeroesToBuy[which]);
    hm = HypotheticalGameState::HeroModel(hgs.AvailableHeroesToBuy[which]);
    hm.pos = whichTown->t->getSightCenter();
    hm.remainingMovement = hm.h->maxMovePoints(true);
    hgs.heroModels.push_back(hm);
    hgs.resourceAmounts[6] -= 2500;
    break;

  case buildBuilding:
    b = VLC->buildh->buildings[whichTown->t->subID][which];
    if (cg.call_back_->canBuildStructure(whichTown->t,which) == 7) {
      cout << "built " << b->Name() << "." << endl;
      if (!cg.call_back_->buildBuilding(whichTown->t, which))
        cout << "really tried to build unbuildable building" << endl;
      for (si16 i = 0; b && i < b->resources.size(); i++) // TODO: b what?
        hgs.resourceAmounts[i] -= b->resources[i];
    } else
      cout << "trying to build a structure we cannot build" << endl;
    whichTown->hasBuilt=true;
    break;

  case recruitCreatures:
    // Buy upgraded if possible.
    ID = whichTown->creaturesToRecruit[which].second.back();
    creature = &VLC->creh->creatures[ID];
    howMany = whichTown->creaturesToRecruit[which].first;
    for (si16 i = 0; i < creature->cost.size(); i++)
      // TODO: rewrite.
      amin(howMany, creature->cost[i] ? hgs.resourceAmounts[i]/creature->cost[i] : INT_MAX);
    if (howMany == 0)
      cout << "tried to recruit without enough money.";
    cout << "recruiting " << howMany  << " "
      << creature->namePl << " (Total AI Strength "
      << creature->AIValue*howMany << ")." << endl;
    cg.call_back_->recruitCreatures(whichTown->t, ID, howMany);
    break;

  case upgradeCreatures:
    UpgradeInfo ui = cg.call_back_->getUpgradeInfo(whichTown->t, which);
    ID = whichTown->creaturesInGarrison.slots[which].first;
    newID = ui.newID.back();
    // TODO: reduce resources in hgs
    cg.call_back_->upgradeCreature(whichTown->t, which, newID);
    cout << "upgrading " << VLC->creh->creatures[ID].namePl << endl;
    break;
  }
}

void TownObjective::print() const
{
  HypotheticalGameState::HeroModel hm;
  CBuilding* b               = NULL;
  const CCreature* creature  = NULL;
  si16 ID                    = 0;
  si16 howMany               = 0;
  si16 newID                 = 0; // TODO: Needed?
  si16 hSlot                 = 0; // TODO: Needed?
  ui32 creatures_max;

  switch (type) {
    case recruitHero:
      cout << "recruit hero.";
      break;

    case buildBuilding:
      b = VLC->buildh->buildings[whichTown->t->subID][which];
      cout << "build " << b->Name() << " cost = ";
      if (b->resources.size() != 0) {
        if (b->resources[0] != 0)
          cout << b->resources[0] << " wood. ";
        if (b->resources[1] != 0)
          cout << b->resources[1] << " mercury. ";
        if (b->resources[2] != 0)
          cout << b->resources[2] << " ore. ";
        if (b->resources[3] != 0)
          cout << b->resources[3] << " sulfur. ";
        if (b->resources[4] != 0)
          cout << b->resources[4] << " crystal. ";
        if (b->resources[5] != 0)
          cout << b->resources[5] << " gems. ";
        if (b->resources[6] != 0)
          cout << b->resources[6] << " gold. ";
      }
      break;

    case recruitCreatures:
      // Buy upgraded if possible.
      ID = whichTown->creaturesToRecruit[which].second.back();
      creature = &VLC->creh->creatures[ID];
      howMany = whichTown->creaturesToRecruit[which].first;
      creatures_max = 0;

      for (si16 i = 0; i < creature->cost.size(); i++) {
        if (creature->cost[i] != 0)
          creatures_max = hgs.resourceAmounts[i]/creature->cost[i];
        else
          creatures_max = INT_MAX;
        amin(howMany, creatures_max);
      }
      cout << "recruit " << howMany  << " " << creature->namePl
           << " (Total AI Strength " << creature->AIValue*howMany
           << "). cost = ";

      if (creature->cost.size() != 0) {
        if (creature->cost[0] != 0)
          cout << creature->cost[0]*howMany << " wood. ";
        if (creature->cost[1] != 0)
          cout << creature->cost[1]*howMany << " mercury. ";
        if (creature->cost[2] != 0)
          cout << creature->cost[2]*howMany << " ore. ";
        if (creature->cost[3] != 0)
          cout << creature->cost[3]*howMany << " sulfur. ";
        if (creature->cost[4] != 0)
          cout << creature->cost[4]*howMany << " cristal. ";
        if (creature->cost[5] != 0)
          cout << creature->cost[5]*howMany << " gems. ";
        if (creature->cost[6] != 0)
          cout << creature->cost[6]*howMany << " gold. ";
      }
      break; // case recruitCreatures.

    case upgradeCreatures:
      UpgradeInfo ui = AI->call_back_->getUpgradeInfo(whichTown->t,which);
      ID = whichTown->creaturesInGarrison.slots[which].first;
      cout << "upgrade " << VLC->creh->creatures[ID].namePl;
      //ui.cost	
      break;
  } // switch(type)
}
} // namespace geniusai