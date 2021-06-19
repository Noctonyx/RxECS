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

#include <array>
#include "System.h"
#include "Stream.h"
#include "SystemBuilder.h"

namespace ecs
{
    template<class ... TArgs>
    SystemBuilder & SystemBuilder::withQuery()
    {
        assert(type == SystemType::None);
        type = SystemType::Query;

        world->markSystemsDirty();

        std::set<component_id_t> with = {world->getComponentId<TArgs>()...};
        q = world->createQuery(with).id;
        world->update<System>(id, [=](auto * s){
            s->query = q;
            for (auto value: with) {
                s->reads.insert(value);
            }
        });

        qb = QueryBuilder{q, world};

        return *this;
    }

    template<class T>
    SystemBuilder & SystemBuilder::withStream()
    {
        assert(type == SystemType::None);
        type = SystemType::Stream;

        world->markSystemsDirty();

        //auto s = world->getUpdate<System>(id);
        world->update<System>(id, [=](auto * s){
            s->stream = world->getComponentId<T>();
            s->reads.insert(world->getComponentId<T>());
        });

        stream = id;
        return *this;
    }

    template<class ... TArgs>
    SystemBuilder & SystemBuilder::without()
    {
        assert(type == SystemType::Query);

        world->markSystemsDirty();

        assert(q);
        qb.without<TArgs...>();

        return *this;
    }
#if 0
    template<class ... TArgs>
    SystemBuilder & SystemBuilder::with()
    {
        world->markSystemsDirty();

        assert(q);
        qb.with<TArgs...>();

        std::array<component_id_t, sizeof...(TArgs)> comps = {world->getComponentId<TArgs>()...};
        for (auto value : comps) {
            world->getUpdate<System>(id)->reads.insert(value);
        }

        return *this;
    }
#endif
    template<class T, class ... U>
    SystemBuilder & SystemBuilder::withRelation()
    {
        assert(type == SystemType::Query);
        world->markSystemsDirty();

        assert(q);
        qb.withRelation<T, U ...>();

        std::array<component_id_t, sizeof...(U)> comps = {world->getComponentId<U>()...};

        world->update<System>(id, [&](System * sp){
            sp->reads.insert(world->getComponentId<T>());
            for (auto value : comps) {
                sp->reads.insert(value);
            }
        });

        return *this;
    }

    template<class ... TArgs>
    SystemBuilder & SystemBuilder::withSingleton()
    {
        assert(type == SystemType::Query);

        world->markSystemsDirty();

        if (q) {
            assert(q);
            qb.withSingleton<TArgs ...>();
        }
        return *this;
    }

    template<typename T>
    SystemBuilder & SystemBuilder::label()
    {
        world->markSystemsDirty();

        label(world->getComponentId<T>());
        return *this;
    }

    template<typename T>
    SystemBuilder & SystemBuilder::before()
    {
        world->markSystemsDirty();

        before(world->getComponentId<T>());
        return *this;
    }

    template<typename T>
    SystemBuilder & SystemBuilder::after()
    {
        world->markSystemsDirty();

        after(world->getComponentId<T>());
        return *this;
    }

    template<class ... TArgs>
    SystemBuilder & SystemBuilder::withRead()
    {
        world->markSystemsDirty();

        std::array<component_id_t, sizeof...(TArgs)> comps = {world->getComponentId<TArgs>() ...};
        world->update<System>(id, [&](auto * s){
            for (auto c: comps) {
                s->reads.insert(c);
            }
        });

        return *this;
    }

    template<class ... TArgs>
    SystemBuilder & SystemBuilder::withWrite()
    {
        world->markSystemsDirty();

        std::array<component_id_t, sizeof...(TArgs)> comps = {world->getComponentId<TArgs>() ...};
        world->update<System>(id, [&](auto * s){
            for (auto c: comps) {
                s->writes.insert(c);
            }
        });

        return *this;
    }

    template<class ... TArgs>
    SystemBuilder & SystemBuilder::withStreamRead()
    {
        world->markSystemsDirty();

        std::array<component_id_t, sizeof...(TArgs)> comps = {world->getComponentId<TArgs>() ...};
        world->update<System>(id, [&](auto * s){
            for (auto c: comps) {
                s->streamReads.insert(c);
            }
        });

        return *this;
    }

    template<class ... TArgs>
    SystemBuilder & SystemBuilder::withStreamWrite()
    {
        world->markSystemsDirty();

        std::array<component_id_t, sizeof...(TArgs)> comps = {world->getComponentId<TArgs>() ...};
        world->update<System>(id, [&](auto * s){
            for (auto c: comps) {
                s->streamWrites.insert(c);
            }
        });

        return *this;
    }

    template<typename ... U, typename Func>
    SystemBuilder & SystemBuilder::each(Func && f)
    {
        assert(type == SystemType::Query);
        assert(q);

        //auto s = world->getUpdate<System>(id);

        world->update<System>(id, [&](System * s){
            assert(s->groupId);

            auto mutableParameters = get_mutable_parameters(f);
            std::array<component_id_t, sizeof...(U)> comps = {world->getComponentId<U>() ...};

            uint32_t i = 0;
            for (auto mutableParameter: mutableParameters) {
                if (i > 0) {
                    if (mutableParameter) {
                        s->writes.insert(comps[i - 1]);
                    } else {
                        s->reads.insert(comps[i - 1]);
                    }
                }
                i++;
            }

            s->queryProcessor = [=](QueryResult& res) {
                res.each<U...>(f);
                return res.getProcessed();
            };
        });

        return *this;
    }

    template<typename Func>
    SystemBuilder & SystemBuilder::execute(Func && f)
    {
        assert(type == SystemType::None);
        type = SystemType::Execute;

        world->update<System>(id, [=](System * s){
            assert(s->groupId);
            s->executeProcessor = [=](ecs::World * w) {
                f(w);
            };
        });

        return *this;
    }

    template<typename Func>
    SystemBuilder & SystemBuilder::executeIfNone(Func && f)
    {
        assert(type == SystemType::Query);
        assert(q);

//        auto s = world->getUpdate<System>(id);
        world->update<System>(id, [=](System * s){
            assert(s->groupId);
            s->executeIfNoneProcessor = [=](ecs::World * w) {
                f(w);
            };
        });

        return *this;
    }

    template<typename U, typename Func>
    SystemBuilder & SystemBuilder::execute(Func && f)
    {
        assert(type == SystemType::Stream);
        assert(stream);

        //auto s = world->getUpdate<System>(id);

        world->update<System>(id, [=](System * s){
            if (!s->groupId) {
                throw std::runtime_error("Missing group for System");
            }
            s->reads.insert(world->getComponentId<U>());

            s->streamProcessor = [=](Stream * stream) {
                stream->each<U>(f);
            };
        });

        return *this;
    }
}
