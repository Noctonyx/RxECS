////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde (shane@noctonyx.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "EntityHandle.h"
#include "World.h"

namespace ecs
{
    template<typename T>
    EntityHandle & EntityHandle::set(const T & v)
    {
        world->set<T>(id, v);
        return *this;
    }

    template<typename T>
    EntityHandle & EntityHandle::add()
    {
        world->add<T>(id);
        return *this;
    }
#if 0
    template<typename T>
    T * EntityHandle::addAndUpdate()
    {
        world->add<T>(id);
        return world->getUpdate<T>(id);
    }
#endif
    template<typename T>
    EntityHandle & EntityHandle::addDeferred()
    {
        world->addDeferred<T>(id);
        return *this;
    }

    template<typename T>
    EntityHandle & EntityHandle::removeDeferred()
    {
        world->removeDeferred<T>(id);
        return *this;
    }

    template<typename T>
    EntityHandle & EntityHandle::setDeferred(const T & value)
    {
        world->setDeferred<T>(id, value);
        return *this;
    }

    template<typename T>
    bool EntityHandle::has()
    {
        return world->has<T>(id);
    }

#if 0
    template <typename T>
    T * EntityHandle::getUpdate()
    {
        return world->getUpdate<T>(id);
    }
#endif

    template<class T>
    void EntityHandle::update(std::function<void(T * )> && f)
    {
        world->update(id, std::move(f));
    }

    template<typename T>
    const T * EntityHandle::get(const bool inherit)
    {
        return world->get<T>(id, inherit);
    }

    template<typename U, typename T>
    const T * EntityHandle::getRelated()
    {
        return world->getRelated<U, T>(id);
    }

    template<typename T>
    EntityHandle EntityHandle::getRelatedEntity()
    {
        return world->getRelatedEntity<T>(id);
    }

    template<typename T>
    EntityHandle & EntityHandle::remove()
    {
        world->remove<T>(id);
        return *this;
    }

    template<class T>
    void EntityHandle::addAndUpdate(std::function<void(T * )> && f)
    {
        add<T>();
        world->update<T>(id, std::move(f));
    }
}
