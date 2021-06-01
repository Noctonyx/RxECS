#pragma once
#include <string>

#include "ComponentIterator.h"
#include "Entity.h"

namespace ecs
{
    class World;

    struct EntityHandle
    {
        entity_t id;
        World * world;

        template <typename T>
        EntityHandle & set(const T & v);

        template <typename T>
        EntityHandle & add();
        EntityHandle & addDynamic(component_id_t id);

        template <typename T>
        T * addAndUpdate();

        template <typename T>
        EntityHandle & addDeferred();

        template <typename T>
        EntityHandle & removeDeferred();

        EntityHandle & destroyDeferred();

        template <typename T>
        EntityHandle & setDeferred(const T & value);

        template <typename T>
        bool has();
        bool hasDynamic(component_id_t id);

        template <typename T>
        T * getUpdate();

        template <typename T>
        const T * get(bool inherit = false);

        template <typename U, typename T>
        const T * getRelated();

        template <typename T>
        EntityHandle getRelatedEntity();

        template <typename T>
        EntityHandle & remove();
        EntityHandle & removeDynamic(component_id_t id);

        bool isAlive() const;
        EntityHandle & destroy();

        EntityHandle instantiate(const char * name = nullptr);

        EntityHandle getHandle(entity_t o)
        {
            return EntityHandle{o, world};
        }

        bool operator==(const EntityHandle & o) const
        {
            return o.id == id && o.world == world;
        }

        World * getWorld() const
        {
            return world;
        }

        operator bool() const;
        operator entity_t() const;

        std::string description() const;

        ComponentIterator begin();
        ComponentIterator end();
    };
}
