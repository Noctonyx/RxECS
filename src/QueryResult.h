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
#include <cstdint>
#include <set>
#include <unordered_set>
#include <vector>
#include <array>

#include "Entity.h"
#include "Table.h"
#include "World.h"
#include "TableView.h"

template<class T>
concept mutable_parameter = (std::is_reference_v<T> || std::is_pointer_v<T>)
                            && !std::is_const_v<std::remove_reference_t<std::remove_pointer_t<T>>>;

template<class R, class... A>
constexpr auto get_mutable_parameters(R (*)(A ...))
{
    return std::array{mutable_parameter<A>...};
}

template<class R, class O, class... A>
constexpr auto get_mutable_parameters(R (O::*)(A ...))
{
    return std::array{mutable_parameter<A>...};
}

template<class R, class O, class... A>
constexpr auto get_mutable_parameters(R (O::*)(A ...) const)
{
    return std::array{mutable_parameter<A>...};
}

template<class T>
constexpr auto get_mutable_parameters(T)
{
    return get_mutable_parameters(&T::operator());
}

namespace ecs
{
    struct TableView;
    struct QueryResult;

    struct TableViewIterator
    {
        uint32_t row;
        const QueryResult * result;

        bool operator!=(const TableViewIterator & rhs) const
        {
            return row != rhs.row;
        }

        void operator++()
        {
            row++;
        }

        const TableView & operator*() const;
    };

    struct QueryResult
    {
        friend struct TableViewIterator;

    private:
        World * world;
        std::set<component_id_t> components;
        std::vector<TableView> tableViews;
        //std::set<std::pair<component_id_t, std::set<component_id_t>>> relations;
        robin_hood::unordered_map<component_id_t, component_id_t> relationLookup;
        std::set<component_id_t> singletons;
        bool inheritance;
        bool thread;

        uint64_t updatedAfter = 0;
        uint32_t processed = 0;

        std::unordered_map<component_id_t, std::vector<EntityQueue *>> updateTriggers;

    public:
        uint32_t total;
        //bool valid;

        uint32_t getProcessed() const
        { return processed; }

        QueryResult(World * world,
                    const std::vector<Table *> & tableList,
                    const std::set<component_id_t> & with,
                    const std::set<std::pair<component_id_t, std::set<component_id_t>>> & relations,
                    const std::set<component_id_t> & singletons,
                    bool inheritance,
                    bool thread
        );

        void onlyUpdatedAfter(uint64_t seq)
        {
            updatedAfter = seq;
        }

        [[nodiscard]] uint32_t count() const
        {
            return total;
        }

        [[nodiscard]] TableViewIterator begin() const
        {
            return TableViewIterator{0, this};
        }

        [[nodiscard]] TableViewIterator end() const
        {
            return TableViewIterator{static_cast<uint32_t>(tableViews.size()), this};
        }

        [[nodiscard]] size_t size() const
        {
            return tableViews.size();
        }

        TableView & operator[](size_t ix)
        {
            return tableViews[ix];
        }

        template<typename Func>
        void iter(Func && f) const
        {
            for (auto & ch: *this) {
                f(world, ch);
            }
        }

        void * checkTables(TableView & view,
                           component_id_t componentId,
                           uint32_t row,
                           bool mutate);
        void * checkSingletons(component_id_t componentId, bool mutate) const;
        void * checkRelations(const TableView & view,
                              uint32_t row,
                              component_id_t componentId,
                              bool mutate) const;

        const void * checkInstancing(entity_t entity,
                                     component_id_t componentId,
                                     bool mutate) const;

        template<typename Tuple, std::size_t I>
        void setResultValue(entity_t entity,
                            const TableView & view,
                            Column * col,
                            Tuple & t,
                            const uint32_t row,
                            const component_id_t componentId,
                            bool mutate) const;

        template<std::size_t Fixed, typename Tuple, typename Comps, typename Muts, std::size_t ...
        I>
        void populateResult(entity_t entity,
                            const TableView & view,
                            std::array<Column *, sizeof...(I)> columns,
                            Tuple & result,
                            Comps & comps,
                            Muts & muts,
                            const uint32_t row,
                            std::index_sequence<I...>) const;

        template<typename ... U, typename Func>
        void each(Func && f);

        template<typename ... U, typename Func>
        uint32_t eachView(Func && f,
                          const std::array<component_id_t, sizeof...(U)> & comps,
                          const std::array<bool, sizeof...(U) + 1> & mp,
                          const TableView & view) const;

