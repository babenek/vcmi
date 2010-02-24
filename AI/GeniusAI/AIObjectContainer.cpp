#include "StdAfx.h"
#include "AIObjectContainer.h"
#include "../../hch/CObjectHandler.h"

namespace geniusai {
AIObjectContainer::AIObjectContainer(const ::CGObjectInstance* obj_inst) : 
    object_instance_(obj_inst)
{}

bool AIObjectContainer::operator<
    (const AIObjectContainer& rhs) const
{
  if (object_instance_->pos != rhs.object_instance_->pos)
    return object_instance_->pos < rhs.object_instance_->pos;
  else
    return object_instance_->id < rhs.object_instance_->id;
}
}