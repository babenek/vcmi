#ifndef AI_OBJECT_CONTAINER_H
#define AI_OBJECT_CONTAINER_H

#include "Common.h"

namespace geniusai {
struct AIObjectContainer {
  AIObjectContainer(const ::CGObjectInstance* obj_inst);
  bool operator<(const AIObjectContainer& rhs) const;
  
  const CGObjectInstance* object_instance_;
};
}

#endif // AI_OBJECT_CONTAINER_H