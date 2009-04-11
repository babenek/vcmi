#include "CGeniusAI.h"
#include <iostream>
#include <fstream>
#include <math.h>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::lambda;
using namespace std;
using namespace GeniusAI;

#if defined (_MSC_VER) && (_MSC_VER >= 1020) || (__MINGW32__)
#include <windows.h>
#endif

void MsgBox(const char *msg, bool messageBox = false)
{
#if defined _DEBUG
#	if defined (_MSC_VER) && (_MSC_VER >= 1020)
	if (messageBox)
	{
		MessageBoxA(NULL, msg, "Debug message", MB_OK | MB_ICONASTERISK);
	}
#	endif
	std::cout << msg << std::endl;
#endif
}

void CGeniusAI::init(ICallback *CB)
{
	m_cb = CB;
	human = false;
	playerID = m_cb->getMyColor();
	serialID = m_cb->getMySerial();
	std::string info = std::string("GeniusAI initialized for player ") + boost::lexical_cast<std::string>(playerID);
	m_battleLogic = NULL;
	MsgBox(info.c_str());
}

void CGeniusAI::yourTurn()
{
	m_cb->endTurn();
}

void CGeniusAI::heroKilled(const CGHeroInstance *)
{
}

void CGeniusAI::heroCreated(const CGHeroInstance *)
{
}

void CGeniusAI::heroMoved(const HeroMoveDetails &)
{
}

void CGeniusAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	callback(rand() % skills.size());
}

void CGeniusAI::showBlockingDialog( const std::string &text, const std::vector<Component> &components, ui32 askID, bool selection, bool cancel )
{
	m_cb->selectionMade(cancel ? 0 : 1, askID);
}

/**
 * occurs AFTER every action taken by any stack or by the hero
 */
void CGeniusAI::actionFinished(const BattleAction *action)
{
	std::string message("\t\tCGeniusAI::actionFinished - type(");
	message += boost::lexical_cast<std::string>((unsigned)action->actionType);
	message += "), side(";
	message += boost::lexical_cast<std::string>((unsigned)action->side);
	message += ")";
	MsgBox(message.c_str());
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
	MsgBox(message.c_str());
}
/**
 * called when stack is performing attack
 */
void CGeniusAI::battleAttack(BattleAttack *ba)
{
	MsgBox("\t\t\tCGeniusAI::battleAttack");
}
/**
 * called when stack receives damage (after battleAttack())
 */
void CGeniusAI::battleStacksAttacked(std::set<BattleStackAttacked> & bsa)
{
	MsgBox("\t\t\tCGeniusAI::battleStacksAttacked");
}
/**
 * called by engine when battle starts; side=0 - left, side=1 - right
 */
void CGeniusAI::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side)
{
	assert(!m_battleLogic);
	m_battleLogic = new CBattleLogic(m_cb, army1, army2, tile, hero1, hero2, side);
	assert(m_battleLogic);

	MsgBox("** CGeniusAI::battleStart **");
}
/**
 *
 */
void CGeniusAI::battleEnd(BattleResult *br)
{
	delete m_battleLogic;
	m_battleLogic = NULL;

	MsgBox("** CGeniusAI::battleEnd **");
}
/**
 * called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
 */
void CGeniusAI::battleNewRound(int round)
{
	std::string message("\tCGeniusAI::battleNewRound - ");
	message += boost::lexical_cast<std::string>(round);
	MsgBox(message.c_str());

	m_battleLogic->SetCurrentTurn(round);
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
	MsgBox(message.c_str());
}
/**
 *
 */
void CGeniusAI::battleSpellCasted(SpellCasted *sc)
{
	MsgBox("\t\t\tCGeniusAI::battleSpellCasted");
}
/**
 * called when battlefield is prepared, prior the battle beginning
 */
void CGeniusAI::battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles)
{
	MsgBox("CGeniusAI::battlefieldPrepared");
}
/**
 *
 */
void CGeniusAI::battleStackMoved(int ID, int dest, bool startMoving, bool endMoving)
{
	MsgBox("\t\t\tCGeniusAI::battleStackMoved");
}
/**
 *
 */
