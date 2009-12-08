#ifndef BATTLE_HELPER_H
#define BATTLE_HELPER_H

#include "Common.h"

namespace geniusai {
namespace battleai {
class CBattleHelper
{
public:
	CBattleHelper();
	~CBattleHelper();

	int GetBattleFieldPosition(int x, int y);
	int DecodeXPosition(int battleFieldPosition);
	int DecodeYPosition(int battleFieldPosition);

	int GetShortestDistance(int pointA, int pointB);
	int GetDistanceWithObstacles(int pointA, int pointB);
	
	// TODO: Definitions always into the .cpp file.
	int GetVoteForMaxDamage() const { return voteForMaxDamage_; }
	int GetVoteForMinDamage() const { return voteForMinDamage_; }
	int GetVoteForMaxSpeed() const { return voteForMaxSpeed_; }
	int GetVoteForDistance() const { return voteForDistance_; }
	int GetVoteForDistanceFromShooters() const { return voteForDistanceFromShooters_; }
	int GetVoteForHitPoints() const { return voteForHitPoints_; }

	const int InfiniteDistance;
	const int BattlefieldWidth;
	const int BattlefieldHeight;
private:
	int voteForMaxDamage_;
	int voteForMinDamage_;
	int voteForMaxSpeed_;
	int voteForDistance_;
	int voteForDistanceFromShooters_;
	int voteForHitPoints_;

	CBattleHelper(const CBattleHelper &);
	CBattleHelper& operator=(const CBattleHelper &);
};
}}

#endif // BATTLE_HELPER_H