        template<int I>
        void setupUpdateTriggerLists(std::array<component_id_t, I> & comps, const std::array<bool, I + 1> & mp);
    };

    template<typename Tuple, std::size_t I>
    void QueryResult::setResultValue(entity_t entity,
                                     const TableView & view,
                                     Column * col,
                                     Tuple & t,
                                     const uint32_t row,
                                     const component_id_t componentId,
                                     bool mutate) const
    {
        void * result_ptr = nullptr;

        if (col) {
            std::get<I>(t) = static_cast<std::tuple_element_t<I, Tuple>>(col->getEntry(row));
            return;
        }

        if (!result_ptr) {
            result_ptr = checkRelations(view, row, componentId, mutate);
        }
        if (!result_ptr && !mutate && inheritance) {
            result_ptr = const_cast<void *>(checkInstancing(entity, componentId, mutate));
        }
        if (!result_ptr) {
            result_ptr = checkSingletons(componentId, mutate);
        }

        std::get<I>(t) = static_cast<std::tuple_element_t<I, Tuple>>(result_ptr);
    }

    template<std::size_t Fixed, typename Tuple, typename Comps, typename Muts, std::size_t...
    I>
    void QueryResult::populateResult(entity_t entity,
                                     const TableView & view,
                                     std::array<Column *, sizeof...(I)> columns,
                                     Tuple & result,
                                     Comps & comps,
                                     Muts & muts,
                                     const uint32_t row,
                                     std::index_sequence<I...>) const
    {
        (setResultValue<Tuple, I + Fixed>(
            entity, view, columns[I], result, row, comps[I],
            muts[I + Fixed]
        ), ...);
    }

    template<typename ... U, typename Func>
    uint32_t QueryResult::eachView(Func && f,
                                   const std::array<component_id_t, sizeof...(U)> & comps,
                                   const std::array<bool, sizeof...(U) + 1> & mp,
                                   const TableView & view) const
    {
        uint32_t proc = 0;
        auto columns = view.getColumns<U...>(comps);

        for (auto row: view) {
            entity_t ent = view.entity(row);
            if (updatedAfter > 0) {
                if (world->entities[index(ent)].updateSequence <= updatedAfter) {
                    continue;
                }
            }
            //for(size_t row = view.startRow; row < view.count + view.startRow; row++){
            std::tuple<EntityHandle, U * ...> result;
            std::get<0>(result) = EntityHandle{ent, world};
            populateResult<std::tuple_size<decltype(result)>() - sizeof...(U)>(
                ent, view, columns, result, comps, mp, row,
                std::make_index_sequence<sizeof...(U)>()
            );
            std::apply(f, result);
            for(auto c: comps) {
                auto it = updateTriggers.find(c);
                if(it != updateTriggers.end()) {
                    //auto & v = it->second;
                    for(auto t: it->second) {
                        t->add(ent);
                    }
                }
            }
            proc++;
        }

        return proc;
    }

    template<typename ... U, typename Func>
    void QueryResult::each(Func && f)
    {
        std::array<component_id_t, sizeof...(U)> comps = {
            world->getComponentId<std::remove_const_t<U>>()...
        };
        auto mp = get_mutable_parameters(f);

        setupUpdateTriggerLists(comps, mp);

        auto job = world->jobInterface;

        if (job && thread && tableViews.size() > 2 && total > 1000) {
            std::vector<JobInterface::JobHandle> jobs;

            for (auto & view: *this) {
                auto jh = job->create(
                    [=]() {
                        return eachView<U...>(f, comps, mp, view);
                    }
                );

                job->schedule(jh);

                jobs.push_back(jh);
            }

            for (auto & jh: jobs) {
                job->awaitCompletion(jh);
                processed += job->getJobResult(jh);
            }
            jobs.clear();
        } else {
            for (auto & view: *this) {
                processed += eachView<U...>(f, comps, mp, view);
            }
        }
    }

    template<int I>
    void QueryResult::setupUpdateTriggerLists(std::array<component_id_t, I> & comps,
                                              const std::array<bool, I + 1> & mutableParameters)
    {
        uint32_t i = 0;
        for (auto & c: comps) {
            if (mutableParameters[i + 1]) {
                auto cd = world->getComponentDetails(c);
                for (auto updateTrigger: cd->onUpdates) {
                    auto eq = world->getEntityQueue(updateTrigger);
                    if (eq) {
                        updateTriggers[c].push_back(eq);
                    }
                }
            }
        }
    }
}
