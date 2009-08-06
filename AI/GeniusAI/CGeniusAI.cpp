#include "CGeniusAI.h"
#include <iostream>
#include "../../hch/CBuildingHandler.h"
#include "../../hch/CHeroHandler.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/NetPacks.h"
using namespace std;
using namespace GeniusAI;

#if defined (_MSC_VER) && (_MSC_VER >= 1020) || (__MINGW32__)
#include <windows.h>
#endif

void DbgBox(const char *msg, bool messageBox)
{
#if defined PRINT_DEBUG
#if defined _DEBUG
//#if 0
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


bool CGeniusAI::AIObjectContainer::operator<(const AIObjectContainer& b)const
{
	if (o->pos!=b.o->pos)
		return o->pos<b.o->pos;
	return o->id<b.o->id;
}

CGeniusAI::HypotheticalGameState::HeroModel::HeroModel(const CGHeroInstance * h)
:h(h),finished(false)
{
	pos = h->getPosition(false);remainingMovement = h->movement;
}

CGeniusAI::HypotheticalGameState::TownModel::TownModel(const CGTownInstance *t):t(t)
{
	hasBuilt = t->builded;
	creaturesToRecruit = t->creatures;
	creaturesInGarrison = t->army;
}

CGeniusAI::HypotheticalGameState::HypotheticalGameState(CGeniusAI & AI)
	:knownVisitableObjects(AI.knownVisitableObjects)
{
	std::vector < const CGHeroInstance *> heroes = AI.m_cb->getHeroesInfo();	
	for(std::vector < const CGHeroInstance *>::iterator i = heroes.begin(); i != heroes.end(); i++)
		heroModels.push_back(HeroModel(*i));
	
	std::vector < const CGTownInstance *> towns = AI.m_cb->getTownsInfo();	
	for(std::vector < const CGTownInstance *>::iterator i = towns.begin(); i != towns.end(); i++)
		if((*i)->tempOwner==AI.m_cb->getMyColor())
		townModels.push_back(TownModel(*i));

	if(AI.m_cb->howManyTowns()!=0)
		AvailableHeroesToBuy = AI.m_cb->getAvailableHeroes(AI.m_cb->getTownInfo(0,0));

	for(int i = 0; i < 8;i++)resourceAmounts.push_back(AI.m_cb->getResourceAmount(i));
}

void CGeniusAI::HypotheticalGameState::update(CGeniusAI & AI)
{
	knownVisitableObjects = AI.knownVisitableObjects;

	std::vector<HeroModel> oldModels = heroModels;
	heroModels.clear();
	std::vector < const CGHeroInstance *> heroes = AI.m_cb->getHeroesInfo();	
	for(std::vector < const CGHeroInstance *>::iterator i = heroes.begin(); i != heroes.end(); i++)
		heroModels.push_back(HeroModel(*i));
	for(int i = 0; i < oldModels.size();i++)
		for(int ii = 0; ii < heroModels.size();ii++)
			if(oldModels[i].finished&&oldModels[i].h->id==heroModels[ii].h->id)
				heroModels[ii].finished = true;
	
	townModels.clear();
	std::vector < const CGTownInstance *> towns = AI.m_cb->getTownsInfo();	
	for(std::vector < const CGTownInstance *>::iterator i = towns.begin(); i != towns.end(); i++)
		if((*i)->tempOwner==AI.m_cb->getMyColor())
			townModels.push_back(TownModel(*i));

	if(AI.m_cb->howManyTowns()!=0)
		AvailableHeroesToBuy = AI.m_cb->getAvailableHeroes(AI.m_cb->getTownInfo(0,0));

	resourceAmounts.clear();
	for(int i = 0; i < 8;i++)resourceAmounts.push_back(AI.m_cb->getResourceAmount(i));
}

CGeniusAI::HeroObjective::HeroObjective(Type t,const CGObjectInstance * object,HypotheticalGameState::HeroModel *h):object(object)
{
	pos = object->pos;
	type = t;
	whoCanAchieve.push_back(h);
	_value = 100 + rand()%30;
}

bool CGeniusAI::HeroObjective::operator < (const HeroObjective &other)const
{
	if(type != other.type)
		return type<other.type;
	if(pos!=other.pos)
		return pos < other.pos;
	if(object->id!=other.object->id)
		return object->id < other.object->id;
	return false;
}

CGeniusAI::TownObjective::TownObjective(Type t,HypotheticalGameState::TownModel * tn,int Which)
:whichTown(tn),which(Which)
{
	type = t;
	_value = 100 + rand()%30 + (t==upgradeCreatures)*200;
}

bool CGeniusAI::TownObjective::operator < (const TownObjective &other)const
{
	if(type != other.type)
		return type<other.type;
	if(which!=other.which)
		return which<other.which;
	if(whichTown->t->id!=other.whichTown->t->id)
		return whichTown->t->id < other.whichTown->t->id;
	return false;
}

CGeniusAI::CGeniusAI()
	: m_generalAI(), m_state(NO_BATTLE)
{
}

CGeniusAI::~CGeniusAI()
{
}

void CGeniusAI::init(ICallback *CB)
{
	m_cb = CB;
	m_generalAI.init(CB);

	human = false;
	playerID = m_cb->getMyColor();
	serialID = m_cb->getMySerial();
	std::string info = std::string("GeniusAI initialized for player ") + boost::lexical_cast<std::string>(playerID);
	m_battleLogic = NULL;
	DbgBox(info.c_str());
}


void CGeniusAI::reportResources()
{
	cout << "Day " << m_cb->getDate() << ": ";
	cout << "AI Player " <<m_cb->getMySerial()<< " with " <<  m_cb->howManyHeroes(true) << " heroes. " << endl;
	
	cout << m_cb->getResourceAmount(0) << " wood. ";
	cout << m_cb->getResourceAmount(1) << " mercury. ";
	cout << m_cb->getResourceAmount(2) << " ore. ";
	cout << m_cb->getResourceAmount(3) << " sulfer. ";
	cout << m_cb->getResourceAmount(4) << " cristal. ";
	cout << m_cb->getResourceAmount(5) << " gems. ";
	cout << m_cb->getResourceAmount(6) << " gold.";
	cout << endl;
}

void CGeniusAI::addHeroObjectives(CGeniusAI::HypotheticalGameState::HeroModel &h, CGeniusAI::HypotheticalGameState &hgs)
{
	int3 hpos, destination;
	CPath path;
	hpos = h.pos;
	int movement = h.remainingMovement;
	int3 interestingPos;
	int maxInteresting=0;
	AIObjective::Type tp = AIObjective::visit;
	if(h.finished) return;
	for(std::set<AIObjectContainer>::const_iterator i = hgs.knownVisitableObjects.begin(); i != hgs.knownVisitableObjects.end();i++)
	{
		//TODO: what would the hero actually visit if he went to that spot
		//      IE maybe the hero wants to visit a seemingly unguarded enemy town, but there is a hero on top of it.

		if(i->o->ID!=34)			//unless you are trying to visit a hero
		{
			bool heroThere = false;
			for(int ii = 0; ii < hgs.heroModels.size();ii++)
				if(hgs.heroModels[ii].pos==i->o->getSightCenter())
					heroThere = true;
			if(heroThere)			//it won't work if there is already someone visiting that spot.
				continue;
		}
		if(i->o->getOwner()!=m_cb->getMyColor())	
		{
			int enemyStrength = 0;							//TODO: I feel like the AI shouldn't have access to this information.
															//      We must get an approximation based on few, many, ... zounds etc.
			if(dynamic_cast<const CArmedInstance *> (i->o))
				enemyStrength = (dynamic_cast<const CArmedInstance *> (i->o))->getArmyStrength();//TODO: should be virtual maybe, Army strength should be comparable across objects
			if(dynamic_cast<const CGHeroInstance *> (i->o))
				enemyStrength = (dynamic_cast<const CGHeroInstance *> (i->o))->getHeroStrength();
			if(dynamic_cast<const CGTownInstance *> (i->o))
				enemyStrength = (dynamic_cast<const CGTownInstance *> (i->o))->getArmyStrength()*1.5;
			
			if(enemyStrength*1.5 > h.h->getHeroStrength())  //TODO: ballence these numbers using objective cost formula.
				continue;
			if(enemyStrength!=0)tp  = AIObjective::attack;
		}

		if(i->o->ID==53&&i->o->getOwner()==m_cb->getMyColor())//don't visit a mine if you own, there's almost no point(maybe to leave guards or because the hero's trapped).
			continue;

		destination = i->o->getSightCenter();

		if(hpos.z==destination.z)							//don't try to take a path from the underworld to the top or vice versa
		{
			if(m_cb->getPath(hpos,destination,h.h,path))
			{
				path.convert(0);
				if(path.nodes[0].dist<movement)
				{
					
					HeroObjective ho(tp,i->o,&h);
					std::set<HeroObjective>::iterator found = currentHeroObjectives.find(ho);
					if(found==currentHeroObjectives.end())
						currentHeroObjectives.insert(ho);
					else
						found->whoCanAchieve.push_back(&h);
				}

				// find the most interesting object that is eventually reachable, and set that position to the ultimate goal position
				int hi = rand(); //TODO: replace random numbers with some sort of ranking system

				if(hi>maxInteresting)
				{
					maxInteresting = hi;
					interestingPos = destination;
				}
			}		

		}
	}
	
	h.interestingPos = interestingPos;
	if(h.remainingMovement>0&&m_cb->getPath(hpos,interestingPos,h.h,path)) // there ought to be a path   
		currentHeroObjectives.insert(HeroObjective(HeroObjective::finishTurn,h.h,&h));
		

}

void CGeniusAI::HeroObjective::fulfill(CGeniusAI & cg,HypotheticalGameState & hgs)
{
	
	cg.m_cb->waitTillRealize = true;
	HypotheticalGameState::HeroModel * h;
	int3 hpos, destination;
	CPath path;
	CPath path2;
	int3 bestPos,currentPos,checkPos;
	int howGood;
	switch(type)
	{
	case finishTurn:
		h = whoCanAchieve.front();
		hpos = h->pos;
		destination = h->interestingPos;
		if(!cg.m_cb->getPath(hpos,destination,h->h,path)) {cout << "AI error: invalid destination" << endl; return;}
//		path.convert(0);
		destination = h->pos;
		for(int i = path.nodes.size()-2;i>=0;i--)		//find closest coord that we can get to
			if(cg.m_cb->getPath(hpos,path.nodes[i].coord,h->h,path2)&&path2.nodes[0].dist<=h->remainingMovement)
				destination = path.nodes[i].coord;

		if(destination == h->interestingPos) break;
		
		///////// Find close pos with the most neighboring empty squares. We don't want to get in the way. ///////////////////
		bestPos = destination;
		howGood=0;
		for(int x = -3;x <= 3;x++)
			for(int y = -3;y <= 3;y++)
			{
				currentPos=destination+int3(x,y,0);
				if(cg.m_cb->getVisitableObjs(currentPos).size()!=0)			//there better not be anything there
					continue;
				if(!cg.m_cb->getPath(hpos,currentPos,h->h,path)||path.nodes[0].dist>h->remainingMovement)				//it better be reachable from the hero
					continue;
				
				int count = 0;
				for(int xx = -1;xx <= 1;xx++)
					for(int yy = -1;yy <= 1;yy++)
					{
						checkPos = currentPos+int3(xx,yy,0);
						if(cg.m_cb->getPath(currentPos,checkPos,h->h,path))
							count++;
					}
				if(count > howGood)
				{
					howGood = count;
					bestPos = currentPos;
				}
			}
			
		destination = bestPos;
		h->finished=true;
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		break;
	case visit:case attack:
		//std::cout << "trying to visit " << object->hoverName << std::endl;
		h = whoCanAchieve[rand()%whoCanAchieve.size()];//TODO:replace with best hero for the job
		hpos = h->pos;
		destination = object->getSightCenter();

		break;
		


	}
	if(type == visit||type == finishTurn)
	if(cg.m_cb->getPath(hpos,destination,h->h,path))
	{
		path.convert(0);
		
		if(cg.m_state.get() != NO_BATTLE)
			cg.m_state.waitUntil(NO_BATTLE);//wait for battle end
												//wait over, battle over too. hero might be killed. check.
		for(int i = path.nodes.size()-2;i>=0&&(cg.m_cb->getHeroSerial(h->h) >= 0);i--)
		{
			cg.m_cb->moveHero(h->h,path.nodes[i].coord);

			if(cg.m_state.get() != NO_BATTLE)
				cg.m_state.waitUntil(NO_BATTLE);//wait for battle end
		}

		h->remainingMovement-=path.nodes[0].dist;
		if(object->blockVisit)
			h->pos = path.nodes[1].coord;
		else
			h->pos=destination;
		std::set<AIObjectContainer>::iterator i = hgs.knownVisitableObjects.find(AIObjectContainer(object));
		if(i!=hgs.knownVisitableObjects.end())
			hgs.knownVisitableObjects.erase(i);
	}

	const CGTownInstance * town = dynamic_cast<const CGTownInstance *> (object);
	if(town&&object->getOwner()==cg.m_cb->getMyColor())
	{
		//upgrade hero's units
		cout << "visiting town" << endl;
		CCreatureSet hcreatures = h->h->army;
		for(std::map<si32,std::pair<ui32,si32> >::const_iterator i = hcreatures.slots.begin();i!=hcreatures.slots.end();i++) // for each hero slot
		{
			UpgradeInfo ui = cg.m_cb->getUpgradeInfo(h->h,i->first);

			bool canUpgrade = false;
			if(ui.newID.size()!=0)						//does this stack need upgrading?
			{
				canUpgrade = true;
				for(int ii=0;ii<ui.cost.size();ii++)//can afford the upgrade?
					for (std::set<std::pair<int,int> >::iterator j=ui.cost[ii].begin(); j!=ui.cost[ii].end(); j++)
						if(hgs.resourceAmounts[j->first] < j->second*i->second.second)
							canUpgrade = false;
			}
			if(canUpgrade)
			{
				cg.m_cb->upgradeCreature(h->h,i->first,ui.newID.back());
				cout << "upgrading hero's " << VLC->creh->creatures[i->second.first].namePl << endl;
			}
		}

		//give town's units to hero
		CCreatureSet tcreatures = town->army;
		for(std::map<si32,std::pair<ui32,si32> >::const_iterator i = tcreatures.slots.begin();i!=tcreatures.slots.end();i++) // for each town slot
		{
			hcreatures = h->h->army;
			int hSlot = hcreatures.getSlotFor(i->second.first);
			if(hSlot == -1) continue;
			cout << "giving hero " << VLC->creh->creatures[i->second.first].namePl << endl;
			if(hcreatures.slots.find(hSlot)!=hcreatures.slots.end())
			{
				cg.m_cb->mergeStacks(town,h->h,i->first,hSlot);	//TODO: the comment says that this code is not safe for the AI.
			}
			else
			{
				cg.m_cb->swapCreatures(town,h->h,i->first,hSlot);	
			}
		}

	}
	
}

void CGeniusAI::addTownObjectives(HypotheticalGameState::TownModel &t, HypotheticalGameState & hgs)
{
	//recruitHero
	//buildBuilding
	//recruitCreatures
	//upgradeCreatures
	if(hgs.heroModels.size()<3&&hgs.resourceAmounts[6]>=2500) //recruitHero
	{
		bool heroAtTown = false;
		for(int i = 0; i < hgs.heroModels.size();i++)
			if(hgs.heroModels[i].pos==t.t->getSightCenter())
				heroAtTown = true;
		if(!heroAtTown  &&  vstd::contains(t.t->builtBuildings, 5)) //no visiting hero and built tavern

		{
			for(int i =0; i < hgs.AvailableHeroesToBuy.size();i++)
				if(hgs.AvailableHeroesToBuy[i]!=NULL&&(t.t->subID==(hgs.AvailableHeroesToBuy[i]->type->heroType/2)))
				{
					TownObjective to(AIObjective::recruitHero,&t,0);
					currentTownObjectives.insert(to);
				}
		}
	}
		//buildBuilding
	if(!t.hasBuilt)
	{
		std::map<int, CBuilding *> thisTownsBuildings = VLC->buildh->buildings[t.t->subID];// m_cb->getCBuildingsByID(t.t);
		for(std::map<int, CBuilding *>::iterator i = thisTownsBuildings.begin(); i != thisTownsBuildings.end();i++)
		{
			if(m_cb->canBuildStructure(t.t,i->first)==7)
			{
				TownObjective to(AIObjective::buildBuilding,&t,i->first);
				currentTownObjectives.insert(to);
				//cout <<"can build " << i->first << " "<< i->second->Name() << endl;
			}
		}
	}
	
	//recruitCreatures

	for(int i = 0; i < t.creaturesToRecruit.size() ;i++)
	{
		if(t.creaturesToRecruit[i].first==0) continue;
		int ID = t.creaturesToRecruit[i].second.back();
		
		const CCreature *creature = &VLC->creh->creatures[ID];//m_cb->getCCreatureByID(ID);
		bool canAfford = true;
		for(int ii = 0;ii<creature->cost.size();ii++)
			if(creature->cost[ii]>hgs.resourceAmounts[ii])
				canAfford = false;							// can we afford at least one creature?
		if(!canAfford) continue;
				
		//cout << "town has " << t.t->creatures[i].first  << " "<< creature->namePl << " (AI Strength " << creature->AIValue << ")." << endl;
		TownObjective to(AIObjective::recruitCreatures,&t,i);
		currentTownObjectives.insert(to);

	}
	//upgradeCreatures


	for(std::map<si32,std::pair<ui32,si32> >::iterator i = t.creaturesInGarrison.slots.begin();i!=t.creaturesInGarrison.slots.end();i++)
	{
		UpgradeInfo ui = m_cb->getUpgradeInfo(t.t,i->first);
		if(ui.newID.size()!=0)
		{
			//int newID = ui.newID.back();
			//we have some. Can we afford to upgrade them?
		//	const CCreature *creature = &VLC->creh->creatures[ID];//m_cb->getCCreatureByID(ID);
		//	const CCreature *upgrade = &VLC->creh->creatures[newID];//m_cb->getCCreatureByID(ID);
			bool canAfford = true;
			for(int ii=0;ii<ui.cost.size();ii++)
				for (std::set<std::pair<int,int> >::iterator j=ui.cost[ii].begin(); j!=ui.cost[ii].end(); j++)
					if(hgs.resourceAmounts[j->first] < j->second*i->second.second)
						canAfford = false;
			if(canAfford)
			{
				TownObjective to(AIObjective::upgradeCreatures,&t,i->first);
				currentTownObjectives.insert(to);
			}
		}
	}

}

void CGeniusAI::TownObjective::fulfill(CGeniusAI & cg,HypotheticalGameState &hgs)
{
	
	cg.m_cb->waitTillRealize = true;
	CBuilding * b;
	const CCreature *creature;
	HypotheticalGameState::HeroModel hm;
	int ID, howMany, newID, hSlot;
	switch(type)
	{
	case recruitHero:
		cg.m_cb->recruitHero(whichTown->t,hgs.AvailableHeroesToBuy[which]);
		hm = HypotheticalGameState::HeroModel(hgs.AvailableHeroesToBuy[which]);
		hm.pos = whichTown->t->getSightCenter();
		hm.remainingMovement = hm.h->maxMovePoints(true);
		hgs.heroModels.push_back(hm);
		hgs.resourceAmounts[6]-=2500;
		break;
	case buildBuilding:
		b = VLC->buildh->buildings[whichTown->t->subID][which];
		if(cg.m_cb->canBuildStructure(whichTown->t,which)==7)
		{
			cout << "built " << b->Name() << "." << endl;
			if(!cg.m_cb->buildBuilding(whichTown->t,which)) cout << "really tried to build unbuildable building" <<endl;
			for(int i = 0; b && i < b->resources.size();i++)
				hgs.resourceAmounts[i]-=b->resources[i];
		}
		else cout << "trying to build a structure we cannot build" << endl;
		
		whichTown->hasBuilt=true;
		break;
	case recruitCreatures:
		ID = whichTown->creaturesToRecruit[which].second.back();				//buy upgraded if possible
		creature = &VLC->creh->creatures[ID];
		howMany = whichTown->creaturesToRecruit[which].first;
		for(int i = 0; i < creature->cost.size();i++)
			howMany = min(howMany,creature->cost[i]?hgs.resourceAmounts[i]/creature->cost[i]:INT_MAX);
		if(howMany == 0) cout << "tried to recruit without enough money.";
		cout << "recruiting " << howMany  << " "<< creature->namePl << " (Total AI Strength " << creature->AIValue*howMany << ")." << endl;
		cg.m_cb->recruitCreatures(whichTown->t,ID,howMany);

		break;
	case upgradeCreatures:
		UpgradeInfo ui = cg.m_cb->getUpgradeInfo(whichTown->t,which);
		ID = whichTown->creaturesInGarrison.slots[which].first;
		newID = ui.newID.back();
		cg.m_cb->upgradeCreature(whichTown->t,which,newID);//TODO: reduce resources in hgs
		cout << "upgrading " << VLC->creh->creatures[ID].namePl << endl;
			
		break;
		

	}
	
}

void CGeniusAI::fillObjectiveQueue(HypotheticalGameState & hgs)
{
	objectiveQueue.clear();
	currentHeroObjectives.clear();
	currentTownObjectives.clear();
	
	for(std::vector <CGeniusAI::HypotheticalGameState::HeroModel>::iterator i = hgs.heroModels.begin(); i != hgs.heroModels.end(); i++)
		addHeroObjectives(*i,hgs);

	for(std::vector <CGeniusAI::HypotheticalGameState::TownModel>::iterator i = hgs.townModels.begin(); i != hgs.townModels.end(); i++)
		addTownObjectives(*i,hgs);
	for(std::set<CGeniusAI::HeroObjective>::iterator i = currentHeroObjectives.begin(); i != currentHeroObjectives.end(); i++)
		objectiveQueue.push_back(AIObjectivePtrCont(&(*i)));
	for(std::set<CGeniusAI::TownObjective>::iterator i = currentTownObjectives.begin(); i != currentTownObjectives.end(); i++)
		objectiveQueue.push_back(AIObjectivePtrCont(&(*i)));
}
CGeniusAI::AIObjective * CGeniusAI::getBestObjective()
{
	trueGameState.update(*this);
	
	fillObjectiveQueue(trueGameState);
	
	if(!objectiveQueue.empty())
		return max_element(objectiveQueue.begin(),objectiveQueue.end())->obj;
	return NULL;

}
void CGeniusAI::yourTurn()
{
	static boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);
	m_cb->waitTillRealize = true;
	static int seed = rand();
	srand(seed);
	if(m_cb->getDate()==1)
	{
		startFirstTurn();
		
		//m_cb->endTurn();

	}
	//////////////TODO: replace with updates. Also add suspected objects list./////////
	knownVisitableObjects.clear();
	int3 pos = m_cb->getMapSize();
	for(int x = 0;x<pos.x;x++)
		for(int y = 0;y<pos.y;y++)
			for(int z = 0;z<pos.z;z++)
				tileRevealed(int3(x,y,z));
	///////////////////////////////////////////////////////////////////////////////////

	reportResources();

	trueGameState = HypotheticalGameState(*this);

	AIObjective * objective;
	while((objective = getBestObjective())!=NULL)
		objective->fulfill(*this,trueGameState);

	seed = rand();
	m_cb->endTurn();
	
	m_cb->waitTillRealize = false;
}