void CGeniusAI::battleStackAttacking(int ID, int dest)
{
	MsgBox("\t\t\tCGeniusAI::battleStackAttacking");
}
/**
 *
 */
void CGeniusAI::battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting)
{
	MsgBox("\t\t\tCGeniusAI::battleStackIsAttacked");
}

/**
 * called when it's turn of that stack
 */
BattleAction CGeniusAI::activeStack(int stackID)
{
	std::string message("\t\t\tCGeniusAI::activeStack stackID(");

	message += boost::lexical_cast<std::string>(stackID);
	message += ")";
	MsgBox(message.c_str());

	return m_battleLogic->MakeDecision(stackID);
};

/*
ui8 side; //who made this action: false - left, true - right player
	ui32 stackNumber;//stack ID, -1 left hero, -2 right hero,
	ui8 actionType; //
		0 = Cancel BattleAction
		1 = Hero cast a spell
		2 = Walk
		3 = Defend
		4 = Retreat from the battle
		5 = Surrender
		6 = Walk and Attack
		7 = Shoot
		8 = Wait
		9 = Catapult
		10 = Monster casts a spell (i.e. Faerie Dragons)
	ui16 destinationTile;
	si32 additionalInfo; // e.g. spell number if type is 1 || 10; tile to attack if type is 6
*/

/**
 *	Implementation of CBattleHelper class.
 */

