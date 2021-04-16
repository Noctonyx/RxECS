#pragma once

#include "EntityBuilder.h"

namespace ecs
{
    template <typename T>
    EntityBuilder& EntityBuilder::set(const T& v)
    {
        world->set<T>(id, v);
        return *this;
    }

    template <typename T>
    EntityBuilder & EntityBuilder::add()
    {
        world->add<T>(id);
        return *this;
    }

    template <typename T>
    EntityBuilder & EntityBuilder::addDeferred()
    {
        world->addDeferred<T>(id);
        return *this;
    }

    template <typename T>
    EntityBuilder & EntityBuilder::removeDeferred()
    {
        world->removeDeferred<T>(id);
        return *this;
    }

    template <typename T>
    EntityBuilder & EntityBuilder::setDeferred(T && value)
    {
        world->setDeferred<T>(id, std::move(value));
        return *this;
    }

    template <typename T>
    bool EntityBuilder::has()
    {
        return world->has<T>(id);
    }

    template <typename T>
    T * EntityBuilder::getUpdate()
    {
        return world->getUpdate<T>(id);
    }

    template <typename T>
    const T * EntityBuilder::get(const bool inherit)
    {
        return world->get<T>(id, inherit);
    }

    template <typename T>
    EntityBuilder & EntityBuilder::remove()
    {
        world->remove<T>(id);
        return *this;
    }

    inline EntityBuilder & EntityBuilder::destroyDeferred()
    {
        world->destroyDeferred(id);
        return *this;
    }

    inline bool EntityBuilder::isAlive() const
    {
        return world->isAlive(id);
    }

    inline EntityBuilder & EntityBuilder::destroy()
    {
        world->destroy(id);
        return *this;
    }
}
