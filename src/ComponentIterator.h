#pragma once
#include <set>

#include "Entity.h"

namespace ecs
{
    class World;
    struct Table;

    struct ComponentIterator
    {
        World* world;

        uint32_t archetypeId;
        std::set<component_id_t>::iterator it;

        bool operator!=(ComponentIterator& other) const
        {
            return other.world != world || it != other.it || archetypeId != other.archetypeId;
        }

        component_id_t operator*() const
        {
            return *it;
        }

        ComponentIterator& operator++()
        {
            ++it;
            return *this;
        }
    };    
}
