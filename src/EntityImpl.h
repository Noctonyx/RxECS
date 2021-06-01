#pragma once

#include "EntityHandle.h"
#include "World.h"

namespace ecs
{
    template <typename T>
    EntityHandle & EntityHandle::set(const T & v)
    {
        world->set<T>(id, v);
        return *this;
    }

    template <typename T>
    EntityHandle & EntityHandle::add()
    {
        world->add<T>(id);
        return *this;
    }

    template <typename T>
    T * EntityHandle::addAndUpdate()
    {
        world->add<T>(id);
        return world->getUpdate<T>(id);
    }

    template <typename T>
    EntityHandle & EntityHandle::addDeferred()
    {
        world->addDeferred<T>(id);
        return *this;
    }

    template <typename T>
    EntityHandle & EntityHandle::removeDeferred()
    {
        world->removeDeferred<T>(id);
        return *this;
    }

    template <typename T>
    EntityHandle & EntityHandle::setDeferred(const T & value)
    {
        world->setDeferred<T>(id, value);
        return *this;
    }

    template <typename T>
    bool EntityHandle::has()
    {
        return world->has<T>(id);
    }

    template <typename T>
    T * EntityHandle::getUpdate()
    {
        return world->getUpdate<T>(id);
    }

    template <typename T>
    const T * EntityHandle::get(const bool inherit)
    {
        return world->get<T>(id, inherit);
    }

    template <typename U, typename T>
    const T * EntityHandle::getRelated()
    {
        return world->getRelated<U, T>(id);
    }

    template <typename T>
    EntityHandle EntityHandle::getRelatedEntity()
    {
        return world->getRelatedEntity<T>(id);
    }

    template <typename T>
    EntityHandle & EntityHandle::remove()
    {
        world->remove<T>(id);
        return *this;
    }

    inline EntityHandle & EntityHandle::addDynamic(component_id_t componentId)
    {
        world->add(id, componentId);
        return *this;
    }

    inline EntityHandle & EntityHandle::destroyDeferred()
    {
        world->destroyDeferred(id);
        return *this;
    }

    inline bool EntityHandle::hasDynamic(component_id_t componentId)
    {
        return world->has(id, componentId);
    }

    inline EntityHandle & EntityHandle::removeDynamic(component_id_t componentId)
    {
        world->remove(id, componentId);
        return *this;
    }

    inline bool EntityHandle::isAlive() const
    {
        return world->isAlive(id);
    }

    inline EntityHandle & EntityHandle::destroy()
    {
        world->destroy(id);
        return *this;
    }

    inline EntityHandle EntityHandle::instantiate(const char * name)
    {
        assert(isAlive());
        assert(has<Prefab>());

        auto e = world->instantiate(id);
        if (name) {
            e.set<Name>({name});
        }
        return e;
    }
}
