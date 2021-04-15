#include "System.h"
#include "Query.h"
#include "QueryImpl.h"
#include "Stream.h"

namespace ecs
{
    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withQuery()
    {
        std::set<component_id_t> with = {world->getComponentId<TArgs>()...};
        q = world->createQuery(with).id;
        world->getUpdate<System>(id)->query = q;

        qb = QueryBuilder{q, world};

        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::without()
    {
        assert(q);
        qb.without<TArgs...>();

        return *this;
    }

    template <class T, class ... U>
    SystemBuilder & SystemBuilder::withRelation()
    {
        assert(q);
        qb.withRelation<T, U ...>();
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withOptional()
    {
        assert(q);
        qb.withOptional<TArgs ...>();
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withSingleton()
    {
        assert(q);
        qb.withOptional<TArgs ...>();
        return *this;
    }

    template <typename T>
    SystemBuilder & SystemBuilder::label()
    {
        label(world->getComponentId<T>());
        return *this;
    }

    template <typename T>
    SystemBuilder & SystemBuilder::before()
    {
        before(world->getComponentId<T>());
        return *this;
    }

    template <typename T>
    SystemBuilder & SystemBuilder::after()
    {
        after(world->getComponentId<T>());
        return *this;
    }

    template <typename ... U, typename Func>
    SystemBuilder & SystemBuilder::each(Func && f)
    {
        assert(q);
        assert(!stream);

        auto s = world->getUpdate<System>(id);

        auto mp = get_mutable_parameters(f);
        (void) mp;

        s->queryProcessor = [&](QueryResult & res)
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

        s->executeProcessor = [&]()
        {
            f(world);
        };

        return *this;
    }

    template <typename U, typename Func>
    SystemBuilder & SystemBuilder::execute(Func && f)
    {
        assert(!q);
        assert(stream);

        auto s = world->getUpdate<System>(id);

        s->streamProcessor = [&](Stream * stream)
        {
            stream->each<U>(f);
        };

        return *this;
    }
}
