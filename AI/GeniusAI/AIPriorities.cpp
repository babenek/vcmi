#include "StdAfx.h"
#include "Common.h"
#include "AIPriorities.h"

#include "CGeniusAI.h"
#include "../../hch/CObjectHandler.h"
#include "HypotheticalGameState.h"
#include "HeroObjective.h"
#include "AIObjectContainer.h"

using std::vector;
using std::string;
using std::cout;
using std::endl;

namespace geniusai {

Network::Network()
{}

// TODO: No magic numbers ala 0.601, 0.251, etc.
Network::Network(vector<ui16> whichFeatures)// random network
    : net(static_cast<si16>(whichFeatures.size()),
          static_cast<si16>(whichFeatures.size() * 0.601f + 2.0f),
          static_cast<si16>(whichFeatures.size() * 0.251f + 2.0f),
          1),
      whichFeatures(whichFeatures)
{}

Network::Network(std::istream& input)
{
	// vector<si16> whichFeatures;
	si16 feature;
	string line;
	std::getline(input,line);
	std::stringstream lineIn(line);
	while (lineIn >> feature)
		whichFeatures.push_back(feature);

	std::getline(input, line); //get R
	net = neuralNetwork(whichFeatures.size(),
                      static_cast<si16>(whichFeatures.size()*0.601 + 2),
                      static_cast<si16>(whichFeatures.size()*0.251 + 2),
                      1);
}

Network::~Network()
{}

float Network::feedForward(const vector<float>& stateFeatures)
{
  // TODO: Should comment/rewrite it...
	return (rand() % 1000)/800.0f;
	/*
	double * input = new double[whichFeatures.size()];
	for (si16 i = 0; i < whichFeatures.size(); i++)
		input[i] = state_features_[whichFeatures[i]];
	
	float ans = net.feedForwardPattern(input)[0];
	
	delete input;
	return ans;
	*/
}

// TODO: Magical numbers strike again!!
// Read brain from file
Priorities::Priorities(const std::string& filename) : numSpecialFeatures(8)
{
	std::ifstream infile(filename.c_str());

	// object_num [list of features]
	// brain data or "R" for random brain
	object_networks_.resize(255);
	building_networks_.resize(9);

	char type;
	si16 object_num;
	si16 town_num;
	si16 building_num;
	while (infile >> type) {
		switch (type) {
		case 'o': // Map object.
			infile >> object_num;
			object_networks_[object_num].push_back(Network(infile));
			break;
		case 't': // Town building.
			infile >> town_num >> building_num;
			building_networks_[town_num][building_num] = Network(infile);
			break;
    }
  }
}

Priorities::~Priorities()
{}

void Priorities::fillFeatures(const HypotheticalGameState& hgs)
{
	state_features_.clear();
	state_features_.resize(50); // Why 50?
	for (si16 i = 0; i < state_features_.size(); i++)
		state_features_[i] = 0;
	// Features 0-7 are resources.
	for (si16 i = 0; i < hgs.resourceAmounts.size(); i++)
		state_features_[i] = hgs.resourceAmounts[i];

	// TODO: features 8-15 are incomes.
	// Features 16-23 are special features (filled in by get functions before 
	// Artificial Neural Network).
	specialFeaturesStart = 16;
	
	state_features_[24] = static_cast<float>(hgs.AI->call_back_->getDate());
	state_features_[25] = 1; // TODO: Magical numbers MUST stop!!!
}

float Priorities::getCost(vector<si16>& resourceCosts,
                          const CGHeroInstance* moved, 
                          si16 distOutOfTheWay)
{
	if (resourceCosts.size() == 0)
	  return -1;
	// TODO: * replace with neural network, or at leastr
	//       * remove the 4.0, 2.0, 3000.0 with some understandable names
	float cost = resourceCosts[0] / 4.0f +
	             resourceCosts[1] / 2.0f +
	             resourceCosts[2] / 4.0f +
	             resourceCosts[3] / 2.0f +
	             resourceCosts[4] / 2.0f +
	             resourceCosts[5] / 2.0f +
	             resourceCosts[6] / 3000.0f;
	
	if (moved != NULL) // TODO: multiply by importance of hero
		cost += distOutOfTheWay / 10000.0; // TODO: Magical Number
	return cost;
}

float Priorities::getValue(const AIObjective& obj)
{
  // Resource.
	vector<si16> resourceAmounts(8, 0);
	si16 amount;
	
	// TODO: replace with value of visiting that object divided by days till 
	// completed
	if (obj.type == AIObjective::finishTurn)
		return 0.0001f;			//small nonzero
	float a;
	if (obj.type == AIObjective::attack)
		return 100;
	if (dynamic_cast<const HeroObjective*>(&obj)) {
		const HeroObjective* hobj =
		    dynamic_cast<const HeroObjective*>(&obj);
		// Unfortunately "intelligent" VC++ can't see private declarations...
		state_features_[16] = hobj->whoCanAchieve.front()->h->getTotalStrength();
		if (dynamic_cast<const CArmedInstance*>(hobj->object))
			state_features_[17] =
			    dynamic_cast<const CArmedInstance*>(hobj->object)->getArmyStrength();
		switch(hobj->object->ID) {
		case 5: //artifact //TODO: return value of each artifact
			return 0;
		
		case 79: // Resources on the ground.
			switch(hobj->object->subID) {
			case 6:
				amount = 800;
				break;
			case 0:
			case 2:
				amount = 9.5;	// Will be rounded, sad...
				break;
			default:
				amount = 4;
				break;
			}
			resourceAmounts[hobj->object->subID] = amount;
			return getCost(resourceAmounts, NULL, 0);
			break;

		case 55: // Mystical garden.
			resourceAmounts[6] = 500;
			a = getCost(resourceAmounts, NULL, 0);
			resourceAmounts[6] = 0;
			resourceAmounts[5] = 5;
			return (a + getCost(resourceAmounts, NULL, 0)) * 0.5;

		case 109:
			resourceAmounts[6] = 1000;
			return getCost(resourceAmounts, NULL, 0);

		case 12: // Campfire.
			resourceAmounts[6] = 500;
			for(si16 i = 0; i < 6; i++)
				resourceAmounts[i] = 1;
			return getCost(resourceAmounts, NULL, 0);

		case 112: // Windmill.
			for(si16 i = 1; i < 6; i++)//no wood
				resourceAmounts[i]=1;
			return getCost(resourceAmounts, NULL, 0);

		case 49://magic well
			// TODO: add features for hero's spell points
			break;
		// TODO: dynamic_casts are evil!

		case 34: // Hero.
			if (dynamic_cast<const CGHeroInstance*>(hobj->object)->getOwner() == 
			    obj.AI->call_back_->getMyColor()) {//friendly hero
				state_features_[17] = 
				  dynamic_cast<const CGHeroInstance*>(hobj->object)->getTotalStrength();
				return object_networks_[34][0].feedForward(state_features_);
			} else {
				state_features_[17] =
				  dynamic_cast<const CGHeroInstance*>(hobj->object)->getTotalStrength();
				return object_networks_[34][1].feedForward(state_features_);
			}
			break;

		case 98:
		  // Friendly Town.
			if (dynamic_cast<const CGTownInstance*>(hobj->object)->getOwner() == 
			    obj.AI->call_back_->getMyColor()) {//friendly town
				state_features_[17] =
				    dynamic_cast<const CGTownInstance*>(hobj->object)->getArmyStrength();
				return 0; // object_networks_[98][0].feedForward(state_features_);
			} else { // Enemy Town.
				state_features_[17] =
				    dynamic_cast<const CGTownInstance*>(hobj->object)->getArmyStrength();
				return 0; //object_networks_[98][1].feedForward(state_features_);
			}
			break;
		case 88:
			//TODO: average value of unknown level 1 spell, or value of known spell
		case 89:
			//TODO: average value of unknown level 2 spell, or value of known spell
		case 90:
			//TODO: average value of unknown level 3 spell, or value of known spell
			return 0;
		break;
		
		case 215://quest guard
			return 0;
		
		case 53:	//various mines
			return 0; //out of range crash
			//object_networks_[53][hobj->object->subID].feedForward(state_features_);
		case 113://TODO: replace with value of skill for the hero
			return 0;
		case 103:
		case 58: //TODO: replace with value of seeing x number of new tiles 
			return 0;
		default:
			if (object_networks_[hobj->object->ID].size() != 0)
				return object_networks_[hobj->object->ID][0].feedForward(state_features_);
			cout << "Don't know the value of ";
			switch (obj.type) {
			case AIObjective::visit:
				cout << "visiting " << hobj->object->ID;
				break;
			case AIObjective::attack:
				cout << "attacking " << hobj->object->ID;
				break;
			case AIObjective::finishTurn:
				obj.print();
				break;
			}
			cout << endl;
		}
	} else { // Town objective.
		if (obj.type == AIObjective::buildBuilding) {
			const TownObjective* tnObj = dynamic_cast<const TownObjective*>(&obj);
			if (building_networks_[tnObj->whichTown->t->subID].find(tnObj->which) !=
			    building_networks_[tnObj->whichTown->t->subID].end())
				return building_networks_[tnObj->whichTown->t->subID][tnObj->which].feedForward(state_features_);
			else {
				cout << "Don't know the value of ";
				obj.print();
				cout << endl;
			}
		}
	}
	return 0;
}
}