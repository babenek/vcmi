#ifndef CGENIUS_AI_H
#define CGENIUS_AI_H

#include "Common.h"

#include "HypotheticalGameState.h"
#include "../../lib/CondSh.h"
#include "GeneralAI.h"
#include "BattleLogic.h"
#include "TownObjective.h"
#include "HeroObjective.h"
#include "AIPriorities.h"

class CBuilding;

namespace geniusai {
enum BattleState
{
	NO_BATTLE,
	UPCOMING_BATTLE,
	ONGOING_BATTLE,
	ENDING_BATTLE
};

class AIObjective;

class CGeniusAI : public CGlobalAI {
public:  
	CGeniusAI();
	virtual ~CGeniusAI();

	virtual void init(ICallback* CB);
	virtual void yourTurn();
	virtual void heroKilled(const CGHeroInstance*);
	virtual void heroCreated(const CGHeroInstance*);
	virtual void heroMoved(const TryMoveHero&);
	virtual void heroPrimarySkillChanged(const CGHeroInstance* hero,
	                                     si16 which,
	                                     si64 val);
	virtual void showSelDialog(std::string text,
	                           std::vector<CSelectableComponent*>& components,
	                           si16 askID);
  // Show a dialog, player must take decision. If selection then he has to
  // choose between one of given components, if cancel he is allowed to not
  // choose. After making choice, CCallback::selectionMade should be called
  // with number of selected component (1 - n) or 0 for cancel (if allowed) and
  // askID.
	virtual void showBlockingDialog(const std::string& text,
	                                const std::vector<Component>& components,
	                                ui32 askID,
	                                const si16 soundID,
	                                bool selection,
	                                bool cancel);
	virtual void tileRevealed(int3 pos);
	virtual void tileHidden(int3 pos);
	// TODO: Change the base class function's parameters to boost
	virtual void heroGotLevel(const CGHeroInstance* hero,
	                          int pskill,
	                          std::vector<si16>& skills,
	                          boost::function<void(ui32)>& callback);
	virtual void showGarrisonDialog(const CArmedInstance* up,
	                                const CGHeroInstance* down,
	                                bool removableUnits,
	                                boost::function<void()>& onEnd);
	virtual void playerBlocked(si16 reason);
	//eg. collected resource, picked artifact, beaten hero
	virtual void objectRemoved(const CGObjectInstance* obj);
	//eg. ship built in shipyard
	virtual void newObject(const CGObjectInstance* obj);
	
	// battle
	//occurs AFTER every action taken by any stack or by the hero
	virtual void actionFinished(const BattleAction *action);
	//occurs BEFORE every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction *action);
	// called when stack is performing attack
	virtual void battleAttack(BattleAttack* ba);
	// Called when stack receives damage (after battleAttack()).
	virtual void battleStacksAttacked(std::set<BattleStackAttacked>& bsa);
	virtual void battleEnd(BattleResult* br);
	// called at the beggining of each turn, round=-1 is the tactic phase, 
	// round=0 is the first "normal" turn
	virtual void battleNewRound(si16 round);
	virtual void battleStackMoved(si16 ID, si16 dest, si16 distance, bool end);
	virtual void battleSpellCast(SpellCast *sc);
	// Called by engine when battle starts; side = 0 - left, side = 1 - right.
	virtual void battleStart(CCreatureSet* army1,
	                         CCreatureSet* army2,
	                         int3 tile,
	                         CGHeroInstance* hero1,
	                         CGHeroInstance* hero2,
	                         bool side);
	// Called when battlefield is prepared, prior the battle beginning.
	virtual void battlefieldPrepared(si16 battlefieldType,
	                                 std::vector<CObstacle*> obstacles);
	virtual void battleStackMoved(si16 ID, si16 dest,
	                              bool startMoving, bool endMoving);
	virtual void battleStackAttacking(si16 ID, si16 dest);
	virtual void battleStackIsAttacked(si16 ID,
	                                   si16 dmg,
	                                   si16 killed,
	                                   si16 IDby,
	                                   bool byShooting);
	virtual BattleAction activeStack(si16 stackID);
	void battleResultsApplied();

private:
  class AIObjectivePtrCont {
  public:
    AIObjectivePtrCont() : obj(NULL) {};
    AIObjectivePtrCont(AIObjective* obj) : obj(obj){};
    AIObjective* obj;
    bool operator<(const AIObjectivePtrCont& other) const {
      return obj->getValue() < other.obj->getValue();
    }
  };

  ::ICallback*            call_back_;
  battleai::BattleLogic*  battle_logic_;
  generalai::CGeneralAI   general_AI_;
  Priorities*             priorities_;
  ::CondSh<BattleState>   state_; //are we engaged in battle?

  friend class Priorities;
  friend class HypotheticalGameState;
  friend class HeroObjective;
  friend class TownObjective;

  AIObjective* getBestObjective();
  void addHeroObjectives(geniusai::HypotheticalGameState::HeroModel& h,
    HypotheticalGameState& hgs);
  void addTownObjectives(geniusai::HypotheticalGameState::TownModel& h,
    HypotheticalGameState& hgs);
  void fillObjectiveQueue(geniusai::HypotheticalGameState& hgs);
  void reportResources();
  void startFirstTurn();

  HypotheticalGameState trueGameState;
  std::map<si16, bool> isHeroStrong;//hero
  std::set<AIObjectContainer> knownVisitableObjects;
  std::set<HeroObjective> currentHeroObjectives;	//can be fulfilled right now
  std::set<TownObjective> currentTownObjectives;
  std::vector<AIObjectivePtrCont> objectiveQueue;
};
}

#endif // CGENIUS_AI_H