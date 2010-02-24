#include "StdAfx.h"
#include "HeroObjective.h"
#include "HypotheticalGameState.h"
#include "CGeniusAI.h"

using std::cout;
using std::endl;
using std::vector;

namespace geniusai {
HeroObjective::HeroObjective(const HypotheticalGameState& hgs,
                             Type t,
                             const CGObjectInstance* object,
                             HypotheticalGameState::HeroModel* h,
                             CGeniusAI* ai) : object(object), hgs(hgs)
{
  AI = ai;
  pos = object->pos;
  type = t;
  whoCanAchieve.push_back(h);
  _value = -1;
}

void HeroObjective::fulfill(CGeniusAI& cg,
                            HypotheticalGameState& hgs)
{
  cg.call_back_->waitTillRealize = true;
  HypotheticalGameState::HeroModel* h = NULL;
  int3 hpos;
  int3 destination;
  int3 bestPos;
  int3 currentPos;
  int3 checkPos;
  CPath path;
  CPath path2;
  si16 howGood = 0;

  switch (type) {
  case finishTurn:
    h = whoCanAchieve.front();
    h->finished=true;
    hpos = h->pos;
    destination = h->interestingPos;

    if (!cg.call_back_->getPath(hpos, destination, h->h, path)) {
      cout << "AI error: invalid destination" << endl;
      return;
    }
    destination = h->pos;
    // Find closest coord that we can get to.
    for (si16 i = path.nodes.size() - 2; i >= 0; i--) {
      // TODO: getPath what??
      if ((cg.call_back_->getPath(hpos, path.nodes[i].coord, h->h, path2)) &&
        (path2.nodes[0].dist <= h->remainingMovement))
        destination = path.nodes[i].coord;
    }

    if (destination == h->interestingPos)
      break;

    // ! START ! //
    // Find close pos with the most neighboring empty squares. We don't want to
    // get in the way.
    bestPos = destination;
    howGood = 0;
    // TODO: Add a meaning to 3.
    for (si16 x = -3; x <= 3; x++) {
      for (si16 y = -3; y <= 3; y++) {
        currentPos = destination + int3(x,y,0);
        // There better not be anything there.
        if (cg.call_back_->getVisitableObjs(currentPos).size() != 0)
          continue;
        if ((cg.call_back_->getPath(hpos, currentPos, h->h, path) == false) ||
          // It better be reachable from the hero
          // TODO: remainingMovement > 0...
          (path.nodes[0].dist>h->remainingMovement))
          continue;

        si16 count = 0;
        for (si16 xx = -1; xx <= 1; xx++) {
          for (si16 yy = -1; yy <= 1; yy++) {
            checkPos = currentPos + int3(xx, yy, 0);
            if (cg.call_back_->getPath(currentPos, checkPos, h->h, path) != false)
              count++;
          }
        }
        if (count > howGood) {
          howGood = count;
          bestPos = currentPos;
        }
      }
    }

    destination = bestPos;
    // ! END ! //
    cg.call_back_->getPath(hpos, destination, h->h, path);
    path.convert(0);
    break;

  case visit:
  case attack:
    h = whoCanAchieve.front();		//lowest cost hero
    h->previouslyVisited_pos = object->getSightCenter();
    hpos = h->pos;
    destination = object->getSightCenter();
    break;
  } // switch(type)

  if ((type == visit || type == finishTurn || type == attack) &&
    (cg.call_back_->getPath(hpos, destination, h->h, path) != false))
    path.convert(0);

  if (cg.state_.get() != NO_BATTLE)
    cg.state_.waitUntil(NO_BATTLE); // Wait for battle end

  // Wait over, battle over too. hero might be killed. check.
  for (si16 i = path.nodes.size() - 2;
    (i >= 0) && (cg.call_back_->getHeroSerial(h->h) >= 0);
    i--) {
      cg.call_back_->moveHero(h->h,path.nodes[i].coord);

      if (cg.state_.get() != NO_BATTLE)
        cg.state_.waitUntil(NO_BATTLE); // Wait for battle end
  }

  h->remainingMovement -= path.nodes[0].dist;
  if (object->blockVisit)
    h->pos = path.nodes[1].coord;
  else
    h->pos = destination;

  std::set<AIObjectContainer>::iterator
    i = hgs.knownVisitableObjects.find(AIObjectContainer(object));
  if (i != hgs.knownVisitableObjects.end())
    hgs.knownVisitableObjects.erase(i);

  const CGTownInstance* town = dynamic_cast<const CGTownInstance*> (object);
  if (town && object->getOwner() == cg.call_back_->getMyColor()) {
    //upgrade hero's units
    cout << "visiting town" << endl;
    CCreatureSet hcreatures = h->h->army;
    for (std::map< si32, std::pair<ui32,si32> >::const_iterator
      i = hcreatures.slots.begin();
      i != hcreatures.slots.end();
    i++) { // For each hero slot.
      UpgradeInfo ui = cg.call_back_->getUpgradeInfo(h->h,i->first);

      bool canUpgrade = false;
      if (ui.newID.size() != 0) { // Does this stack need upgrading?
        canUpgrade = true;
        for (si16 ii = 0; ii < ui.cost.size(); ii++) // Can afford the upgrade?
          // TODO: Modify the CGameState upgrade info...
          for (std::set<std::pair<int, int> >::iterator
            j = ui.cost[ii].begin();
            j != ui.cost[ii].end();
        j++)
          if (hgs.resourceAmounts[j->first] < j->second * i->second.second)
            canUpgrade = false;
      }

      if (canUpgrade) {
        cg.call_back_->upgradeCreature(h->h, i->first, ui.newID.back());
        cout << "upgrading hero's "
          << VLC->creh->creatures[i->second.first].namePl
          << endl;
      }
    }

    // Give town's units to hero.
    CCreatureSet tcreatures = town->army;
    si16 weakestCreatureStack;
    si16 weakestCreatureAIValue = 99999; // TODO: Better write INT_MAX ...

    for (std::map< si32, std::pair<ui32,si32> >::const_iterator
      i = tcreatures.slots.begin();
      i != tcreatures.slots.end();
    i++) {
      if (VLC->creh->creatures[i->second.first].AIValue < 
        weakestCreatureAIValue) {
          weakestCreatureAIValue = VLC->creh->creatures[i->second.first].AIValue;
          weakestCreatureStack = i->first;
      }
    }
    for (std::map< si32, std::pair<ui32, si32> >::const_iterator
      i = tcreatures.slots.begin();
      i != tcreatures.slots.end();
    i++) { // For each town slot.
      hcreatures = h->h->army;
      si16 hSlot = hcreatures.getSlotFor(i->second.first);

      if (hSlot == -1)
        continue;
      cout << "giving hero "
        << VLC->creh->creatures[i->second.first].namePl
        << endl;
      if (hcreatures.slots.find(hSlot) != hcreatures.slots.end()) {
        // Can't take garrisonHero's last unit.
        if ( (i->first == weakestCreatureStack)
          && (town->garrisonHero != NULL) )
          cg.call_back_->splitStack(town, h->h, i->first, hSlot, i->second.second - 1);
        else
          // TODO: the comment says that this code is not safe for the AI.
          cg.call_back_->mergeStacks(town, h->h, i->first, hSlot);
      } else
        cg.call_back_->swapCreatures(town, h->h, i->first, hSlot);	
    } // for (std::map< si32, std::pair<ui32, si32> >::const_iterator ...
  } // if (town && object->getOwner == cg.call_back_->getMyColor())
}

float HeroObjective::getValue() const
{
  if (_value >= 0)
    return _value - _cost;

  // TODO: each object should have an associated cost to visit IE 
  // (tree of knowledge 1000 gold/10 gems)
  vector<si16> resourceCosts;
  for (ui8 i = 0; i < 8; i++)
    resourceCosts.push_back(0);

  if (object->ID == 47) // School of magic
    resourceCosts[6] += 1000;

  float bestCost = FLT_MAX;
  HypotheticalGameState::HeroModel* bestHero = NULL;
  if (type != AIObjective::finishTurn) {
    for (si16 i = 0; i < whoCanAchieve.size(); i++) {
      si16 distOutOfTheWay = 0;
      CPath path3;
      //from hero to object
      if (AI->call_back_->getPath(whoCanAchieve[i]->pos,
                                  pos,
                                  whoCanAchieve[i]->h,
                                  path3))
          distOutOfTheWay+=path3.nodes[0].dist;

      // from object to goal
      if (AI->call_back_->getPath(pos,
        whoCanAchieve[i]->interestingPos,
        whoCanAchieve[i]->h,
        path3)) {
          distOutOfTheWay += path3.nodes[0].dist;
          // from hero directly to goal
          if (AI->call_back_->getPath(whoCanAchieve[i]->pos,
            whoCanAchieve[i]->interestingPos,
            whoCanAchieve[i]->h,
            path3))
            distOutOfTheWay -= path3.nodes[0].dist;
      }

      float cost = AI->priorities_->getCost(resourceCosts,
                                            whoCanAchieve[i]->h,
                                            distOutOfTheWay);
      if (cost < bestCost) {
        bestCost = cost;
        bestHero = whoCanAchieve[i];
      }
    } // for (si16 i = 0; i < whoCanAchieve.size(); i++)
  } else  // if (type != AIObjective::finishTurn)
    bestCost = 0;

  if (bestHero) {
    whoCanAchieve.clear();
    whoCanAchieve.push_back(bestHero);
  }

  _value = AI->priorities_->getValue(*this);
  _cost = bestCost;
  return _value - _cost;
}

bool HeroObjective::operator<(const HeroObjective& other) const
{
  if (type != other.type)
    return type < other.type;
  else if (pos != other.pos)
    return pos < other.pos;
  else if (object->id != other.object->id)
    return object->id < other.object->id;
  else if ((dynamic_cast<const CGVisitableOPH*>(object) != NULL) &&
    (whoCanAchieve.front()->h->id != other.whoCanAchieve.front()->h->id))
    return whoCanAchieve.front()->h->id < other.whoCanAchieve.front()->h->id;
  else
    return false;
}

void HeroObjective::print() const
{
  switch (type) {
    case visit:
      cout << "visit " << object->hoverName
           << " at (" <<object->pos.x << ","<< object->pos.y << ")" ;
      break;
    case attack:
      cout << "attack " << object->hoverName;
      break;
    case finishTurn:
      cout << "finish turn";
    // TODO: Add a default, just in case.
  }
  if (whoCanAchieve.size() == 1)
    cout << " with " << whoCanAchieve.front()->h->hoverName;
}
} // namespace geniusai