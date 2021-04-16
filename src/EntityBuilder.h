#pragma once

#include "Entity.h"
#include "World.h"

namespace ecs
{
    class World;
    //class World;

    struct EntityBuilder
    {
        entity_t id;
        World* world;

        template<typename T>
        EntityBuilder & set(const T & v);

        template<typename T>
        EntityBuilder& add();

        template<typename T>
        EntityBuilder& addDeferred();

        template<typename T>
        EntityBuilder& removeDeferred();

        EntityBuilder& destroyDeferred();

        template <typename T>
        EntityBuilder& setDeferred( T&& value);

        template<typename T>
        bool has();

        template<typename T>
        T* getUpdate();

        template<typename T>
        const T* get(bool inherit = false);

        template<typename T>
        EntityBuilder& remove();

        bool isAlive() const;
        EntityBuilder& destroy();

        bool operator==(const EntityBuilder & o) const
        {
            return o.id == id && o.world == world;
        }
    };
}
