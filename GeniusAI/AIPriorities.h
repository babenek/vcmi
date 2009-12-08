#ifndef AI_Priorities_H
#define AI_Priorities_H

#pragma warning(push, 0)
#include <string>
#pragma warning(pop)

#include "CGeniusAI.h"
#include "neuralNetwork.h"

// TODO: * no public variables
//       * definition of destructs

namespace geniusai {
class Network
{
public:
	Network();
	Network(std::vector<unsigned int> whichFeatures);// random network
	Network(std::istream& input);
	~Network();
	
	float feedForward(const std::vector<float>& stateFeatures);
	
	std::vector<unsigned int> whichFeatures;
	neuralNetwork net; // A network with whichFeatures.size() inputs and 1 output.
};

class Priorities
{
public:
	Priorities(const std::string& filename);	//read brain from file
	~Priorities();
	
  void fillFeatures(const CGeniusAI::HypotheticalGameState& AI);
  float getValue(const CGeniusAI::AIObjective& obj);
  float getCost(std::vector<int>& resourceCosts, const CGHeroInstance* moved,
      int distOutOfTheWay);

	int specialFeaturesStart;
	int numSpecialFeatures;
	std::vector<float>                    stateFeatures;
	std::vector< std::vector<Network> >   object_networks_;
	std::vector< std::map<int, Network> > building_networks_;
};

}
#endif // AI_Priorities_H