CBattleHelper::CBattleHelper():
	InfiniteDistance(0xffff),
	BattlefieldHeight(11),
	BattlefieldWidth(15),
	m_voteForDistance(10),
	m_voteForDistanceFromShooters(20),
	m_voteForHitPoints(10),
	m_voteForMaxDamage(10),
	m_voteForMaxSpeed(10),
	m_voteForMinDamage(10)
{
	// loads votes
	std::fstream f;
	f.open("AI\\CBattleHelper.txt", std::ios::in);
	if (f)
	{
		//char c_line[512];
		std::string line;
		while (std::getline(f, line, '\n'))
		{
			//f.getline(c_line, sizeof(c_line), '\n');
			//std::string line(c_line);
			//std::getline(f, line);
			std::vector<std::string> parts;
			boost::algorithm::split(parts, line, boost::algorithm::is_any_of("="));
			if (parts.size() >= 2)
			{
				boost::algorithm::trim(parts[0]);
				boost::algorithm::trim(parts[1]);
				if (parts[0].compare("m_voteForDistance") == 0)
				{
					try
					{
						m_voteForDistance = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForDistanceFromShooters") == 0)
				{
					try
					{
						m_voteForDistanceFromShooters = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForHitPoints") == 0)
				{
					try
					{
						m_voteForHitPoints = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMaxDamage") == 0)
				{
					try
					{
						m_voteForMaxDamage = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMaxSpeed") == 0)
				{
					try
					{
						m_voteForMaxSpeed = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("m_voteForMinDamage") == 0)
				{
					try
					{
						m_voteForMinDamage = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
			}
		}
		f.close();
	}
}

CBattleHelper::~CBattleHelper()
{}

int CBattleHelper::GetBattleFieldPosition(int x, int y)
{
	return x + 17 * (y - 1);
}

int CBattleHelper::DecodeXPosition(int battleFieldPosition)
{
	int pos = battleFieldPosition - (DecodeYPosition(battleFieldPosition) - 1) * 17;
	assert(pos > 0 && pos < 16);
	return pos;
}

int CBattleHelper::DecodeYPosition(int battleFieldPosition)
{
	double y = (double)battleFieldPosition / 17.0;
	if (y - (int)y > 0.0)
	{
		return (int)y + 1;
	}
	assert((int)y > 0 && (int)y <= 11);
	return (int)y;
}


int CBattleHelper::GetShortestDistance(int pointA, int pointB)
{
	/**
	 * TODO: function hasn't been checked!
	 */
	int x1 = DecodeXPosition(pointA);
	int y1 = DecodeYPosition(pointA);
	//
	int x2 = DecodeXPosition(pointB);
	//x2 += (x2 % 2)? 0 : 1;
	int y2 = DecodeYPosition(pointB);
	//
	double dx = x1 - x2;
	double dy = y1 - y2;

	return (int)sqrt(dx * dx + dy * dy);
}

int CBattleHelper::GetDistanceWithObstacles(int pointA, int pointB)
{
	// TODO - implement this!
	return GetShortestDistance(pointA, pointB);
}

/**
 *	Implementation of CBattleLogic class.
 */
CBattleLogic::CBattleLogic(ICallback *cb,  CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side) :
	m_cb(cb),
	m_iCurrentTurn(-2),
	m_bIsAttacker(false),
	m_army1(army1),
	m_army2(army2),
	m_tile(tile),
	m_hero1(hero1),
	m_hero2(hero2),
	m_side(side)
{
	const int max_enemy_creatures = 12;
	m_statMaxDamage.reserve(max_enemy_creatures);
	m_statMinDamage.reserve(max_enemy_creatures);

	m_statMaxSpeed.reserve(max_enemy_creatures);
	m_statDistance.reserve(max_enemy_creatures);
	m_statDistanceFromShooters.reserve(max_enemy_creatures);
	m_statHitPoints.reserve(max_enemy_creatures);
}

CBattleLogic::~CBattleLogic()
{}

void CBattleLogic::SetCurrentTurn(int turn)
{
	m_iCurrentTurn = turn;
}

void CBattleLogic::MakeStatistics(int currentCreatureId)
{
	typedef std::map<int, CStack> map_stacks;
	map_stacks allStacks = m_cb->battleGetStacks();
	const CStack *currentStack = m_cb->battleGetStackByID(currentCreatureId);
	/*
	// find all creatures belong to the enemy
	std::for_each(allStacks.begin(), allStacks.end(),
		if_(bind<ui8>(&CStack::attackerOwned, bind<CStack>(&map_stacks::value_type::second, _1)) == m_bIsAttacker)
		[
			var(enemy)[ret<int>(bind<int>(&map_stacks::value_type::first, _1))] =
				ret<CStack>(bind<CStack>(&map_stacks::value_type::second, _1))
		]
	);
	// fill other containers
	// max damage
	std::for_each(enemy.begin(), enemy.end(),
		var(m_statMaxDamage)[ret<int>(bind<int>(&map_stacks::value_type::first, _1))] =
			ret<int>(bind<int>(&CCreature::damageMax, bind<CCreature*>(&CStack::creature,
				bind<CStack>(&map_stacks::value_type::second, _1))))
	);
	// min damage
	std::for_each(enemy.begin(), enemy.end(),
		var(m_statMinDamage)[ret<int>(bind<int>(&map_stacks::value_type::first, _1))] =
			ret<int>(bind<int>(&CCreature::damageMax, bind<CCreature*>(&CStack::creature,
				bind<CStack>(&map_stacks::value_type::second, _1))))
	);
	*/
	m_statMaxDamage.clear();
	m_statMinDamage.clear();
	m_statHitPoints.clear();
	m_statMaxSpeed.clear();
	m_statDistanceFromShooters.clear();
	m_statDistance.clear();
	m_statDistance.clear();

	m_statCasualties.clear();

	int totalEnemyDamage = 0;
	int totalEnemyHitPoints = 0;

	int totalDamage = 0;
	int totalHitPoints = 0;

	for (map_stacks::const_iterator it = allStacks.begin(); it != allStacks.end(); ++it)
	{
		const CStack *st = &it->second;

		if ((it->second.attackerOwned != 0) != m_bIsAttacker)
		{
			int id = it->first;
			if (st->amount < 1)
			{
				continue;
			}
			// make stats
			int hitPoints = st->amount * st->creature->hitPoints - (st->creature->hitPoints - st->firstHPleft);

			m_statMaxDamage.push_back(std::pair<int, int>(id, st->creature->damageMax * st->amount));
			m_statMinDamage.push_back(std::pair<int, int>(id, st->creature->damageMin * st->amount));
			m_statHitPoints.push_back(std::pair<int, int>(id, hitPoints));
			m_statMaxSpeed.push_back(std::pair<int, int>(id, st->creature->speed));

			totalEnemyDamage += (st->creature->damageMax + st->creature->damageMin) * st->amount / 2;
			totalEnemyHitPoints += hitPoints;

			// calculate casualties
			SCreatureCasualties cs;
			// hp * amount - damage * ( (att - def)>=0 )
			// hit poionts
			assert(hitPoints >= 0 && "CGeniusAI - creature cannot have hit points less than zero");

			CGHeroInstance *attackerHero = (m_side)? m_hero1 : m_hero2;
			CGHeroInstance *defendingHero = (m_side)? m_hero2 : m_hero1;

			int attackDefenseBonus = currentStack->creature->attack + (attackerHero ? attackerHero->getPrimSkillLevel(0) : 0) - (st->creature->defence + (defendingHero ? defendingHero->getPrimSkillLevel(1) : 0));
			float damageFactor = 1.0f;
			if(attackDefenseBonus < 0) //decreasing dmg
			{
				if(0.02f * (-attackDefenseBonus) > 0.3f)
				{
					damageFactor += -0.3f;
				}
				else
				{
					damageFactor += 0.02f * attackDefenseBonus;
				}
			}
			else //increasing dmg
			{
				if(0.05f * attackDefenseBonus > 4.0f)
				{
					damageFactor += 4.0f;
				}
				else
				{
					damageFactor += 0.05f * attackDefenseBonus;
				}
			}

			cs.damage_max = (int)(currentStack->creature->damageMax * currentStack->amount * damageFactor);
			if (cs.damage_max > hitPoints)
			{
				cs.damage_max = hitPoints;
			}

			cs.damage_min = (int)(currentStack->creature->damageMin * currentStack->amount * damageFactor);
			if (cs.damage_min > hitPoints)
			{
				cs.damage_min = hitPoints;
			}

			cs.amount_max = cs.damage_max / st->creature->hitPoints;
			cs.amount_min = cs.damage_min / st->creature->hitPoints;

			cs.leftHitPoints_for_max = (hitPoints - cs.damage_max) % st->creature->hitPoints;
			cs.leftHitPoint_for_min = (hitPoints - cs.damage_min) % st->creature->hitPoints;

			m_statCasualties.push_back(std::pair<int, SCreatureCasualties>(id, cs));

			if (st->creature->isShooting() && st->shots > 0)
			{
				m_statDistanceFromShooters.push_back(std::pair<int, int>(id, m_battleHelper.GetShortestDistance(currentStack->position, st->position)));
			}

			if (currentStack->creature->isFlying() || (currentStack->creature->isShooting() && currentStack->shots > 0))
			{
				m_statDistance.push_back(std::pair<int, int>(id, m_battleHelper.GetShortestDistance(currentStack->position, st->position)));
			}
			else
			{
				m_statDistance.push_back(std::pair<int, int>(id, m_battleHelper.GetDistanceWithObstacles(currentStack->position, st->position)));
			}
		}
		else
		{
			if (st->amount < 1)
			{
				continue;
			}
			int hitPoints = st->amount * st->creature->hitPoints - (st->creature->hitPoints - st->firstHPleft);

			totalDamage += (st->creature->damageMax + st->creature->damageMin) * st->amount / 2;
			totalHitPoints += hitPoints;
		}
	}
	if ((float)totalDamage / (float)totalEnemyDamage < 0.5f &&
		(float)totalHitPoints / (float)totalEnemyHitPoints < 0.5f)
	{
		m_bEnemyDominates = true;
		MsgBox("** EnemyDominates!");
	}
	else
	{
		m_bEnemyDominates = false;
	}
	// sort max damage
	std::sort(m_statMaxDamage.begin(), m_statMaxDamage.end(),
		bind(&creature_stat::value_type::second, _1) > bind(&creature_stat::value_type::second, _2));
	// sort min damage
	std::sort(m_statMinDamage.begin(), m_statMinDamage.end(),
		bind(&creature_stat::value_type::second, _1) > bind(&creature_stat::value_type::second, _2));
	// sort max speed
	std::sort(m_statMaxSpeed.begin(), m_statMaxSpeed.end(),
		bind(&creature_stat::value_type::second, _1) > bind(&creature_stat::value_type::second, _2));
	// sort distance
	std::sort(m_statDistance.begin(), m_statDistance.end(),
		bind(&creature_stat::value_type::second, _1) < bind(&creature_stat::value_type::second, _2));
	// sort distance from shooters
	std::sort(m_statDistanceFromShooters.begin(), m_statDistanceFromShooters.end(),
		bind(&creature_stat::value_type::second, _1) < bind(&creature_stat::value_type::second, _2));
	// sort hit points
	std::sort(m_statHitPoints.begin(), m_statHitPoints.end(),
		bind(&creature_stat::value_type::second, _1) > bind(&creature_stat::value_type::second, _2));
	// sort casualties
	std::sort(m_statCasualties.begin(), m_statCasualties.end(),
		bind(&creature_stat_casualties::value_type::second_type::damage_max, bind(&creature_stat_casualties::value_type::second, _1))
		>
		bind(&creature_stat_casualties::value_type::second_type::damage_max, bind(&creature_stat_casualties::value_type::second, _2)));
}

BattleAction CBattleLogic::MakeDecision(int stackID)
{
	MakeStatistics(stackID);

	list<int> creatures;
	int additionalInfo;

	if (m_bEnemyDominates)
	{
		creatures = PerformBerserkAttack(stackID, additionalInfo);
	}
	else
	{
		creatures = PerformDefaultAction(stackID, additionalInfo);
	}
	/*
	std::string message("Creature will be attacked - ");
	message += boost::lexical_cast<std::string>(creature_to_attack);
	MsgBox(message.c_str());
	*/

	if (additionalInfo == -1 || creatures.empty())
	{
		// defend
		return MakeDefend(stackID);
	}
	else if (additionalInfo == -2)
	{
		return MakeWait(stackID);
	}

	list<int>::iterator it, eit;
	eit = creatures.end();
	for (it = creatures.begin(); it != eit; ++it)
	{
		BattleAction ba = MakeAttack(stackID, *it);
		if (ba.actionType != action_walk_and_attack)
		{
			continue;
		}
		else
		{
			PrintBattleAction(ba);
			return ba;
		}
	}
	BattleAction ba = MakeAttack(stackID, *creatures.begin());
	return ba;
}



std::vector<int> CBattleLogic::GetAvailableHexesForAttacker(CStack *defender, CStack *attacker)
{
	int x = m_battleHelper.DecodeXPosition(defender->position);
	int y = m_battleHelper.DecodeYPosition(defender->position);
	bool defenderIsDW = defender->creature->isDoubleWide();
	bool attackerIsDW = attacker->creature->isDoubleWide();
	// TOTO: should be std::vector<int> but for debug purpose std::pair is used
	typedef std::pair<int, int> hexPoint;
	std::list<hexPoint> candidates;
	std::vector<int> fields;
	if (defenderIsDW)
	{
		if (defender->attackerOwned)
		{
			// from left side
			if (!(y % 2))
			{
				// up
				candidates.push_back(hexPoint(x - 2, y - 1));
				candidates.push_back(hexPoint(x - 1, y - 1));
				candidates.push_back(hexPoint(x, y - 1));
				// down
				candidates.push_back(hexPoint(x - 2, y + 1));
				candidates.push_back(hexPoint(x - 1, y + 1));
				candidates.push_back(hexPoint(x, y + 1));
			}
			else
			{
				// up
				candidates.push_back(hexPoint(x - 1, y - 1));
				candidates.push_back(hexPoint(x, y - 1));
				candidates.push_back(hexPoint(x + 1, y - 1));
				// down
				candidates.push_back(hexPoint(x - 1, y + 1));
				candidates.push_back(hexPoint(x, y + 1));
				candidates.push_back(hexPoint(x + 1, y + 1));

			}
			candidates.push_back(hexPoint(x - 2, y));
			candidates.push_back(hexPoint(x + 1, y));

		}
		else
		{
			// from right
			if (!(y % 2))
			{
				// up
				candidates.push_back(hexPoint(x - 1, y - 1));
				candidates.push_back(hexPoint(x, y - 1));
				candidates.push_back(hexPoint(x + 1, y - 1));
				// down
				candidates.push_back(hexPoint(x - 1, y + 1));
				candidates.push_back(hexPoint(x, y + 1));
				candidates.push_back(hexPoint(x + 1, y + 1));
			}
			else
			{
				// up
				candidates.push_back(hexPoint(x, y - 1));
				candidates.push_back(hexPoint(x + 1, y - 1));
				candidates.push_back(hexPoint(x + 2, y - 1));
				// down
				candidates.push_back(hexPoint(x, y + 1));
				candidates.push_back(hexPoint(x + 1, y + 1));
				candidates.push_back(hexPoint(x + 2, y + 1));
			}
			candidates.push_back(hexPoint(x - 1, y));
			candidates.push_back(hexPoint(x + 2, y));
		}
	}
	else
	{
		if (!(y % 2)) // even line
		{
			// up
			candidates.push_back(hexPoint(x - 1, y - 1));
			candidates.push_back(hexPoint(x, y - 1));
			// down
			candidates.push_back(hexPoint(x - 1, y + 1));
			candidates.push_back(hexPoint(x, y + 1));
		}
		else // odd line
		{
			// up
			candidates.push_back(hexPoint(x, y - 1));
			candidates.push_back(hexPoint(x + 1, y - 1));
			// down
			candidates.push_back(hexPoint(x, y + 1));
			candidates.push_back(hexPoint(x + 1, y + 1));
		}

		candidates.push_back(hexPoint(x + 1, y));
		candidates.push_back(hexPoint(x - 1, y));
	}

	// remove fields which are out of bounds or obstacles
	for (std::list<hexPoint>::iterator it = candidates.begin(); it != candidates.end(); ++it)
	{
		if (it->first < 1 || it->first > m_battleHelper.BattlefieldWidth ||
			it->second < 1 || it->second > m_battleHelper.BattlefieldHeight)
		{
			// field is out of bounds
			//it = candidates.erase(it);
			continue;
		}

		int new_pos = m_battleHelper.GetBattleFieldPosition(it->first, it->second);
		CStack *st = m_cb->battleGetStackByPos(new_pos);

		if (st == NULL || st->amount < 1)
		{
			if (attackerIsDW)
			{
				int tail_pos = -1;
				if (attacker->attackerOwned) // left side
				{
					int tail_pos_x = it->first - 1;
					if (tail_pos_x < 1)
					{
						continue;
					}
					tail_pos = m_battleHelper.GetBattleFieldPosition(it->first, it->second);
				}
				else // right side
				{
					int tail_pos_x = it->first + 1;
					if (tail_pos_x > m_battleHelper.BattlefieldWidth)
					{
						continue;
					}
					tail_pos = m_battleHelper.GetBattleFieldPosition(it->first, it->second);
				}
				assert(tail_pos >= 0 && "Error during calculation position of double wide creature");
				CStack *tailStack = m_cb->battleGetStackByPos(tail_pos);
				if (st != NULL && st->amount >= 1)
				{
					continue;
				}
			}

			fields.push_back(new_pos);

		}
		else if (attacker)
		{
			if (attacker->ID == st->ID)
			{
				fields.push_back(new_pos);
			}
		}
		//
		//++it;
	}
	return fields;
}

BattleAction CBattleLogic::MakeDefend(int stackID)
{
	BattleAction ba;
	ba.side = 1;
	ba.actionType = action_defend;
	ba.stackNumber = stackID;
	ba.additionalInfo = -1;
	return ba;
}

BattleAction CBattleLogic::MakeWait(int stackID)
{
	BattleAction ba;
	ba.side = 1;
	ba.actionType = action_wait;
	ba.stackNumber = stackID;
	ba.additionalInfo = -1;
	return ba;
}

BattleAction CBattleLogic::MakeAttack(int attackerID, int destinationID)
{
	if (m_cb->battleCanShoot(attackerID, m_cb->battleGetPos(destinationID)))
	{
		// shoot
		BattleAction ba;
		ba.side = 1;
		ba.additionalInfo = -1;
		ba.actionType = action_shoot; // shoot
		ba.stackNumber = attackerID;
		ba.destinationTile = (ui16)m_cb->battleGetPos(destinationID);
		return ba;
	}
	else
	{
		// go or go&attack
		int dest_tile = -1;
		std::vector<int> av_tiles = GetAvailableHexesForAttacker(m_cb->battleGetStackByID(destinationID), m_cb->battleGetStackByID(attackerID));
		if (av_tiles.size() < 1)
		{
			return MakeDefend(attackerID);
		}

		// get the best tile - now the nearest

		int prev_distance = m_battleHelper.InfiniteDistance;
		int currentPos = m_cb->battleGetPos(attackerID);

		for (std::vector<int>::iterator it = av_tiles.begin(); it != av_tiles.end(); ++it)
		{
			int dist = m_battleHelper.GetDistanceWithObstacles(*it, m_cb->battleGetPos(attackerID));
			if (dist < prev_distance)
			{
				prev_distance = dist;
				dest_tile = *it;
			}
			if (*it == currentPos)
			{
				dest_tile = currentPos;
				break;
			}
		}

		std::vector<int> fields = m_cb->battleGetAvailableHexes(attackerID, false);

		if(fields.size() == 0)
		{
			return MakeDefend(attackerID);
		}

		BattleAction ba;
		ba.side = 1;
		//ba.actionType = 6; // go and attack
		ba.stackNumber = attackerID;
		ba.destinationTile = (ui16)dest_tile;
		//simplified checking for possibility of attack (previous was too simplified)
		int destStackPos = m_cb->battleGetPos(destinationID);
		if(BattleInfo::mutualPosition(dest_tile, destStackPos) != -1)
			ba.additionalInfo = destStackPos;
		else if(BattleInfo::mutualPosition(dest_tile, destStackPos+1) != -1)
			ba.additionalInfo = destStackPos+1;
		else if(BattleInfo::mutualPosition(dest_tile, destStackPos-1) != -1)
			ba.additionalInfo = destStackPos-1;
		else
			MakeDefend(attackerID);

		int nearest_dist = m_battleHelper.InfiniteDistance;
		int nearest_pos = -1;

		// if double wide calculate tail
		CStack *attackerStack = m_cb->battleGetStackByID(attackerID);
		assert(attackerStack != NULL);

		int tail_pos = -1;
		if (attackerStack->creature->isDoubleWide())
		{
			int x_pos = m_battleHelper.DecodeXPosition(attackerStack->position);
			int y_pos = m_battleHelper.DecodeYPosition(attackerStack->position);
			if (attackerStack->attackerOwned)
			{
				x_pos -= 1;
			}
			else
			{
				x_pos += 1;
			}
			// if creature can perform attack without movement - do it!
			tail_pos = m_battleHelper.GetBattleFieldPosition(x_pos, y_pos);
			if (dest_tile == tail_pos)
			{
				ba.additionalInfo = dest_tile;
				ba.actionType = action_walk_and_attack;
				PrintBattleAction(ba);
				return ba;
			}
		}

		for (std::vector<int>::const_iterator it = fields.begin(); it != fields.end(); ++it)
		{
			if (*it == dest_tile)
			{
				// attack!
				ba.actionType = action_walk_and_attack;
#if defined _DEBUG
				PrintBattleAction(ba);
#endif
				return ba;
			}
			int d = m_battleHelper.GetDistanceWithObstacles(dest_tile, *it);
			if (d < nearest_dist)
			{
				nearest_dist = d;
				nearest_pos = *it;
			}
		}
		string message;
		message = "Attacker position X=";
		message += boost::lexical_cast<std::string>(m_battleHelper.DecodeXPosition(nearest_pos)) + ", Y=";
		message += boost::lexical_cast<std::string>(m_battleHelper.DecodeYPosition(nearest_pos));
		MsgBox(message.c_str());

		ba.actionType = action_walk;
		ba.destinationTile = (ui16)nearest_pos;
		ba.additionalInfo  = -1;
#if defined _DEBUG
		PrintBattleAction(ba);
#endif
		return ba;
	}
}
/**
 * The main idea is to perform maximum casualties.
 */
list<int> CBattleLogic::PerformBerserkAttack(int stackID, int &additionalInfo)
{
	CCreature c = m_cb->battleGetCreature(stackID);
	// attack to make biggest damage
	list<int> creatures;

	if (!m_statCasualties.empty())
	{
		//creature_to_attack = m_statCasualties.begin()->first;
		creature_stat_casualties::iterator it = m_statCasualties.begin();
		for (; it != m_statCasualties.end(); ++it)
		{
			if (it->second.amount_min <= 0)
			{
				creatures.push_back(it->first);
				continue;
			}
			for (creature_stat::const_iterator it2 = m_statDistance.begin(); it2 != m_statDistance.end(); ++it2)
			{
				if (it2->first == it->first && it2->second - 1 <= c.speed)
				{
					creatures.push_front(it->first);
				}
			}
		}
		creatures.push_back(m_statCasualties.begin()->first);
	}
	return creatures;
}

list<int> CBattleLogic::PerformDefaultAction(int stackID, int &additionalInfo)
{
	// first approach based on the statistics and weights
	// if this solution was fine we would develop this idea
	//
	std::map<int, int> votes;

	for (creature_stat::iterator it = m_statMaxDamage.begin(); it != m_statMaxDamage.end(); ++it)
	{
		votes[it->first]  = 0;
	}

	votes[m_statMaxDamage.begin()->first] += m_battleHelper.GetVoteForMaxDamage();
	votes[m_statMinDamage.begin()->first] += m_battleHelper.GetVoteForMinDamage();
	if (m_statDistanceFromShooters.size())
	{
		votes[m_statDistanceFromShooters.begin()->first] += m_battleHelper.GetVoteForDistanceFromShooters();
	}
	votes[m_statDistance.begin()->first] += m_battleHelper.GetVoteForDistance();
	votes[m_statHitPoints.begin()->first] += m_battleHelper.GetVoteForHitPoints();
	votes[m_statMaxSpeed.begin()->first] += m_battleHelper.GetVoteForMaxSpeed();


	// get creature to attack

	int max_vote = 0;

	list<int> creatures;

	for (std::map<int, int>::iterator it = votes.begin(); it != votes.end(); ++it)
	{
		if (it->second > max_vote)
		{
			max_vote = it->second;
			creatures.push_front(it->first);
		}
	}
	additionalInfo = 0; // list contains creatures which shoud be attacked
	return creatures;
}

void CBattleLogic::PrintBattleAction(const BattleAction &action) // for debug purpose
{
	std::string message("Battle action \n");
	message += "\taction type - ";
	switch (action.actionType)
	{
	case 0:
		message += "Cancel BattleAction\n";
		break;
	case 1:
		message += "Hero cast a spell\n";
		break;
	case 2:
		message += "Walk\n";
		break;
	case 3:
		message += "Defend\n";
		break;
	case 4:
		message += "Retreat from the battle\n";
		break;
	case 5:
		message += "Surrender\n";
		break;
	case 6:
		message += "Walk and Attack\n";
		break;
	case 7:
		message += "Shoot\n";
		break;
	case 8:
		message += "Wait\n";
		break;
	case 9:
		message += "Catapult\n";
		break;
	case 10:
		message += "Monster casts a spell\n";
		break;
	}
	message += "\tDestination tile: X = ";
	message += boost::lexical_cast<std::string>(m_battleHelper.DecodeXPosition(action.destinationTile));
	message += ", Y = " + boost::lexical_cast<std::string>(m_battleHelper.DecodeYPosition(action.destinationTile));
	message += "\nAdditional info: ";
	if (action.actionType == 6)// || action.actionType == 7)
	{
		message += "stack - " + boost::lexical_cast<std::string>(m_battleHelper.DecodeXPosition(action.additionalInfo));
		message += ", " + boost::lexical_cast<std::string>(m_battleHelper.DecodeYPosition(action.additionalInfo));
		message += ", creature - ";
		CStack *c = m_cb->battleGetStackByPos(action.additionalInfo);
		if (c && c->creature)
		{
			message += c->creature->nameRef;
		}
		else
		{
			message += "NULL";
		}
	}
	else
	{
		message += boost::lexical_cast<std::string>(action.additionalInfo);
	}

#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

#else
	std::string color;
	color = "\x1b[1;40;32m";
	std::cout << color;
#endif

	std::cout << message.c_str() << std::endl;

#ifdef _WIN32
	SetConsoleTextAttribute(hConsole, csbi.wAttributes);
#else
	color = "\x1b[0m";
	std::cout << color;
#endif
}
