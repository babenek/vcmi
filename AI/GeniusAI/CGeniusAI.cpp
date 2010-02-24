#include "StdAfx.h"
#include "CGeniusAI.h"

#if defined (_MSC_VER) && (_MSC_VER >= 1020) || (__MINGW32__)
// Excludes rarely used stuff from windows headers - delete this line if
// something is missing.
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#endif

// TODO: Is this debug box needed?
void debugBox(const char* msg, bool messageBox)
{
#if defined PRINT_DEBUG
#if defined _DEBUG
#if defined (_MSC_VER) && (_MSC_VER >= 1020)
  if (messageBox)
    MessageBoxA(NULL, msg, "Debug message", MB_OK | MB_ICONASTERISK);
#endif
  std::cout << msg << std::endl;
#endif
#endif
}

#include "../../hch/CBuildingHandler.h"
#include "../../hch/CHeroHandler.h"
#include "../../hch/CObjectHandler.h"
#include "../../CCallback.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/NetPacks.h"

#include "BattleLogic.h"
#include "HeroObjective.h"
#include "TownObjective.h"

using std::cout;
using std::endl;
using std::vector;

namespace geniusai {
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

void CGeniusAI::addHeroObjectives(HypotheticalGameState::HeroModel& h,
                                  HypotheticalGameState& hgs)
{
	int3 hpos = h.pos;
  int3 destination;
  int3 interestingPos;
  CPath path;
	si16 movement = h.remainingMovement;
	si16 maxInteresting = 0;
	AIObjective::Type tp = AIObjective::visit;

	if (h.finished)
    return;

	for (std::set<AIObjectContainer>::const_iterator
      i = hgs.knownVisitableObjects.begin();
      i != hgs.knownVisitableObjects.end();
      i++) {
		tp = AIObjective::visit;
		if(	h.previouslyVisited_pos==i->object_instance_->getSightCenter())
			continue;

		//TODO: what would the hero actually visit if he went to that spot
		// maybe the hero wants to visit a seemingly unguarded enemy town,
    // but there is a hero on top of it.
		// if(i->o->)
    if (i->object_instance_->ID != HEROI_TYPE) {// Unless you are trying to visit a hero.
			bool heroThere = false;
      for(si16 j = 0; j < hgs.heroModels.size(); j++) {
				if (hgs.heroModels[j].pos == i->object_instance_->getSightCenter())
          heroThere = true;
      }
			if (heroThere) // It won't work if there is already someone visiting that spot.
				continue;
		}
		// Visiting friendly heroes not yet supported.
		if (i->object_instance_->ID == HEROI_TYPE && 
		    i->object_instance_->getOwner() == call_back_->getMyColor())
			continue;
	  // Don't visit yourself (should be caught by above).
		if (i->object_instance_->id == h.h->id)	
			continue;
    // Don't visit a mine if you own, there's almost no
    // point(maybe to leave guards or because the hero's trapped).
		if (i->object_instance_->ID == 53
        && i->object_instance_->getOwner() == call_back_->getMyColor())
			continue;

		if (i->object_instance_->getOwner() != call_back_->getMyColor()) {
      // TODO: I feel like the AI shouldn't have access to this information.
      // We must get an approximation based on few, many, ... zounds etc.
			si16 enemyStrength = 0;  

      // TODO: should be virtual maybe, army strength should be
      // comparable across objects.
      // TODO: Rewrite all those damn i->o. For someone reading it the first
      // time it's completely inconprehensible.
      // TODO: NO MAGIC NUMBERS !!!
			if (dynamic_cast<const CArmedInstance*> (i->object_instance_))
				enemyStrength =
          (dynamic_cast<const CArmedInstance*> (i->object_instance_))->getArmyStrength();
			if (dynamic_cast<const CGHeroInstance*> (i->object_instance_))
				enemyStrength =
          (dynamic_cast<const CGHeroInstance*> (i->object_instance_))->getTotalStrength();
      // TODO: Make constants of those 1.2 & 2.5.
			if (dynamic_cast<const CGTownInstance*> (i->object_instance_))
				enemyStrength = static_cast<si16>( (dynamic_cast<const CGTownInstance*>
              (i->object_instance_) )->getArmyStrength()*1.2);
			float heroStrength = h.h->getTotalStrength();
      // TODO: ballence these numbers using objective cost formula.
      // TODO: it would be nice to do a battle simulation.
			if (enemyStrength * 2.5 > heroStrength)  
				continue;

			if (enemyStrength > 0)
        tp = AIObjective::attack;
		}

    // Don't visit things that have already been visited this week.
		if ((dynamic_cast<const CGVisitableOPW*> (i->object_instance_) != NULL) &&
        (dynamic_cast<const CGVisitableOPW*> (i->object_instance_)->visited))
			continue;

    // Don't visit things that you have already visited OPH.
		if ((dynamic_cast<const CGVisitableOPH*> (i->object_instance_) != NULL) &&
        vstd::contains(dynamic_cast<const CGVisitableOPH*>
            (i->object_instance_)->visitors, h.h->id))
			continue;

    // TODO: Some descriptions of those included so someone can undestand them.
		if (i->object_instance_->ID == 88 ||
		    i->object_instance_->ID == 89 ||
		    i->object_instance_->ID == 90) {
			//TODO: if no spell book continue
			//TODO: if the shrine's spell is identified, and the hero already has it, continue
		}

		destination = i->object_instance_->getSightCenter();

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
					HeroObjective ho(hgs, tp, i->object_instance_, &h, this);
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
				si16 hi = rand();
				if (hi > maxInteresting) {
					maxInteresting = hi;
					interestingPos = destination;
				}
      }
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
    for (si16 i = 0; i < hgs.heroModels.size(); i++) {
			if (hgs.heroModels[i].pos == t.t->getSightCenter())
				heroAtTown = true;
    }
    // No visiting hero and built tavern.
		if (!heroAtTown && vstd::contains(t.t->builtBuildings, 5)) {
      for (si16 i = 0; i < hgs.AvailableHeroesToBuy.size(); i++) {
				if ( (hgs.AvailableHeroesToBuy[i] != NULL)
            && (t.t->subID ==
              hgs.AvailableHeroesToBuy[i]->type->heroType / 2) ) {
					TownObjective to(hgs, AIObjective::recruitHero, &t, 0, this);
					currentTownObjectives.insert(to);
				}
      }
    }
  }
	
