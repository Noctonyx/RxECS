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

#include "SystemBuilder.h"
#include "System.h"
#include "EntityImpl.h"
#include "QueryImpl.h"

namespace ecs
{
    SystemBuilder & SystemBuilder::withInheritance(bool inherit)
    {
        assert(q);
        qb.withInheritance(inherit);

        return *this;
    }

    SystemBuilder & SystemBuilder::label(const entity_t label)
    {
        auto sp = world->getUpdate<System>(id);

        sp->labels.insert(label);
        //sp->dirtyOrder = true;

        return *this;
    }

    SystemBuilder & SystemBuilder::before(const entity_t label)
    {
        auto sp = world->getUpdate<System>(id);

        sp->befores.insert(label);
        //sp->dirtyOrder = true;

        return *this;
    }

    SystemBuilder & SystemBuilder::after(const entity_t label)
    {
        auto sp = world->getUpdate<System>(id);

        sp->afters.insert(label);
        //sp->dirtyOrder = true;

        return *this;
    }

    SystemBuilder & SystemBuilder::withSet(entity_t setId)
    {
        world->set<SetForSystem>(id, {{setId}});
        //        const auto sp = world->getUpdate<System>(id);
        //sp->dirtyOrder = true;

        return *this;
    }
#if 0
    SystemBuilder & SystemBuilder::removeSet()
    {
        world->remove<SetForSystem>(id);
        //const auto sp = world->getUpdate<System>(id);
        //sp->dirtyOrder = true;

        return *this;
    }
#endif
    SystemBuilder & SystemBuilder::withInterval(float v)
    {
        world->markSystemsDirty();

        auto sp = world->getUpdate<System>(id);
        sp->interval = v;
        return *this;
    }

    SystemBuilder & SystemBuilder::inGroup(entity_t group)
    {
        world->markSystemsDirty();

        auto sp = world->getUpdate<System>(id);
        assert(world->has<SystemGroup>(group));

        sp->groupId = group;
        return *this;
    }

    SystemBuilder & SystemBuilder::inGroup(const char * name)
    {
        world->markSystemsDirty();

        auto e = world->lookup(name);
        assert(e.isAlive());
        assert(e.has<SystemGroup>());
        return inGroup(e.id);
    }

    SystemBuilder & SystemBuilder::withJob()
    {
        world->markSystemsDirty();

        if(q) {
            qb.withJob();
        } else {
            auto sp = world->getUpdate<System>(id);
            sp->thread = true;
        }
        return *this;
    }

    SystemBuilder & SystemBuilder::withUpdates()
    {
        world->markSystemsDirty();

        auto sp = world->getUpdate<System>(id);
        sp->updatesOnly = true;

        return *this;
    }
}