void CGeniusAI::startFirstTurn()
{
	/*
	HypotheticalGameState hgs(*this);
	
	const CGTownInstance * town = m_cb->getTownInfo(0,0);
	const CGHeroInstance * heroInst = m_cb->getHeroInfo(0,0);
	
	TownObjective(AIObjective::recruitHero,&hgs.townModels.front(),0).fulfill(*this,hgs);
	
	m_cb->swapGarrisonHero(town);
	hgs.update(*this);
	for(int i = 0; i < hgs.townModels.front().creaturesToRecruit.size() ;i++)
	{
		if(hgs.townModels.front().creaturesToRecruit[i].first==0) continue;
		int ID = hgs.townModels.front().creaturesToRecruit[i].second.back();
		const CCreature *creature = &VLC->creh->creatures[ID];
		bool canAfford = true;
		for(int ii = 0;ii<creature->cost.size();ii++)
			if(creature->cost[ii]>hgs.resourceAmounts[ii])
				canAfford = false;							// can we afford at least one creature?
		if(!canAfford) continue;
		TownObjective(AIObjective::recruitCreatures,&hgs.townModels.front(),i).fulfill(*this,hgs);
	}
		hgs.update(*this);

	HypotheticalGameState::HeroModel *hero;
	for(int i = 0; i < hgs.heroModels.size();i++)
		if(hgs.heroModels[i].h->id==heroInst->id)
			HeroObjective(AIObjective::visit,town,hero=&hgs.heroModels[i]).fulfill(*this,hgs);
	
	hgs.update(*this);
	

	m_cb->swapGarrisonHero(town);

	//TODO: choose the strongest hero.
*/
}

