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

	si16 GetBattleFieldPosition(si16 x, si16 y);
	si16 DecodeXPosition(si16 battleFieldPosition);
	si16 DecodeYPosition(si16 battleFieldPosition);

	si16 GetShortestDistance(si16 pointA, si16 pointB);
	si16 GetDistanceWithObstacles(si16 pointA, si16 pointB);
	
	// TODO: Definitions always into the .cpp file.
	si16 GetVoteForMaxDamage() const { return voteForMaxDamage_; }
	si16 GetVoteForMinDamage() const { return voteForMinDamage_; }
	si16 GetVoteForMaxSpeed() const { return voteForMaxSpeed_; }
	si16 GetVoteForDistance() const { return voteForDistance_; }
	si16 GetVoteForDistanceFromShooters() const { return voteForDistanceFromShooters_; }
	si16 GetVoteForHitPoints() const { return voteForHitPoints_; }

	const si16 InfiniteDistance;
	const si16 BattlefieldWidth;
	const si16 BattlefieldHeight;
private:
	si16 voteForMaxDamage_;
	si16 voteForMinDamage_;
	si16 voteForMaxSpeed_;
	si16 voteForDistance_;
	si16 voteForDistanceFromShooters_;
	si16 voteForHitPoints_;

	CBattleHelper(const CBattleHelper &);
	CBattleHelper& operator=(const CBattleHelper &);
};
}}

#endif // BATTLE_HELPER_H