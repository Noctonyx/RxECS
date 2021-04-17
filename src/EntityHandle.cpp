#include "EntityHandle.h"
#include "World.h"

namespace ecs
{
    std::string EntityHandle::description() const
    {
        return world->description(id);
    }

    ComponentIterator EntityHandle::begin()
    {
        auto a = world->getEntityArchetypeDetails(id);
        return ComponentIterator{ world ,a.id, a.components.begin() };
    }

    ComponentIterator EntityHandle::end()
    {
        auto a = world->getEntityArchetypeDetails(id);
        return ComponentIterator{ world ,a.id, a.components.end() };
    }
}