  // Build a building.
	if (!t.hasBuilt) {
	  // TODO: Int to si16
	  typedef std::map<int, CBuilding*> townBuilding;
    // call_back_->getCBuildingsByID(t.t);
		townBuilding thisTownsBuildings =
      VLC->buildh->buildings[t.t->subID];
		for (townBuilding::iterator i = thisTownsBuildings.begin();
        i != thisTownsBuildings.end();
        i++) {
			if (call_back_->canBuildStructure(t.t, i->first) == 7) {
				TownObjective to(hgs, AIObjective::buildBuilding, &t, i->first, this);
				currentTownObjectives.insert(to);
			}
		}
	}
	
	// Recruit creatures.
	for (si16 i = 0; i < t.creaturesToRecruit.size(); i++) {
		if (t.creaturesToRecruit[i].first == 0 ||
        t.creaturesToRecruit[i].second.empty())
      continue;
		
    si16 ID = t.creaturesToRecruit[i].second.back();
    // call_back_->getCCreatureByID(ID);
		const CCreature* creature = &VLC->creh->creatures[ID];
		bool canAfford = true;
		for (si16 ii = 0; ii < creature->cost.size(); ii++) {
			if (creature->cost[ii] > hgs.resourceAmounts[ii])
				canAfford = false; // Can we afford at least one creature?
		}
		if (!canAfford) 
		  continue;
				
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
			
			si16 upgrade_serial = ui.newID.size() - 1;
			// TODO: Int to si16
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

void CGeniusAI::fillObjectiveQueue(HypotheticalGameState& hgs)
{
	objectiveQueue.clear();
	currentHeroObjectives.clear();
	currentTownObjectives.clear();
	
	typedef std::vector<HypotheticalGameState::HeroModel>::iterator
	  heroModelIterator;
  typedef std::vector<HypotheticalGameState::TownModel>::iterator
    townModelIterator;
	
	for (heroModelIterator i = hgs.heroModels.begin(); i != hgs.heroModels.end();
       i++)
		addHeroObjectives(*i, hgs);

	for (townModelIterator i = hgs.townModels.begin(); i != hgs.townModels.end();
      i++)
		addTownObjectives(*i, hgs);
	
	for (std::set<HeroObjective>::iterator
      i = currentHeroObjectives.begin();
      i != currentHeroObjectives.end();
      i++)
    // TODO: Recheck and try to write simpler expression.
		objectiveQueue.push_back(
		    AIObjectivePtrCont( static_cast<HeroObjective*>(&(*i)) ));

	for (std::set<TownObjective>::iterator
       i = currentTownObjectives.begin();
       i != currentTownObjectives.end();
       i++)
		objectiveQueue.push_back(
		    AIObjectivePtrCont( static_cast<TownObjective*>(&(*i)) ));
}

AIObjective* CGeniusAI::getBestObjective()
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
	si16 num = 1;
//	for(std::vector<AIObjectivePtrCont> ::iterator i = objectiveQueue.begin(); i < objectiveQueue.end();i++)
//	{
//		if(!dynamic_cast<HeroObjective*>(i->obj))continue;
//		cout << num++ << ": ";
//		i->obj->print();
//		cout << " value: " << i->obj->getValue();
//		cout << endl;
//	}
//	si16 choice = 0;
//	cout << "which would you do? (enter 0 for none): ";
//	cin >> choice;
	cout << "doing best of " << objectiveQueue.size() << " ";
	AIObjective* best = max_element(objectiveQueue.begin(),
	                                           objectiveQueue.end())->obj;
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
	static si16 seed = rand();
	srand(seed);
// 	if (call_back_->getDate() == 1) {
// 	//	startFirstTurn();
// 		
// 	//	call_back_->endTurn();
// 	//	return;
// 	}
// 	// TODO: replace with updates. Also add suspected objects list.
// 	knownVisitableObjects.clear();
// 	int3 pos = call_back_->getMapSize();
//   for (si16 x = 0; x < pos.x; x++) {
//     for (si16 y = 0; y < pos.y; y++) {
// 			for (si16 z = 0; z < pos.z; z++)
// 				tileRevealed(int3(x,y,z));
//     }
//   }
// 	//////////////////////////////////////////////////////////////////////
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
	
	const CGTownInstance* town = call_back_->getTownInfo(0,0);
	const CGHeroInstance* heroInst = call_back_->getHeroInfo(0,0);
	
	TownObjective(hgs,
	              AIObjective::recruitHero,
	              &hgs.townModels.front(),
	              0,
	              this).fulfill(*this, hgs);
	
	call_back_->swapGarrisonHero(town);
	hgs.update(*this);
	for (si16 i = 0; i < hgs.townModels.front().creaturesToRecruit.size(); i++) {
		if (hgs.townModels.front().creaturesToRecruit[i].first == 0)
      continue;
		si16 ID = hgs.townModels.front().creaturesToRecruit[i].second.back();
		const CCreature* creature = &VLC->creh->creatures[ID];
		bool canAfford = true;
    for (si16 ii = 0; ii < creature->cost.size(); ii++) {
			if (creature->cost[ii] > hgs.resourceAmounts[ii])
				canAfford = false;  // Can we afford at least one creature?
    }
		if (!canAfford)
      continue;
		TownObjective(hgs,
		              AIObjective::recruitCreatures,
		              &hgs.townModels.front(),
		              i,
		              this).fulfill(*this,hgs);
  }
	hgs.update(*this);

	HypotheticalGameState::HeroModel* hero;
  for (si16 i = 0; i < hgs.heroModels.size(); i++) {
		if (hgs.heroModels[i].h->id == heroInst->id)
		  HeroObjective(hgs,
		                AIObjective::visit,
		                town,
		                hero = &hgs.heroModels[i],
		                this).fulfill(*this, hgs);
  }
	hgs.update(*this);
  // call_back_->swapGarrisonHero(town);
	// TODO: choose the strongest hero.
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

// eg. collected resource, picked artifact, beaten hero
void CGeniusAI::objectRemoved(const CGObjectInstance *obj)
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

void CGeniusAI::heroPrimarySkillChanged(const CGHeroInstance* hero, si16 which,
                                        si64 val)
{
  ;
}

void CGeniusAI::showSelDialog(std::string text,
                              std::vector<CSelectableComponent*>& components,
                              si16 askID)
{
  ;
}

void CGeniusAI::heroGotLevel(const CGHeroInstance* hero,
                             int pskill,
                             std::vector<si16>& skills,
                             boost::function<void(ui32)>& callback)
{
  // TODO: Write more meaningul callback id, maybe?
	callback(rand() % skills.size());
}

void CGeniusAI::showGarrisonDialog(const CArmedInstance* up,
                                             const CGHeroInstance* down,
                                             bool removableUnits,
                                             boost::function<void()>& onEnd)
{
	onEnd();
}

void CGeniusAI::playerBlocked(si16 reason)
{
	if (reason == 0) // Battle is coming...
		state_.setn(UPCOMING_BATTLE);
}

void CGeniusAI::battleResultsApplied()
{
	assert(state_.get() == ENDING_BATTLE);
	state_.setn(NO_BATTLE);
}

// TODO: Shouldn't the parameters be made const (apart from cancel)?
// TODO: Why so many parameters?
void CGeniusAI::showBlockingDialog(const std::string& text,
                                   const std::vector<Component> &components,
                                   ui32 askID,
                                   const si16 soundID,
                                   bool selection,
                                   bool cancel)
{
	call_back_->selectionMade(cancel ? false : true, askID);
}

// Occurs BEFORE every action taken by any stack or by the hero.
void CGeniusAI::actionStarted(const BattleAction* action)
{
  std::string message("\t\tCGeniusAI::actionStarted - type(");
  message += boost::lexical_cast<std::string>((unsigned)action->actionType);
  message += "), side(";
  message += boost::lexical_cast<std::string>((unsigned)action->side);
  message += ")";
  debugBox(message.c_str());
}

// Occurs AFTER every action taken by any stack or by the hero.
void CGeniusAI::actionFinished(const BattleAction* action)
{
	std::string message("\t\tCGeniusAI::actionFinished - type(");
	message += boost::lexical_cast<std::string>((unsigned)action->actionType);
	message += "), side(";
	message += boost::lexical_cast<std::string>((unsigned)action->side);
	message += ")";
	debugBox(message.c_str());
}

// Called when stack is performing attack.
void CGeniusAI::battleAttack(BattleAttack* ba)
{
	debugBox("\t\t\tCGeniusAI::battleAttack");
}

// Called when stack receives damage (after battleAttack()).
void CGeniusAI::battleStacksAttacked(std::set<BattleStackAttacked>& bsa)
{
	debugBox("\t\t\tCGeniusAI::battleStacksAttacked");
}

// Called by engine when battle starts; side=0 - left, side=1 - right
void CGeniusAI::battleStart(CCreatureSet* army1,
                            CCreatureSet* army2,
                            int3 tile,
                            CGHeroInstance* hero1,
                            CGHeroInstance* hero2,
                            bool side)
{
  // TODO: Battle logic what...
	assert(!battle_logic_);
  // We have been informed that battle will start (or we are neutral AI)
	assert( (playerID > PLAYER_LIMIT) || (state_.get() == UPCOMING_BATTLE) );

	state_.setn(ONGOING_BATTLE);
	battle_logic_ = new geniusai::battleai::BattleLogic(call_back_, army1, army2,
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
void CGeniusAI::battleNewRound(si16 round)
{
	std::string message("\tCGeniusAI::battleNewRound - ");
	message += boost::lexical_cast<std::string>(round);
	debugBox(message.c_str());

	battle_logic_->SetCurrentTurn(round);
}

void CGeniusAI::battleStackMoved(si16 ID, si16 dest, si16 distance, bool end)
{
	std::string message("\t\t\tCGeniusAI::battleStackMoved ID(");
	message += boost::lexical_cast<std::string>(ID);
	message += "), dest(";
	message += boost::lexical_cast<std::string>(dest);
	message += ")";
	debugBox(message.c_str());
}

void CGeniusAI::battleSpellCast(SpellCast *sc)
{
	debugBox("\t\t\tCGeniusAI::battleSpellCast");
}

// Called when battlefield is prepared, prior the battle beginning.
void CGeniusAI::battlefieldPrepared(si16 battlefieldType,
                                    std::vector<CObstacle*> obstacles)
{
	debugBox("CGeniusAI::battlefieldPrepared");
}


void CGeniusAI::battleStackMoved(si16 ID,
                                 si16 dest,
                                 bool startMoving,
                                 bool endMoving)
{
	debugBox("\t\t\tCGeniusAI::battleStackMoved");
}

/**
 *
 */
void CGeniusAI::battleStackAttacking(si16 ID, si16 dest)
{
	debugBox("\t\t\tCGeniusAI::battleStackAttacking");
}

/**
 *
 */
void CGeniusAI::battleStackIsAttacked(si16 ID,
                                      si16 dmg,
                                      si16 killed,
                                      si16 IDby,
                                      bool byShooting)
{
	debugBox("\t\t\tCGeniusAI::battleStackIsAttacked");
}

/**
 * called when it's turn of that stack
 */
BattleAction CGeniusAI::activeStack(si16 stackID)
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