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
        world->update<System>(id, [=](System * sp){
            sp->labels.insert(label);
        });

        return *this;
    }

    SystemBuilder & SystemBuilder::before(const entity_t label)
    {
        world->update<System>(id, [=](System * sp){
            sp->befores.insert(label);
        });

        return *this;
    }

    SystemBuilder & SystemBuilder::after(const entity_t label)
    {
        world->update<System>(id, [=](System * sp){
            sp->afters.insert(label);
        });

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
        world->update<System>(id, [=](System * sp){
            sp->interval = v;
        });

        return *this;
    }

    SystemBuilder & SystemBuilder::inGroup(entity_t group)
    {
        assert(world->has<SystemGroup>(group));
        world->markSystemsDirty();

        world->update<System>(id, [=](System * sp){
            sp->groupId = group;
        });

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
            world->update<System>(id, [=](System * sp){
                sp->thread = true;
            });
        }
        return *this;
    }

    SystemBuilder & SystemBuilder::withUpdates()
    {
        world->markSystemsDirty();

        world->update<System>(id, [=](System * sp){
            sp->updatesOnly = true;
        });

        return *this;
    }

    SystemBuilder & SystemBuilder::withEntityQueue(entity_t queueid)
    {
        assert(type == SystemType::None);
        type = SystemType::Queue;

        eq = EntityQueueHandle{queueid, world};
        assert(world->has<HasEntityQueue>(queueid));

        world->update<System>(id, [&](System * sp){
            sp->entityQueue = queueid;
        });

        return *this;
    }

    SystemBuilder & SystemBuilder::eachEntity(std::function<bool(EntityHandle)> && f)
    {
        assert(type == SystemType::Queue);

        world->update<System>(id, [&](System * sp){
           sp->queueProcessor = std::move(f);
        });
        return *this;
    }
}
