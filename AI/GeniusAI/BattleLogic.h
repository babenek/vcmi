#ifndef BATTLE_LOGIC_H
#define BATTLE_LOGIC_H

#include "Common.h"
#include "BattleHelper.h"

#pragma warning(push, 0)
#include "../../global.h"
#include "../../CCallback.h"
#include "../../hch/CCreatureHandler.h"
#include "../../hch/CAmbarCendamo.h"
#pragma warning(pop)

namespace geniusai { namespace battleai {
//*****************************************************************************
//* Class is responsible for making decision during the battle.               *
//*****************************************************************************
class BattleLogic {
public:
	BattleLogic(ICallback *cb, CCreatureSet *army1, CCreatureSet *army2, int3 tile,
	    CGHeroInstance *hero1, CGHeroInstance *hero2, bool side);
	~BattleLogic();

	void SetCurrentTurn(ui16 turn);
	/**
	 * Get the final decision.
	 */
	BattleAction MakeDecision(ui16 stackID);
private:
	enum MainStrategyType {
		STRATEGY_SUPER_AGGRESIVE,
		STRATEGY_AGGRESIVE,
		STRATEGY_NEUTRAL,
		STRATEGY_DEFENSIVE,
    STRATEGY_SUPER_DEFENSIVE,
		STRATEGY_SUPER_ATTACK /** cause max damage, usually when creatures fight against overwhelming army*/
	};
	
	enum CreatureRoleInBattle {
		creature_role_shooter,
		creature_role_defenser,
		creature_role_fast_attacker,
		creature_role_attacker
	};
	
	enum ActionType {
		action_cancel = 0,				// Cancel BattleAction
		action_cast_spell = 1,			// Hero cast a spell
		action_walk = 2,				// Walk
		action_defend = 3,				// Defend
		action_retreat = 4,				// Retreat from the battle
		action_surrender = 5,			// Surrender
		action_walk_and_attack = 6,		// Walk and Attack
		action_shoot = 7,				// Shoot
		action_wait = 8,				// Wait
		action_catapult = 9,			// Catapult
		action_monster_casts_spell = 10 // Monster casts a spell (i.e. Faerie Dragons)
	};
	
	struct CreatureCasualties {
		si16 amount_max; // amount of creatures that will be dead
		si16 amount_min;
		si16 damage_max; // number of hit points that creature will lose
		si16 damage_min;
		si16 leftHitPoints_for_max; // number of hit points that will remain
		si16 leftHitPoint_for_min; // (for last unity) scenario
	};
	
	typedef std::vector<std::pair<ui16, CreatureCasualties> > creature_stat_casualties;
	
	CBattleHelper m_battleHelper;
	//BattleInfo m_battleInfo;
	si16 m_iCurrentTurn;
	bool m_bIsAttacker;
	ICallback       *m_cb;
	CCreatureSet    *m_army1;
	CCreatureSet    *m_army2;
	int3            m_tile;
	CGHeroInstance  *m_hero1;
	CGHeroInstance  *m_hero2;
	bool m_side;

	creature_stat_casualties m_statCasualties;
  // Statistics.
  // First - creature id, second - value.
	typedef std::vector< std::pair<ui16, ui16> > creature_stat;
	creature_stat m_statMaxDamage;
	creature_stat m_statMinDamage;
	creature_stat m_statMaxSpeed;
	creature_stat m_statDistance;
	creature_stat m_statDistanceFromShooters;
	creature_stat m_statHitPoints;

	bool m_bEnemyDominates;
	// Before decision we have to make some calculation and simulation.
	void MakeStatistics(ui16 currentCreatureId);
	// Helper function. It's used for performing an attack action.
	std::vector<ui16> GetAvailableHexesForAttacker(const CStack* defender,
	                                               const CStack* attacker = NULL);
	// Just make defend action.
	BattleAction MakeDefend(ui16 stackID);
	// Just make wait action.
	BattleAction MakeWait(ui16 stackID);
	// Make an attack action if it's possible. If it's not possible then 
	// function returns defend action.
	BattleAction MakeAttack(ui16 attackerID, ui16 destinationID);
	// Berserk mode - do maximum casualties. Return vector wiht IDs of creatures 
	// to attack, additional info: -2 means wait, -1 - defend, 0 - make attack
	// TODO: Why to use -2, -1 & 0 instead of normal (unsigned) values??
	std::list<ui16> PerformBerserkAttack(ui16 stackID, si8 additionalInfo);
	// Normal mode - equilibrium between casualties and yields. Return vector 
	// with IDs of creatures to attack, additional info: -2 means wait, 
	// -1 - defend, 0 - make attack.
	std::list<ui16> PerformDefaultAction(ui16 stackID, si8 additionalInfo);

  // Only for debug purpose.
	void PrintBattleAction(const BattleAction& action);
};

}}

#endif // BATTLE_LOGIC_H