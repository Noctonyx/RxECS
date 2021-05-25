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
        /*
        auto sp = world->getUpdate<System>(id);

        sp->thread = true;
        */
        assert(q);
        qb.withJob();
        return *this;
    }
}
