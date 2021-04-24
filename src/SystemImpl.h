#include "System.h"
#include "Query.h"
#include "QueryImpl.h"
#include "Stream.h"

namespace ecs
{
    template <class ... TArgs>
    SystemBuilder& SystemBuilder::withQuery()
    {
        world->markSystemsDirty();

        std::set<component_id_t> with = {world->getComponentId<TArgs>()...};
        q = world->createQuery(with).id;
        world->getUpdate<System>(id)->query = q;

        qb = QueryBuilder{q, world};

        return *this;
    }

    template <class T>
    SystemBuilder& SystemBuilder::withStream()
    {
        world->markSystemsDirty();

        auto s = world->getUpdate<System>(id);

        s->stream = world->getComponentId<T>();
        stream = id;
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder& SystemBuilder::without()
    {
        world->markSystemsDirty();

        assert(q);
        qb.without<TArgs...>();

        return *this;
    }

    template <class T, class ... U>
    SystemBuilder& SystemBuilder::withRelation()
    {
        world->markSystemsDirty();

        assert(q);
        qb.withRelation<T, U ...>();
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder& SystemBuilder::withOptional()
    {
        world->markSystemsDirty();

        assert(q);
        qb.withOptional<TArgs ...>();
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder& SystemBuilder::withSingleton()
    {
        world->markSystemsDirty();

        assert(q);
        qb.withOptional<TArgs ...>();
        return *this;
    }

    template <typename T>
    SystemBuilder& SystemBuilder::label()
    {
        world->markSystemsDirty();

        label(world->getComponentId<T>());
        return *this;
    }

    template <typename T>
    SystemBuilder& SystemBuilder::before()
    {
        world->markSystemsDirty();

        before(world->getComponentId<T>());
        return *this;
    }

    template <typename T>
    SystemBuilder& SystemBuilder::after()
    {
        world->markSystemsDirty();

        after(world->getComponentId<T>());
        return *this;
    }

    template <typename ... U, typename Func>
    SystemBuilder& SystemBuilder::each(Func&& f)
    {
        assert(q);
        assert(!stream);

        auto s = world->getUpdate<System>(id);

        assert(s->groupId);

        auto mp = get_mutable_parameters(f);
        (void)mp;

        s->queryProcessor = [=](QueryResult& res)
        {
            res.each<U...>(f);
        };

        return *this;
    }

    template <typename Func>
    SystemBuilder& SystemBuilder::execute(Func&& f)
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

    template <typename U, typename Func>
    SystemBuilder& SystemBuilder::execute(Func&& f)
    {
        assert(!q);
        assert(stream);

        auto s = world->getUpdate<System>(id);
        assert(s->groupId);

        s->streamProcessor = [=](Stream* stream)
        {
            stream->each<U>(f);
        };

        return *this;
    }
}
