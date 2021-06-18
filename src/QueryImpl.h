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
        world->update<Query>(id, [&](Query * qp){
            for (auto w: without) {
                qp->without.insert(w);
            }
            qp->recalculateQuery(world);
        });

        return *this;
    }
#if 0
    template <class ... TArgs>
    QueryBuilder & QueryBuilder::with()
    {
        //std::vector<component_id_t> with = {world->getComponentId<TArgs>()...};
        auto with = world->makeComponentList<TArgs...>();
        auto qp = world->getUpdate<Query>(id);
        for (auto w: with) {
            qp->with.insert(w);
        }
        qp->recalculateQuery(world);

        return *this;
    }
#endif
    template <class T, class ... U>
    QueryBuilder & QueryBuilder::withRelation()
    {
        static_assert(std::is_base_of_v<Relation, T>);

        component_id_t relative = world->getComponentId<T>();
        std::set<component_id_t> targets = {world->getComponentId<U>() ...};

        world->update<Query>(id, [=](Query * qp){
            qp->relations.insert({relative, targets});
        });

        return *this;
    }

    template <class ... TArgs>
    QueryBuilder & QueryBuilder::withSingleton()
    {
        std::vector<component_id_t> singleton = {world->getComponentId<TArgs>()...};
        world->update<Query>(id, [=](Query * qp){
            for (auto w: singleton) {
                qp->singleton.insert(w);
            }
        });

        return *this;
    }

    inline QueryBuilder & QueryBuilder::withParent(component_id_t parentId)
    {
        assert(world->has<Component>(parentId));
        //auto qp = world->getUpdate<Query>(id);
        world->update<Query>(id, [=](Query * q){
            q->with.insert(parentId);
            q->recalculateQuery(world);
        });

        return *this;
    }

    inline QueryBuilder & QueryBuilder::withPrefabs()
    {
        world->update<Query>(id, [=](Query * qp){
            qp->without.erase(world->getComponentId<Prefab>());
            qp->recalculateQuery(world);
        });

        return *this;
    }

    inline QueryBuilder & QueryBuilder::withInheritance(bool inherit)
    {
        world->update<Query>(id, [=](Query * qp){
            qp->inheritance = inherit;
        });

        return *this;
    }

    inline QueryBuilder & QueryBuilder::withJob()
    {
        world->update<Query>(id, [](Query * qp){
            qp->thread = true;
        });
        return *this;
    }
}
