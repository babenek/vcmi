#include "StdAfx.h"
#include "BattleHelper.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// TODO: No clobbering the file with the WHOLE std...

using namespace geniusai::battleai;
using namespace std;
using std::string;
using std::fstream;
using boost::lexical_cast;

// TODO: More magical numbers...
CBattleHelper::CBattleHelper():
	InfiniteDistance(0xffff),
	BattlefieldWidth(15),
	BattlefieldHeight(11),
	voteForMaxDamage_(10),
	voteForMinDamage_(10),
	voteForMaxSpeed_(10),
	voteForDistance_(10),
	voteForDistanceFromShooters_(20),
	voteForHitPoints_(10)
{
	// loads votes
	fstream f;
	f.open("AI\\CBattleHelper.txt", std::ios::in);
	if (f) {
		//char c_line[512];
		string line;
		while (std::getline(f, line, '\n')) {
			//f.getline(c_line, sizeof(c_line), '\n');
			//std::string line(c_line);
			//std::getline(f, line);
			std::vector<string> parts;
			boost::algorithm::split(parts, line, boost::algorithm::is_any_of("="));
			if (parts.size() >= 2) {
				boost::algorithm::trim(parts[0]);
				boost::algorithm::trim(parts[1]);
				if (parts[0].compare("voteForDistance_") == 0) {
					try {
						voteForDistance_ = lexical_cast<si16>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} else if (parts[0].compare("voteForDistanceFromShooters_") == 0) {
					try {
						voteForDistanceFromShooters_ = lexical_cast<si16>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} else if (parts[0].compare("voteForHitPoints_") == 0) {
				  try {
						voteForHitPoints_ = lexical_cast<si16>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} else if (parts[0].compare("voteForMaxDamage_") == 0) {
					try {
						voteForMaxDamage_ = lexical_cast<si16>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} else if (parts[0].compare("voteForMaxSpeed_") == 0) {
					try {
						voteForMaxSpeed_ = lexical_cast<si16>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("voteForMinDamage_") == 0) {
					try {
						voteForMinDamage_ = lexical_cast<si16>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} // if (parts.size() >= 2) 
			}   // while (std::getline(f, line, '\n'))
		}     // if (f)
		f.close();
	}
}

CBattleHelper::~CBattleHelper()
{}

si16 CBattleHelper::GetBattleFieldPosition(si16 x, si16 y)
{
	return x + 17*(y - 1);
}

si16 CBattleHelper::DecodeXPosition(si16 battleFieldPosition)
{
	si16 pos = battleFieldPosition - (DecodeYPosition(battleFieldPosition) - 1)*17;
	assert(pos > 0 && pos < 16);
	return pos;
}

si16 CBattleHelper::DecodeYPosition(si16 battleFieldPosition)
{
	double y = (double)battleFieldPosition / 17.0;
	if (y - (si16)y > 0.0)
		return (si16)y + 1;
		
	assert((si16)y > 0 && (si16)y <= 11);
	return (si16)y;
}


si16 CBattleHelper::GetShortestDistance(si16 pointA, si16 pointB)
{
	/**
	 * TODO: function hasn't been checked!
	 */
	si16 x1 = DecodeXPosition(pointA);
	si16 y1 = DecodeYPosition(pointA);
	//
	si16 x2 = DecodeXPosition(pointB);
	//x2 += (x2 % 2)? 0 : 1;
	si16 y2 = DecodeYPosition(pointB);
	//
	double dx = x1 - x2;
	double dy = y1 - y2;

	return (si16)sqrt(dx * dx + dy * dy);
}

si16 CBattleHelper::GetDistanceWithObstacles(si16 pointA, si16 pointB)
{
	// TODO - implement this!
	return GetShortestDistance(pointA, pointB);
}
