#pragma once

#include "Entity.h"
#include "World.h"

namespace ecs
{
    class World;
    //class World;

    struct EntityHandle
    {
        entity_t id;
        World* world;

        template <typename T>
        EntityHandle& set(const T& v);

        template <typename T>
        EntityHandle& add();

        template <typename T>
        EntityHandle& addDeferred();

        template <typename T>
        EntityHandle& removeDeferred();

        EntityHandle& destroyDeferred();

        template <typename T>
        EntityHandle& setDeferred(T&& value);

        template <typename T>
        bool has();

        template <typename T>
        T* getUpdate();

        template <typename T>
        const T* get(bool inherit = false);

        template <typename T>
        EntityHandle& remove();

        bool isAlive() const;
        EntityHandle& destroy();

        EntityHandle getHandle(entity_t o)
        {
            return EntityHandle{o, world};
        }

        bool operator==(const EntityHandle& o) const
        {
            return o.id == id && o.world == world;
        }

        World* getWorld() const
        {
            return world;
        }
    };
}
