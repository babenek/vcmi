#ifndef HERO_OBJECTIVE_H
#define HERO_OBJECTIVE_H

#include "AIObjective.h"
#include "HypotheticalGameState.h"
#include "../../lib/VCMI_Lib.h" // VLC->*

namespace geniusai {
class HeroObjective: public AIObjective {
public:
  HypotheticalGameState hgs;
  int3 pos;
  const CGObjectInstance* object;
  mutable std::vector<HypotheticalGameState::HeroModel*> whoCanAchieve;

  //HeroObjective() {}
  //HeroObjective(Type t) : object(NULL){type = t;}
  HeroObjective(const HypotheticalGameState& hgs,
                Type t,
                const CGObjectInstance* object,
                HypotheticalGameState::HeroModel* h,
                CGeniusAI* AI);
  // TODO: Unclear meaning of < operator - hack for STL?
  bool operator< (const HeroObjective& other) const;
  void fulfill(CGeniusAI&, HypotheticalGameState& hgs);
  // TODO: No definitions in header files.
  HypotheticalGameState pretend(const HypotheticalGameState& hgs) {return hgs;};
  float getValue() const;
  void print() const;
private:
  mutable float _value;
  mutable float _cost;
};
}
#endif