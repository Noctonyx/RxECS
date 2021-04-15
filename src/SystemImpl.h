
#include "System.h"
#include "Query.h"

namespace ecs {
    template <class ... TArgs>
    SystemBuilder & SystemBuilder::withQuery()
    {
        std::set<component_id_t> with = { world->getComponentId<TArgs>()... };
        q = world->createQuery(with).id;
        world->getUpdate<System>(id)->query = q;
        return *this;
    }

    template <class ... TArgs>
    SystemBuilder& SystemBuilder::without()
    {
        assert(q);
        std::vector<component_id_t> without = { world->getComponentId<TArgs>()... };
        auto qp = world->getUpdate<Query>(q);
        for (auto w : without) {
            qp->without.insert(w);
        }

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
    SystemBuilder & SystemBuilder::each(Func&& f)
    {
        assert(q);
        auto s = world->getUpdate<System>(id);
        
        auto mp = get_mutable_parameters(f);
        (void)mp;

        s->queryProcessor = [&](QueryResult& res)
        {
            res.each<U...>(f);
        };

        return *this;
    }

    template <typename Func>
    SystemBuilder& SystemBuilder::execute(Func&& f)
    {
        auto s = world->getUpdate<System>(id);

        s->executeProcessor = [&]()
        {
            f(world);
        };

        return *this;
    }
}