void CGeniusAI::heroKilled(const CGHeroInstance * hero)
{

}

void CGeniusAI::heroCreated(const CGHeroInstance *hero)
{

}

void CGeniusAI::tileRevealed(int3 pos)
{
	std::vector < const CGObjectInstance * > objects = 	m_cb->getVisitableObjs(pos);
	for(std::vector < const CGObjectInstance * >::iterator o = objects.begin();o!=objects.end();o++)
		knownVisitableObjects.insert(*o);
	objects = m_cb->getFlaggableObjects(pos);
	for(std::vector < const CGObjectInstance * >::iterator o = objects.begin();o!=objects.end();o++)
		knownVisitableObjects.insert(*o);
}

void CGeniusAI::newObject(const CGObjectInstance * obj) //eg. ship built in shipyard
{
	knownVisitableObjects.insert(obj);
}

void CGeniusAI::objectRemoved(const CGObjectInstance *obj) //eg. collected resource, picked artifact, beaten hero
{
	std::set <AIObjectContainer>::iterator o = knownVisitableObjects.find(obj);
	if(o!=knownVisitableObjects.end())
		knownVisitableObjects.erase(o);
}

void CGeniusAI::tileHidden(int3 pos)
{
	
}

void CGeniusAI::heroMoved(const TryMoveHero &TMH)
{
	//DbgBox("** CGeniusAI::heroMoved **");


}

void CGeniusAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	callback(rand() % skills.size());
}

void GeniusAI::CGeniusAI::showGarrisonDialog( const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd )
{
	onEnd();
}

void GeniusAI::CGeniusAI::playerBlocked( int reason )
{
	if(reason == 0) //battle is coming...
	{
		m_state.setn(UPCOMING_BATTLE);
	}
}

void GeniusAI::CGeniusAI::battleResultsApplied()
{
	assert(m_state.get() == ENDING_BATTLE);
	m_state.setn(NO_BATTLE);
}

void CGeniusAI::showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel)
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
	DbgBox(message.c_str());
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
	DbgBox(message.c_str());
}
/**
 * called when stack is performing attack
 */
void CGeniusAI::battleAttack(BattleAttack *ba)
{
	DbgBox("\t\t\tCGeniusAI::battleAttack");
}
/**
 * called when stack receives damage (after battleAttack())
 */
void CGeniusAI::battleStacksAttacked(std::set<BattleStackAttacked> & bsa)
{
	DbgBox("\t\t\tCGeniusAI::battleStacksAttacked");
}
/**
 * called by engine when battle starts; side=0 - left, side=1 - right
 */

void CGeniusAI::battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side)
{
	assert(!m_battleLogic);
	assert(playerID > PLAYER_LIMIT  ||  m_state.get() == UPCOMING_BATTLE); //we have been informed that battle will start (or we are neutral AI)

	m_state.setn(ONGOING_BATTLE);
	m_battleLogic = new BattleAI::CBattleLogic(m_cb, army1, army2, tile, hero1, hero2, side);

	DbgBox("** CGeniusAI::battleStart **");
}
/**
 *
 */
