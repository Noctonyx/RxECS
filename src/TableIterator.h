#pragma once
#include "EntityHandle.h"

namespace ecs
{
    struct TableIterator
    {
        std::vector<entity_t>::iterator it;
        World* w;

        bool operator!=(TableIterator& other) const
        {
            return other.w != w || it != other.it;
        }

        EntityHandle operator*() const
        {
            return EntityHandle{ *it, w };
        }

        TableIterator& operator++()
        {
            ++it;
            return *this;
        }
    };
}
