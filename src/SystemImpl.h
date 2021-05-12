#pragma once

#include <array>
#include "System.h"
#include "Stream.h"

namespace ecs
{
    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withQuery()
    {
        world->markSystemsDirty();

        std::set<component_id_t> with = {world->getComponentId<TArgs>()...};
        q = world->createQuery(with).id;
        world->getUpdate<System>(id)->query = q;
        for (auto value: with) {
            world->getUpdate<System>(id)->reads.insert(value);
        }

        qb = QueryBuilder{q, world};

        return *this;
    }

    template <class T>
    SystemBuilder & SystemBuilder::withStream()
    {
        world->markSystemsDirty();

        auto s = world->getUpdate<System>(id);

        s->stream = world->getComponentId<T>();
        s->reads.insert(world->getComponentId<T>());

        stream = id;
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::without()
    {
        world->markSystemsDirty();

        assert(q);
        qb.without<TArgs...>();

        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::with()
    {
        world->markSystemsDirty();

        assert(q);
        qb.with<TArgs...>();

        std::array<component_id_t, sizeof...(TArgs)> comps = { world->getComponentId<TArgs>()... };
        for (auto value : comps) {
            world->getUpdate<System>(id)->reads.insert(value);
        }

        return *this;
    }

    template <class T, class ... U>
    SystemBuilder & SystemBuilder::withRelation()
    {
        world->markSystemsDirty();

        assert(q);
        qb.withRelation<T, U ...>();

        world->getUpdate<System>(id)->reads.insert(world->getComponentId<T>());

        std::array<component_id_t, sizeof...(U)> comps = { world->getComponentId<U>()... };
        for (auto value : comps) {
            world->getUpdate<System>(id)->reads.insert(value);
        }
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withOptional()
    {
        world->markSystemsDirty();

        assert(q);
        qb.withOptional<TArgs ...>();

        std::array<component_id_t, sizeof...(TArgs)> comps = { world->getComponentId<TArgs>()... };
        for (auto value : comps) {
            world->getUpdate<System>(id)->reads.insert(value);
        }

        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withSingleton()
    {
        world->markSystemsDirty();

        assert(q);
        qb.withSingleton<TArgs ...>();
        return *this;
    }

    template <typename T>
    SystemBuilder & SystemBuilder::label()
    {
        world->markSystemsDirty();

        label(world->getComponentId<T>());
        return *this;
    }

    template <typename T>
    SystemBuilder & SystemBuilder::before()
    {
        world->markSystemsDirty();

        before(world->getComponentId<T>());
        return *this;
    }

    template <typename T>
    SystemBuilder & SystemBuilder::after()
    {
        world->markSystemsDirty();

        after(world->getComponentId<T>());
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withRead()
    {
        world->markSystemsDirty();

        auto s = world->getUpdate<System>(id);
        std::array<component_id_t, sizeof...(TArgs)> comps = {world->getComponentId<TArgs>() ...};
        for (auto c: comps) {
            s->reads.insert(c);
        }

        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withWrite()
    {
        world->markSystemsDirty();

        auto s = world->getUpdate<System>(id);
        std::array<component_id_t, sizeof...(TArgs)> comps = {world->getComponentId<TArgs>() ...};
        for (auto c: comps) {
            s->writes.insert(c);
        }

        return *this;
    }

    template <typename ... U, typename Func>
    SystemBuilder & SystemBuilder::each(Func && f)
    {
        assert(q);
        assert(!stream);

        auto s = world->getUpdate<System>(id);

        assert(s->groupId);

        auto mp = get_mutable_parameters(f);
        std::array<component_id_t, sizeof...(U)> comps = {world->getComponentId<U>() ...};

        uint32_t i = 0;
        for (auto mpx: mp) {
            if (i > 0) {
                if (mpx) {
                    s->writes.insert(comps[i-1]);
                }
                else {
                    s->reads.insert(comps[i-1]);
                }
            }
            i++;
        }

        s->queryProcessor = [=](QueryResult & res)
        {
            res.each<U...>(f);
        };

        return *this;
    }

    template <typename Func>
    SystemBuilder & SystemBuilder::execute(Func && f)
    {
        assert(!q);
        assert(!stream);

        auto s = world->getUpdate<System>(id);
        assert(s->groupId);

        s->executeProcessor = [=](ecs::World * w)
        {
            f(w);
        };

        return *this;
    }

    template <typename Func>
    SystemBuilder & SystemBuilder::executeIfNone(Func && f)
    {
        assert(q);
        assert(!stream);

        auto s = world->getUpdate<System>(id);
        assert(s->groupId);

        s->executeProcessor = [=](ecs::World* w)
        {
            f(w);
        };

        return *this;
    }

    template <typename U, typename Func>
    SystemBuilder & SystemBuilder::execute(Func && f)
    {
        assert(!q);
        assert(stream);

        auto s = world->getUpdate<System>(id);
        assert(s->groupId);

        s->reads.insert(world->getComponentId<U>());

        s->streamProcessor = [=](Stream * stream)
        {
            stream->each<U>(f);
        };

        return *this;
    }
}
