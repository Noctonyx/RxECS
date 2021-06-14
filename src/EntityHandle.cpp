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

#include "EntityHandle.h"
#include "World.h"
#include "Filter.h"
#include "EntityImpl.h"

namespace ecs
{
    EntityHandle::operator bool() const
    {
        return world->isAlive(id);
    }

    EntityHandle::operator unsigned long long() const
    {
        return id;
    }

    std::string EntityHandle::description() const
    {
        return world->description(id);
    }

    ComponentIterator EntityHandle::begin()
    {
        auto & a = world->getEntityArchetypeDetails(id);
        return ComponentIterator{ world ,a.id, a.components.begin() };
    }

    ComponentIterator EntityHandle::end()
    {
        auto & a = world->getEntityArchetypeDetails(id);
        return ComponentIterator{ world ,a.id, a.components.end() };
    }

    Filter EntityHandle::getChildren(const std::vector<component_id_t> & with, const std::vector<component_id_t> & without)
    {
        assert(has<ecs::Component>());

        auto w = with;
        w.push_back(id);
        return world->createFilter(w, without);
    }

    EntityHandle & EntityHandle::addParent(component_id_t componentId)
    {
        world->add(id, componentId);
        return *this;
    }

    EntityHandle & EntityHandle::destroyDeferred()
    {
        world->destroyDeferred(id);
        return *this;
    }

    bool EntityHandle::hasParent(component_id_t parentId) const
    {
        return world->has(id, parentId);
    }

    EntityHandle & EntityHandle::removeParent(component_id_t parentId)
    {
        world->remove(id, parentId);
        return *this;
    }

    bool EntityHandle::isAlive() const
    {
        return world->isAlive(id);
    }

    EntityHandle & EntityHandle::destroy()
    {
        world->destroy(id);
        return *this;
    }

    EntityHandle EntityHandle::instantiate(const char * name)
    {
        assert(isAlive());
        assert(has<Prefab>());

        auto e = world->instantiate(id);
        if (name) {
            e.set<Name>({name});
        }
        return e;
    }

    EntityHandle & EntityHandle::setAsParent()
    {
        world->setAsParent(id);
        return *this;
    }

    EntityHandle & EntityHandle::removeAsParent()
    {
        world->removeAsParent(id);
        return *this;
    }
}
