#include "BattleHelper.h"

#pragma warning(push, 0)
#include <vector>
#include <string>
#include <fstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#pragma warning(pop)

// TODO: No clobbering the file with the WHOLE std...

using namespace geniusai::battleai;
using namespace std;

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
	std::fstream f;
	f.open("AI\\CBattleHelper.txt", std::ios::in);
	if (f) {
		//char c_line[512];
		std::string line;
		while (std::getline(f, line, '\n')) {
			//f.getline(c_line, sizeof(c_line), '\n');
			//std::string line(c_line);
			//std::getline(f, line);
			std::vector<std::string> parts;
			boost::algorithm::split(parts, line, boost::algorithm::is_any_of("="));
			if (parts.size() >= 2) {
				boost::algorithm::trim(parts[0]);
				boost::algorithm::trim(parts[1]);
				if (parts[0].compare("voteForDistance_") == 0) {
					try {
						voteForDistance_ = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} else if (parts[0].compare("voteForDistanceFromShooters_") == 0) {
					try {
						voteForDistanceFromShooters_ = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} else if (parts[0].compare("voteForHitPoints_") == 0) {
				  try {
						voteForHitPoints_ = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} else if (parts[0].compare("voteForMaxDamage_") == 0) {
					try {
						voteForMaxDamage_ = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				} else if (parts[0].compare("voteForMaxSpeed_") == 0) {
					try {
						voteForMaxSpeed_ = boost::lexical_cast<int>(parts[1]);
					}
					catch (boost::bad_lexical_cast &)
					{}
				}
				else if (parts[0].compare("voteForMinDamage_") == 0) {
					try {
						voteForMinDamage_ = boost::lexical_cast<int>(parts[1]);
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

int CBattleHelper::GetBattleFieldPosition(int x, int y)
{
	return x + 17*(y - 1);
}

int CBattleHelper::DecodeXPosition(int battleFieldPosition)
{
	int pos = battleFieldPosition - (DecodeYPosition(battleFieldPosition) - 1)*17;
	assert(pos > 0 && pos < 16);
	return pos;
}

int CBattleHelper::DecodeYPosition(int battleFieldPosition)
{
	double y = (double)battleFieldPosition / 17.0;
	if (y - (int)y > 0.0)
		return (int)y + 1;
		
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
