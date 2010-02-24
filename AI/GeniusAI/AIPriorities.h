#ifndef AI_PRIORITIES_H
#define AI_PRIORITIES_H

#include "neuralNetwork.h"

#include <vector>
#include <sstream>
#include <string>
#include <map>

// TODO: * no public variables
//       * definition of dctor

namespace geniusai {
class HypotheticalGameState;
class AIObjective;

class Network {
public:
	Network();
	Network(std::vector<ui16> whichFeatures);// random network
	Network(std::istream& input);
	~Network();
	
	float feedForward(const std::vector<float>& stateFeatures);
	
	std::vector<ui16> whichFeatures;
	neuralNetwork net; // A network with whichFeatures.size() inputs and 1 output.
};

class Priorities {
public:
	Priorities(const std::string& filename);	// Read brain from file.
	~Priorities();
	
  void fillFeatures(const HypotheticalGameState& AI);
  float getValue(const AIObjective& obj);
  float getCost(std::vector<si16>& resourceCosts, const CGHeroInstance* moved,
                si16 distOutOfTheWay);

	si16 specialFeaturesStart;
	si16 numSpecialFeatures;
	std::vector<float> state_features_;
	std::vector< std::vector<Network> > object_networks_;
	std::vector< std::map<si16, Network> > building_networks_;
};

}
#endif // AI_PRIORITIES_H