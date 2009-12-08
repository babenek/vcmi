#include "CGeniusAI.h"

// TODO: Fix the directory problem.

#pragma warning(push, 0)
#include <iostream>
#include <boost/lexical_cast.hpp>

#include "CBuildingHandler.h"
#include "CHeroHandler.h"
#include "CObjectHandler.h"
#include "CCallback.h"
#include "BattleLogic.h"
#include "VCMI_Lib.h"
#include "NetPacks.h"
#pragma warning(pop)

#include "AIPriorities.h"

using std::cout;
using std::endl;
using std::vector;

#if defined (_MSC_VER) && (_MSC_VER >= 1020) || (__MINGW32__)
// Excludes rarely used stuff from windows headers - delete this line if
// something is missing.
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#endif

void debugBox(const char* msg, bool messageBox)
{
#if defined PRINT_DEBUG
#if defined _DEBUG
//# if 0
#	if defined (_MSC_VER) && (_MSC_VER >= 1020)
	if (messageBox)
	{
		MessageBoxA(NULL, msg, "Debug message", MB_OK | MB_ICONASTERISK);
	}
#	endif
	std::cout << msg << std::endl;
#endif
#endif
}

namespace geniusai {
// TODO: Rewrite those i-s, o-s to something meaningful.
bool CGeniusAI::AIObjectContainer::operator<(
    const AIObjectContainer& b) const
{
	if (o->pos != b.o->pos)
		return o->pos < b.o->pos;
  else
	  return o->id < b.o->id;
}

CGeniusAI::HypotheticalGameState::HeroModel::HeroModel(const CGHeroInstance* h)
    : h(h), finished(false)
{
	pos = h->getPosition(false);
  remainingMovement = h->movement;
}

CGeniusAI::HypotheticalGameState::TownModel::TownModel(const CGTownInstance* t)
    : t(t)
{
	hasBuilt = static_cast<bool>(t->builded);
	creaturesToRecruit = t->creatures;
	creaturesInGarrison = t->army;
}

CGeniusAI::HypotheticalGameState::HypotheticalGameState(CGeniusAI& ai)
    : knownVisitableObjects(ai.knownVisitableObjects)
{
	AI = &ai;
	std::vector<const CGHeroInstance*> heroes = ai.call_back_->getHeroesInfo();	
	for (std::vector<const CGHeroInstance*>::iterator i = heroes.begin();
       i != heroes.end();
       i++)
		heroModels.push_back(HeroModel(*i));
	
	std::vector<const CGTownInstance*> towns = ai.call_back_->getTownsInfo();	
	for (std::vector <const CGTownInstance*>::iterator i = towns.begin();
       i != towns.end();
       i++) {
		if ( (*i)->tempOwner == ai.call_back_->getMyColor() )
		  townModels.push_back(TownModel(*i));
  }

  if (ai.call_back_->howManyTowns() != 0) {
    AvailableHeroesToBuy = 
      ai.call_back_->getAvailableHeroes(ai.call_back_->getTownInfo(0,0));
  }

	for (int i = 0; i < 8; i++)
    resourceAmounts.push_back(ai.call_back_->getResourceAmount(i));
}

void CGeniusAI::HypotheticalGameState::update(CGeniusAI& ai)
{
	AI = &ai;
	knownVisitableObjects = ai.knownVisitableObjects;
	std::vector<HeroModel> oldModels = heroModels;
	heroModels.clear();

	std::vector<const CGHeroInstance*> heroes = ai.call_back_->getHeroesInfo();
	for (std::vector<const CGHeroInstance*>::iterator i = heroes.begin();
       i != heroes.end();
       i++)
		heroModels.push_back(HeroModel(*i));

  for (int i = 0; i < oldModels.size(); i++) {
    for (int j = 0; j < heroModels.size(); j++) {
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
	for (int i = 0; i < 8; i++)
    resourceAmounts.push_back(ai.call_back_->getResourceAmount(i));
}

CGeniusAI::HeroObjective::HeroObjective(const HypotheticalGameState& hgs,
                                        Type t,
                                        const CGObjectInstance* object,
                                        HypotheticalGameState::HeroModel* h,
                                        CGeniusAI* ai)
                                        : object(object), hgs(hgs)
{
	AI = ai;
	pos = object->pos;
	type = t;
	whoCanAchieve.push_back(h);
	_value = -1;
}

float CGeniusAI::HeroObjective::getValue() const
{
	if (_value >= 0)
    return _value - _cost;

  // TODO: each object should have an associated cost to visit IE 
  // (tree of knowledge 1000 gold/10 gems)
	vector<int> resourceCosts;
	for (int i = 0; i < 8; i++)
		resourceCosts.push_back(0);

	if (object->ID == 47) // School of magic
		resourceCosts[6] += 1000;

	float bestCost = FLT_MAX;
	HypotheticalGameState::HeroModel* bestHero = NULL;
	if (type != AIObjective::finishTurn)
	{
		for (int i = 0; i < whoCanAchieve.size(); i++)
		{
			int distOutOfTheWay = 0;
			CPath path3;
			//from hero to object
			if (AI->call_back_->getPath(whoCanAchieve[i]->pos,
                            pos,
                            whoCanAchieve[i]->h,
                            path3)) {
				distOutOfTheWay+=path3.nodes[0].dist;
      }

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
    } // for (int i = 0; i < whoCanAchieve.size(); i++)
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

bool CGeniusAI::HeroObjective::operator<(
    const HeroObjective& other) const
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

void CGeniusAI::HeroObjective::print() const
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

CGeniusAI::TownObjective::TownObjective(
    const HypotheticalGameState& hgs,
    Type t,
    HypotheticalGameState::TownModel* tn,
    int Which,
    CGeniusAI * ai)
    : whichTown(tn), which(Which), hgs(hgs)
{
	AI = ai;
	type = t;
	_value = -1;
}

float CGeniusAI::TownObjective::getValue() const
{
	if (_value >= 0)
		return _value - _cost;

  // TODO: Include a constant stating the meaning of 8 (number of resources).
	vector<int> resourceCosts(8,0);
	CBuilding* b        = NULL;
	CCreature* creature = NULL;
  float cost          = 0; // TODO: Needed?
	int ID              = 0;
  int newID           = 0;
  int howMany         = 0;
  ui32 creatures_max  = 0;

	switch (type) {
		case recruitHero:
			resourceCosts[6] = 2500;  // TODO: Define somehow the meaning of gold etc.
			break;

		case buildBuilding:
			b = VLC->buildh->buildings[whichTown->t->subID][which];
			for (int i = 0;
          b && ( i < b->resources.size() ); // TODO: b what??
          i++)
				resourceCosts[i] = b->resources[i];
			break;

	  case recruitCreatures:
      // Buy upgraded if possible.
		  ID = whichTown->creaturesToRecruit[which].second.back();
		  creature = &VLC->creh->creatures[ID];
		  howMany = whichTown->creaturesToRecruit[which].first;
      creatures_max = 0; // Max creatures you can recruit of this type.
      
      for (int i = 0; i < creature->cost.size(); i++) {
        if (creature->cost[i] != 0)
          creatures_max = hgs.resourceAmounts[i]/creature->cost[i];
        else
          creatures_max = INT_MAX; // TODO: Will have to rewrite it.
        // TODO: Buy the best units (the least I can buy)?
			  amin(howMany, creatures_max);
      }
      // The cost of recruiting the stack of creatures.
		  for (int i = 0;
          creature && ( i < creature->cost.size() ); // TODO: Creature what??
          i++)
			  resourceCosts[i] = creature->cost[i]*howMany;
		  break;

	  case upgradeCreatures:
		  UpgradeInfo ui = AI->call_back_->getUpgradeInfo(whichTown->t,which);
		  ID = whichTown->creaturesInGarrison.slots[which].first;
		  howMany = whichTown->creaturesInGarrison.slots[which].second;

		  newID = ui.newID.back();
		  int upgrade_serial = ui.newID.size() - 1;
		  for (std::set< std::pair<int,int> >::iterator
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

bool CGeniusAI::TownObjective::operator<(const TownObjective &other) const
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

void CGeniusAI::TownObjective::print() const
{
  HypotheticalGameState::HeroModel hm;
	CBuilding* b              = NULL;
	const CCreature* creature = NULL;
	int ID                    = 0;
  int howMany               = 0;
  int newID                 = 0; // TODO: Needed?
  int hSlot                 = 0; // TODO: Needed?
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

      for (int i = 0; i < creature->cost.size(); i++) {
        if (creature->cost[i] != 0)
          creatures_max = hgs.resourceAmounts[i]/creature->cost[i];
        else
          creatures_max = INT_MAX;
        amin(howMany, creatures_max);
      }
		  cout << "recruit " << howMany  << " " << creature->namePl
           << " (Total AI Strength " << creature->AIValue*howMany
           << "). cost = ";
  		
		  if (creature->cost.size() != 0)
		  {
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

CGeniusAI::CGeniusAI() : general_AI_(), state_(NO_BATTLE)
{
	priorities_ = new Priorities("AI/GeniusAI.brain");
}

CGeniusAI::~CGeniusAI()
{
	delete priorities_;
}

void CGeniusAI::init(ICallback *CB)
{
	call_back_ = CB;
	general_AI_.init(CB);

	human = false;
	playerID = call_back_->getMyColor();
	serialID = call_back_->getMySerial();
	std::string info = std::string("GeniusAI initialized for player ") 
                   + boost::lexical_cast<std::string>(playerID);
	battle_logic_ = NULL;
	debugBox(info.c_str());
}

void CGeniusAI::reportResources()
{
	cout << "Day " << call_back_->getDate() << ": ";
	cout << "AI Player " <<call_back_->getMySerial()<< " with "
       <<  call_back_->howManyHeroes(true) << " heroes. " << endl;
	cout << call_back_->getResourceAmount(0) << " wood. ";
	cout << call_back_->getResourceAmount(1) << " mercury. ";
	cout << call_back_->getResourceAmount(2) << " ore. ";
	cout << call_back_->getResourceAmount(3) << " sulfur. ";
	cout << call_back_->getResourceAmount(4) << " cristal. ";
	cout << call_back_->getResourceAmount(5) << " gems. ";
	cout << call_back_->getResourceAmount(6) << " gold.";
	cout << endl;
}

void CGeniusAI::addHeroObjectives(CGeniusAI::HypotheticalGameState::HeroModel& h,
                                  CGeniusAI::HypotheticalGameState& hgs)
{
	int3 hpos = h.pos;
  int3 destination;
  int3 interestingPos;
  CPath path;
	int movement = h.remainingMovement;
	int maxInteresting = 0;
	AIObjective::Type tp = AIObjective::visit;

	if (h.finished)
    return;

	for (std::set<AIObjectContainer>::const_iterator
      i = hgs.knownVisitableObjects.begin();
      i != hgs.knownVisitableObjects.end();
      i++) {
		tp = AIObjective::visit;
		if(	h.previouslyVisited_pos==i->o->getSightCenter())
			continue;

		//TODO: what would the hero actually visit if he went to that spot
		// maybe the hero wants to visit a seemingly unguarded enemy town,
    // but there is a hero on top of it.
		// if(i->o->)
    if (i->o->ID != HEROI_TYPE) {// Unless you are trying to visit a hero.
			bool heroThere = false;
      for(int j = 0; j < hgs.heroModels.size(); j++) {
				if (hgs.heroModels[j].pos == i->o->getSightCenter())
          heroThere = true;
      }
			if (heroThere) // It won't work if there is already someone visiting that spot.
				continue;
		}
		if (i->o->ID == HEROI_TYPE && // Visiting friendly heroes not yet supported.
        i->o->getOwner() == call_back_->getMyColor())
			continue;
		if (i->o->id == h.h->id)	// Don't visit yourself (should be caught by above).
			continue;
    // Don't visit a mine if you own, there's almost no
    // point(maybe to leave guards or because the hero's trapped).
		if (i->o->ID == 53
        && i->o->getOwner() == call_back_->getMyColor())
			continue;

		if (i->o->getOwner() != call_back_->getMyColor()) {
      // TODO: I feel like the AI shouldn't have access to this information.
      // We must get an approximation based on few, many, ... zounds etc.
			int enemyStrength = 0;  

      // TODO: should be virtual maybe, army strength should be
      // comparable across objects.
      // TODO: Rewrite all those damn i->o. For someone reading it the first
      // time it's completely inconprehensible.
      // TODO: NO MAGIC NUMBERS !!!
			if (dynamic_cast<const CArmedInstance*> (i->o))
				enemyStrength =
          (dynamic_cast<const CArmedInstance*> (i->o))->getArmyStrength();
			if (dynamic_cast<const CGHeroInstance*> (i->o))
				enemyStrength =
          (dynamic_cast<const CGHeroInstance*> (i->o))->getTotalStrength();
      // TODO: Make constants of those 1.2 & 2.5.
			if (dynamic_cast<const CGTownInstance*> (i->o))
				enemyStrength = static_cast<int>(
          (dynamic_cast<const CGTownInstance*> (i->o))->getArmyStrength() * 1.2);
			float heroStrength = h.h->getTotalStrength();
      // TODO: ballence these numbers using objective cost formula.
      // TODO: it would be nice to do a battle simulation.
			if (enemyStrength * 2.5 > heroStrength)  
				continue;

			if (enemyStrength > 0)
        tp = AIObjective::attack;
		}

    //don't visit things that have already been visited this week.
		if ((dynamic_cast<const CGVisitableOPW*> (i->o) != NULL) &&
        (dynamic_cast<const CGVisitableOPW*> (i->o)->visited))
			continue;

    //don't visit things that you have already visited OPH
		if ((dynamic_cast<const CGVisitableOPH*> (i->o) != NULL) &&
        vstd::contains(dynamic_cast<const CGVisitableOPH*> (i->o)->visitors,
                       h.h->id))
			continue;

    // TODO: Some descriptions of those included so someone can undestand them.
		if (i->o->ID == 88 || i->o->ID == 89 || i->o->ID == 90) {
			//TODO: if no spell book continue
			//TODO: if the shrine's spell is identified, and the hero already has it, continue
		}

		destination = i->o->getSightCenter();

    // Don't try to take a path from the underworld to the top or vice versa.
    // TODO: Will have to make some calculations so that the AI can enter the
    // underground.
    if (hpos.z == destination.z) {
      //TODO: fix get path so that it doesn't return a path unless z's are \
      // the same, or path goes through sub gate.
			if (call_back_->getPath(hpos, destination, h.h, path)) {
				path.convert(0);
				if (path.nodes[0].dist < movement) {
          // TODO: So easy to understand...
					HeroObjective ho(hgs, tp, i->o, &h, this);
					std::set<HeroObjective>::iterator found = currentHeroObjectives.find(ho);
					if (found == currentHeroObjectives.end())
						currentHeroObjectives.insert(ho);
					else {
            // TODO: Try to rewrite if possible...
            // A cast to a pointer, from a reference, to a pointer
            // of an iterator.
						HeroObjective* objective = (HeroObjective*)&(*found);
						objective->whoCanAchieve.push_back(&h);
					}
				}

				// Find the most interesting object that is eventually reachable,
        // and set that position to the ultimate goal position.
        // TODO: replace random numbers with some sort of ranking system.
				int hi = rand();
				if (hi > maxInteresting) {
					maxInteresting = hi;
					interestingPos = destination;
				}
      } // if (call_back_->getPath(hpos, destination, h.h, path))
    } // if (hpos.z == destination.z)
	} // for (std::set<AIObjectContainer>::const_iterator 
    // i = knownVisitableObjects.begin();

	h.interestingPos = interestingPos;
  // there ought to be a path
  // if(h.remainingMovement>0&&call_back_->getPath(hpos,interestingPos,h.h,path))
	currentHeroObjectives.insert(HeroObjective(hgs,
                                             HeroObjective::finishTurn,
                                             h.h,
                                             &h,
                                             this));
}

void CGeniusAI::HeroObjective::fulfill(CGeniusAI& cg,
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
	int howGood = 0;
	
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
    for (int i = path.nodes.size() - 2; i >= 0; i--) {
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
    for (int x = -3; x <= 3; x++) {
			for (int y = -3; y <= 3; y++) {
				currentPos = destination + int3(x,y,0);
        // There better not be anything there.
				if (cg.call_back_->getVisitableObjs(currentPos).size() != 0)
					continue;
				if ((cg.call_back_->getPath(hpos, currentPos, h->h, path) == false) ||
            // It better be reachable from the hero
            // TODO: remainingMovement > 0...
            (path.nodes[0].dist>h->remainingMovement))
					continue;
				
				int count = 0;
        for (int xx = -1; xx <= 1; xx++) {
					for (int yy = -1; yy <= 1; yy++) {
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
	for (int i = path.nodes.size() - 2;
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
			  for (int ii = 0; ii < ui.cost.size(); ii++) // Can afford the upgrade?
				  for (std::set<std::pair<int,int> >::iterator
               j = ui.cost[ii].begin();
               j != ui.cost[ii].end();
               j++)
					  if (hgs.resourceAmounts[j->first] < j->second * i->second.second)
						  canUpgrade = false;
		  }
			
      if (canUpgrade)
		  {
			  cg.call_back_->upgradeCreature(h->h, i->first, ui.newID.back());
			  cout << "upgrading hero's "
             << VLC->creh->creatures[i->second.first].namePl
             << endl;
		  }
	  }

	  // Give town's units to hero.
	  CCreatureSet tcreatures = town->army;
	  int weakestCreatureStack;
	  int weakestCreatureAIValue = 99999; // TODO: Wtf??

	  for (std::map< si32, std::pair<ui32,si32> >::const_iterator
         i = tcreatures.slots.begin();
         i != tcreatures.slots.end();
         i++) {
		  if (VLC->creh->creatures[i->second.first].AIValue < 
          weakestCreatureAIValue) {
			  weakestCreatureAIValue  = VLC->creh->creatures[i->second.first].AIValue;
			  weakestCreatureStack    = i->first;
		  }
    }
	  for (std::map< si32, std::pair<ui32, si32> >::const_iterator
        i = tcreatures.slots.begin();
        i != tcreatures.slots.end();
        i++) { // For each town slot.
		  hcreatures = h->h->army;
		  int hSlot = hcreatures.getSlotFor(i->second.first);

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


void CGeniusAI::addTownObjectives(HypotheticalGameState::TownModel& t,
                                  HypotheticalGameState& hgs)
{
	//recruitHero
	//buildBuilding
	//recruitCreatures
	//upgradeCreatures

  // Recruit hero.
	if ( (hgs.heroModels.size() < 3) && (hgs.resourceAmounts[6] >= 2500) ) {
		bool heroAtTown = false;
    for (int i = 0; i < hgs.heroModels.size(); i++) {
			if (hgs.heroModels[i].pos == t.t->getSightCenter())
				heroAtTown = true;
    }
    // No visiting hero and built tavern.
		if (!heroAtTown && vstd::contains(t.t->builtBuildings, 5)) {
      for (int i = 0; i < hgs.AvailableHeroesToBuy.size(); i++) {
				if ( (hgs.AvailableHeroesToBuy[i] != NULL)
            && (t.t->subID ==
              hgs.AvailableHeroesToBuy[i]->type->heroType / 2) ) {
					TownObjective to(hgs,AIObjective::recruitHero,&t,0,this);
					currentTownObjectives.insert(to);
				}
      }
    }
  }
	
  // Build a building.
	if (!t.hasBuilt) {
    // call_back_->getCBuildingsByID(t.t);
		std::map<int, CBuilding*> thisTownsBuildings =
      VLC->buildh->buildings[t.t->subID];
		for (std::map<int, CBuilding*>::iterator i = thisTownsBuildings.begin();
        i != thisTownsBuildings.end();
        i++) {
			if (call_back_->canBuildStructure(t.t, i->first) == 7) {
				TownObjective to(hgs, AIObjective::buildBuilding, &t, i->first ,this);
				currentTownObjectives.insert(to);
			}
		}
	}
	
	// Recruit creatures.
	for (int i = 0; i < t.creaturesToRecruit.size(); i++) {
		if (t.creaturesToRecruit[i].first == 0
        || t.creaturesToRecruit[i].second.empty())
      continue;
		
    int ID = t.creaturesToRecruit[i].second.back();
    // call_back_->getCCreatureByID(ID);
		const CCreature *creature = &VLC->creh->creatures[ID];
		bool canAfford = true;
		for (int ii = 0; ii < creature->cost.size(); ii++)
			if (creature->cost[ii] > hgs.resourceAmounts[ii])
				canAfford = false; // Can we afford at least one creature?
		if (!canAfford) continue;
				
		//cout << "town has " << t.t->creatures[i].first  << " "<< creature->namePl << " (AI Strength " << creature->AIValue << ")." << endl;
		TownObjective to(hgs, AIObjective::recruitCreatures, &t, i, this);
		currentTownObjectives.insert(to);
	}

  // Upgrade creatures.
	for (std::map< si32, std::pair<ui32, si32> >::iterator
      i = t.creaturesInGarrison.slots.begin();
      i != t.creaturesInGarrison.slots.end();
      i++) {
		UpgradeInfo ui = call_back_->getUpgradeInfo(t.t, i->first);
		if (ui.newID.size() != 0) {
			bool canAfford = true;
			
			int upgrade_serial = ui.newID.size() - 1;
			for (std::set< std::pair<int, int> >::iterator
          j = ui.cost[upgrade_serial].begin();
          j != ui.cost[upgrade_serial].end();
          j++)
				if (hgs.resourceAmounts[j->first] < j->second * i->second.second)
					canAfford = false;
			if (canAfford) {
				TownObjective to(hgs,AIObjective::upgradeCreatures,&t,i->first,this);
				currentTownObjectives.insert(to);
			}
    } // if (ui.netID.size() != 0)
  } // for (std::map< si32, std::pair ...
}


void CGeniusAI::TownObjective::fulfill(CGeniusAI& cg,
                                       HypotheticalGameState& hgs)
{
	cg.call_back_->waitTillRealize = true;
	CBuilding * b;
	const CCreature *creature;
	HypotheticalGameState::HeroModel hm;
	int ID, howMany, newID, hSlot;

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
			for (int i = 0; b && i < b->resources.size(); i++) // TODO: b what?
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
		for (int i = 0; i < creature->cost.size(); i++)
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


void CGeniusAI::fillObjectiveQueue(HypotheticalGameState & hgs)
{
	objectiveQueue.clear();
	currentHeroObjectives.clear();
	currentTownObjectives.clear();
	
	for (std::vector <CGeniusAI::HypotheticalGameState::HeroModel>::iterator
      i = hgs.heroModels.begin();
      i != hgs.heroModels.end();
      i++)
		addHeroObjectives(*i, hgs);

	for (std::vector <CGeniusAI::HypotheticalGameState::TownModel>::iterator
      i = hgs.townModels.begin();
      i != hgs.townModels.end();
      i++)
		addTownObjectives(*i, hgs);
	
	for (std::set<CGeniusAI::HeroObjective>::iterator
      i = currentHeroObjectives.begin();
      i != currentHeroObjectives.end();
      i++)
    // TODO: Recheck and try to write simpler expression.
		objectiveQueue.push_back(AIObjectivePtrCont((CGeniusAI::HeroObjective*)&(*i)));

	for (std::set<CGeniusAI::TownObjective>::iterator
      i = currentTownObjectives.begin();
      i != currentTownObjectives.end();
      i++)
		objectiveQueue.push_back(AIObjectivePtrCont((CGeniusAI::TownObjective*)&(*i)));
}


CGeniusAI::AIObjective * CGeniusAI::getBestObjective()
{
	trueGameState.update(*this);	
	fillObjectiveQueue(trueGameState);

// TODO: Write this part.
//	if(!objectiveQueue.empty())
//		return max_element(objectiveQueue.begin(),objectiveQueue.end())->obj;
	priorities_->fillFeatures(trueGameState);
	if (objectiveQueue.empty())
    return NULL;
//	sort(objectiveQueue.begin(),objectiveQueue.end());
//	reverse(objectiveQueue.begin(),objectiveQueue.end());
	int num = 1;
//	for(std::vector<AIObjectivePtrCont> ::iterator i = objectiveQueue.begin(); i < objectiveQueue.end();i++)
//	{
//		if(!dynamic_cast<HeroObjective*>(i->obj))continue;
//		cout << num++ << ": ";
//		i->obj->print();
//		cout << " value: " << i->obj->getValue();
//		cout << endl;
//	}
//	int choice = 0;
//	cout << "which would you do? (enter 0 for none): ";
//	cin >> choice;
	cout << "doing best of " << objectiveQueue.size() << " ";
	CGeniusAI::AIObjective* best =
    max_element(objectiveQueue.begin(), objectiveQueue.end())->obj;
	best->print();
	cout << " value = " << best->getValue() << endl;
	if (!objectiveQueue.empty())
		return best;
	return objectiveQueue.front().obj;
}


void CGeniusAI::yourTurn()
{
	static boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);
	call_back_->waitTillRealize = true;
	static int seed = rand();
	srand(seed);
// 	if (call_back_->getDate() == 1) {
// 	//	startFirstTurn();
// 		
// 	//	call_back_->endTurn();
// 	//	return;
// 	}
// 	//////////////TODO: replace with updates. Also add suspected objects list./////////
// 	knownVisitableObjects.clear();
// 	int3 pos = call_back_->getMapSize();
//   for (int x = 0; x < pos.x; x++) {
//     for (int y = 0; y < pos.y; y++) {
// 			for (int z = 0; z < pos.z; z++)
// 				tileRevealed(int3(x,y,z));
//     }
//   }
// 	///////////////////////////////////////////////////////////////////////////////////
// 
// 	reportResources();
// 
// 	trueGameState = HypotheticalGameState(*this);
// 	AIObjective * objective;
// 	while ((objective = getBestObjective()) != NULL)
// 		objective->fulfill(*this,trueGameState);
// 
// 	seed = rand();
	call_back_->endTurn();
	call_back_->waitTillRealize = false;
}


void CGeniusAI::startFirstTurn()
{
	HypotheticalGameState hgs(*this);
	
	const CGTownInstance * town = call_back_->getTownInfo(0,0);
	const CGHeroInstance * heroInst = call_back_->getHeroInfo(0,0);
	
	TownObjective(hgs,
                AIObjective::recruitHero,
                &hgs.townModels.front(),
                0,
                this).fulfill(*this, hgs);
	
	call_back_->swapGarrisonHero(town);
	hgs.update(*this);
	for (int i = 0; i < hgs.townModels.front().creaturesToRecruit.size(); i++) {
		if (hgs.townModels.front().creaturesToRecruit[i].first == 0)
      continue;
		int ID = hgs.townModels.front().creaturesToRecruit[i].second.back();
		const CCreature *creature = &VLC->creh->creatures[ID];
		bool canAfford = true;
    for (int ii = 0; ii < creature->cost.size(); ii++) {
			if (creature->cost[ii] > hgs.resourceAmounts[ii])
				canAfford = false;  // Can we afford at least one creature?
    }
		if (!canAfford)
      continue;
		TownObjective(hgs,AIObjective::recruitCreatures,&hgs.townModels.front(),i,this).fulfill(*this,hgs);
  }
	hgs.update(*this);

	HypotheticalGameState::HeroModel* hero;
  for (int i = 0; i < hgs.heroModels.size(); i++) {
		if (hgs.heroModels[i].h->id == heroInst->id)
		  HeroObjective(hgs, AIObjective::visit, town, hero = &hgs.heroModels[i], this).fulfill(*this,hgs);
  }
	hgs.update(*this);
 //	call_back_->swapGarrisonHero(town);
	//TODO: choose the strongest hero.
}


void CGeniusAI::heroKilled(const CGHeroInstance* hero)
{
}


void CGeniusAI::heroCreated(const CGHeroInstance* hero)
{
}


void CGeniusAI::tileRevealed(int3 pos)
{
	std::vector<const CGObjectInstance*> objects = call_back_->getVisitableObjs(pos);
	for (std::vector < const CGObjectInstance* >::iterator o = objects.begin();
      o != objects.end();
      o++) {
		if ((*o)->id != -1)
			knownVisitableObjects.insert(*o);
  }
	objects = call_back_->getFlaggableObjects(pos);
	for (std::vector<const CGObjectInstance*>::iterator
      o = objects.begin();
      o != objects.end();
      o++)
		if ((*o)->id != -1)
			knownVisitableObjects.insert(*o);
}

// eg. ship built in shipyard
void CGeniusAI::newObject(const CGObjectInstance* obj)
{
  knownVisitableObjects.insert(obj);
}


void CGeniusAI::objectRemoved(const CGObjectInstance *obj) //eg. collected resource, picked artifact, beaten hero
{
	std::set<AIObjectContainer>::iterator o = knownVisitableObjects.find(obj);
	if (o != knownVisitableObjects.end())
		knownVisitableObjects.erase(o);
}


void CGeniusAI::tileHidden(int3 pos)
{
}


void CGeniusAI::heroMoved(const TryMoveHero& TMH)
{
	// debugBox("** CGeniusAI::heroMoved **");
}


void CGeniusAI::heroGotLevel(const CGHeroInstance *hero,
                             int pskill,
                             std::vector<ui16>& skills,
                             boost::function<void(ui32)>& callback)
{
	callback(rand() % skills.size());
}


void geniusai::CGeniusAI::showGarrisonDialog(const CArmedInstance* up,
                                             const CGHeroInstance* down,
                                             bool removableUnits,
                                             boost::function<void()>& onEnd)
{
	onEnd();
}


void geniusai::CGeniusAI::playerBlocked(int reason)
{
	if (reason == 0) // Battle is coming...
		state_.setn(UPCOMING_BATTLE);
}


void geniusai::CGeniusAI::battleResultsApplied()
{
	assert(state_.get() == ENDING_BATTLE);
	state_.setn(NO_BATTLE);
}

// TODO: Shouldn't the parameters be made const (apart from cancel)?
void CGeniusAI::showBlockingDialog(const std::string& text,
                                   const std::vector<Component> &components,
                                   ui32 askID,
                                   const int soundID,
                                   bool selection,
                                   bool cancel)
{
	call_back_->selectionMade(cancel ? false : true, askID);
}


/**
 * occurs AFTER every action taken by any stack or by the hero
 */
void CGeniusAI::actionFinished(const BattleAction* action)
{
	std::string message("\t\tCGeniusAI::actionFinished - type(");
	message += boost::lexical_cast<std::string>((unsigned)action->actionType);
	message += "), side(";
	message += boost::lexical_cast<std::string>((unsigned)action->side);
	message += ")";
	debugBox(message.c_str());
}

/**
 * occurs BEFORE every action taken by any stack or by the hero
 */
void CGeniusAI::actionStarted(const BattleAction *action)
{
	std::string message("\t\tCGeniusAI::actionStarted - type(");
	message += boost::lexical_cast<std::string>((unsigned)action->actionType);
	message += "), side(";
	message += boost::lexical_cast<std::string>((unsigned)action->side);
	message += ")";
	debugBox(message.c_str());
}

/**
 * called when stack is performing attack
 */
void CGeniusAI::battleAttack(BattleAttack* ba)
{
	debugBox("\t\t\tCGeniusAI::battleAttack");
}

/**
 * called when stack receives damage (after battleAttack())
 */
void CGeniusAI::battleStacksAttacked(std::set<BattleStackAttacked>& bsa)
{
	debugBox("\t\t\tCGeniusAI::battleStacksAttacked");
}

/**
 * called by engine when battle starts; side=0 - left, side=1 - right
 */
void CGeniusAI::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side)
{
  // TODO: Battle logic what...
	assert(!battle_logic_);
  // We have been informed that battle will start (or we are neutral AI)
	assert( (playerID > PLAYER_LIMIT) || (state_.get() == UPCOMING_BATTLE) );

	state_.setn(ONGOING_BATTLE);
	battle_logic_ = new geniusai::battleai::CBattleLogic(call_back_, army1, army2,
	    tile, hero1, hero2, side);

	debugBox("** CGeniusAI::battleStart **");
}

/**
 *
 */
void CGeniusAI::battleEnd(BattleResult* br)
{
	switch (br->winner) {
		case 0:
		  std::cout << "The winner is the attacker." << std::endl;
		  break;
		case 1:
		  std::cout << "The winner is the defender." << std::endl;
		  break;
		case 2:
		std::cout << "It's a draw." << std::endl;
		break;
	};
	cout << "lost ";
	for (std::map<ui32,si32>::iterator i = br->casualties[0].begin();\
       i != br->casualties[0].end();
       i++)
		cout << i->second << " " << VLC->creh->creatures[i->first].namePl << endl;
				
	delete battle_logic_;
	battle_logic_ = NULL;

	assert(state_.get() == ONGOING_BATTLE);
	state_.setn(ENDING_BATTLE);

	debugBox("** CGeniusAI::battleEnd **");
}

/*
 * Called at the beggining of each turn, round = -1 is the tactic phase,
 * round = 0 is the first "normal" turn.
 */
void CGeniusAI::battleNewRound(int round)
{
	std::string message("\tCGeniusAI::battleNewRound - ");
	message += boost::lexical_cast<std::string>(round);
	debugBox(message.c_str());

	battle_logic_->SetCurrentTurn(round);
}

/**
 *
 */
void CGeniusAI::battleStackMoved(int ID, int dest, int distance, bool end)
{
	std::string message("\t\t\tCGeniusAI::battleStackMoved ID(");
	message += boost::lexical_cast<std::string>(ID);
	message += "), dest(";
	message += boost::lexical_cast<std::string>(dest);
	message += ")";
	debugBox(message.c_str());
}

/**
 *
 */
void CGeniusAI::battleSpellCast(SpellCast *sc)
{
	debugBox("\t\t\tCGeniusAI::battleSpellCast");
}

/**
 * called when battlefield is prepared, prior the battle beginning
 */
void CGeniusAI::battlefieldPrepared(int battlefieldType,
                                    std::vector<CObstacle*> obstacles)
{
	debugBox("CGeniusAI::battlefieldPrepared");
}

/**
 *
 */
void CGeniusAI::battleStackMoved(int ID,
                                 int dest,
                                 bool startMoving,
                                 bool endMoving)
{
	debugBox("\t\t\tCGeniusAI::battleStackMoved");
}

/**
 *
 */
void CGeniusAI::battleStackAttacking(int ID, int dest)
{
	debugBox("\t\t\tCGeniusAI::battleStackAttacking");
}

/**
 *
 */
void CGeniusAI::battleStackIsAttacked(int ID,
                                      int dmg,
                                      int killed,
                                      int IDby,
                                      bool byShooting)
{
	debugBox("\t\t\tCGeniusAI::battleStackIsAttacked");
}

/**
 * called when it's turn of that stack
 */
BattleAction CGeniusAI::activeStack(int stackID)
{
	std::string message("\t\t\tCGeniusAI::activeStack stackID(");

	message += boost::lexical_cast<std::string>(stackID);
	message += ")";
	debugBox(message.c_str());

	BattleAction bact = battle_logic_->MakeDecision(stackID);
	assert(call_back_->battleGetStackByID(bact.stackNumber));
	return bact;
};
}