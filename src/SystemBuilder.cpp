#include "SystemBuilder.h"
#include "System.h"
#include "EntityImpl.h"

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

    SystemBuilder & SystemBuilder::removeSet()
    {
        world->remove<SetForSystem>(id);
        //const auto sp = world->getUpdate<System>(id);
        //sp->dirtyOrder = true;

        return *this;
    }
}