void CGeniusAI::battleEnd(BattleResult *br)
{
	switch(br->winner)
	{
		case 0:	std::cout << "The winner is the attacker." << std::endl;break;
		case 1:	std::cout << "The winner is the defender." << std::endl;break;
		case 2:	std::cout << "It's a draw." << std::endl;break;
	};
	
	delete m_battleLogic;
	m_battleLogic = NULL;

	assert(m_state.get() == ONGOING_BATTLE);
	m_state.setn(ENDING_BATTLE);

	DbgBox("** CGeniusAI::battleEnd **");
}
/**
 * called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
 */
void CGeniusAI::battleNewRound(int round)
{
	std::string message("\tCGeniusAI::battleNewRound - ");
	message += boost::lexical_cast<std::string>(round);
	DbgBox(message.c_str());

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
	DbgBox(message.c_str());
}
/**
 *
 */
void CGeniusAI::battleSpellCast(SpellCast *sc)
{
	DbgBox("\t\t\tCGeniusAI::battleSpellCast");
}
/**
 * called when battlefield is prepared, prior the battle beginning
 */
void CGeniusAI::battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles)
{
	DbgBox("CGeniusAI::battlefieldPrepared");
}
/**
 *
 */
void CGeniusAI::battleStackMoved(int ID, int dest, bool startMoving, bool endMoving)
{
	DbgBox("\t\t\tCGeniusAI::battleStackMoved");
}
/**
 *
 */
void CGeniusAI::battleStackAttacking(int ID, int dest)
{
	DbgBox("\t\t\tCGeniusAI::battleStackAttacking");
}
/**
 *
 */
void CGeniusAI::battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting)
{
	DbgBox("\t\t\tCGeniusAI::battleStackIsAttacked");
}

/**
 * called when it's turn of that stack
 */
BattleAction CGeniusAI::activeStack(int stackID)
{
	std::string message("\t\t\tCGeniusAI::activeStack stackID(");

	message += boost::lexical_cast<std::string>(stackID);
	message += ")";
	DbgBox(message.c_str());

	return m_battleLogic->MakeDecision(stackID);
};

