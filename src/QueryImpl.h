#pragma once

#include <vector>
#include <type_traits>
#include "World.h"
#include "Query.h"
#include "QueryBuilder.h"

namespace ecs
{
    template <class ... TArgs>
    QueryBuilder & QueryBuilder::without()
    {
        std::vector<component_id_t> without = {world->getComponentId<TArgs>()...};
        auto qp = world->getUpdate<Query>(id);
        for (auto w: without) {
            qp->without.insert(w);
        }
        qp->recalculateQuery(world);

        return *this;
    }

    template <class T, class ... U>
    QueryBuilder & QueryBuilder::withRelation()
    {
        static_assert(std::is_base_of_v<Relation, T>);

        component_id_t relative = world->getComponentId<T>();
        std::set<component_id_t> targets = {world->getComponentId<U>() ...};

        auto qp = world->getUpdate<Query>(id);

        qp->relations.insert({relative, targets});
        qp->with.insert(relative);

        return *this;
    }

    template <class ... TArgs>
    QueryBuilder & QueryBuilder::withOptional()
    {
        std::vector<component_id_t> optional = {world->getComponentId<TArgs>()...};
        auto qp = world->getUpdate<Query>(id);
        for (auto w: optional) {
            qp->optional.insert(w);
        }

        return *this;
    }

    template <class ... TArgs>
    QueryBuilder & QueryBuilder::withSingleton()
    {
        std::vector<component_id_t> singleton = {world->getComponentId<TArgs>()...};
        auto qp = world->getUpdate<Query>(id);
        for (auto w: singleton) {
            qp->singleton.insert(w);
        }

        return *this;
    }

    inline QueryBuilder & QueryBuilder::withPrefabs()
    {       
        auto qp = world->getUpdate<Query>(id);
        qp->without.erase(world->getComponentId<Prefab>());
        qp->recalculateQuery(world);
        return *this;
    }

    inline QueryBuilder & QueryBuilder::withInheritance(bool inherit)
    {
        auto qp = world->getUpdate<Query>(id);
        qp->inheritamce = inherit;

        return *this;
    }
}
