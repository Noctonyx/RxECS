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

#include "QueryResult.h"
#include "World.h"

namespace ecs
{
    const TableView & TableViewIterator::operator*() const
    {
        return result->tableViews.at(row);
    }

    QueryResult::QueryResult(
        World * world,
        const std::vector<Table *> & tableList,
        const std::set<component_id_t> & with,
        const std::set<std::pair<component_id_t, std::set<component_id_t>>> & withRelations,
        const std::set<component_id_t> & withSingletons,
        bool inherit,
        bool thread
    )
        : world(world)
        , inheritance(inherit)
        , thread(thread)
    {
        total = 0;
        for (auto table: tableList) {
            size_t startRow = 0;
            size_t viewSize = std::max(table->entities.size() / 40, 1024ULL);
            while (startRow < table->entities.size()) {
                auto & newView = tableViews.emplace_back();
                newView.world = world;
                newView.table = table;
                newView.tableUpdateTimestamp = table->lastUpdateTimestamp;
                newView.startRow = startRow;
                newView.count = std::min(table->entities.size() - startRow, viewSize);
                for (auto w: with) {
                    components.insert(w);
                }
                startRow += newView.count;
            }
            total += static_cast<uint32_t>(table->entities.size());
        }
        for (auto w: with) {
            components.insert(w);
        }

        for (auto & [c, v]: withRelations) {
            for (auto vx: v) {
                relationLookup[vx] = c;
            }
        }

        for (auto r: withSingletons) {
            singletons.insert(r);
        }
    }

    void * QueryResult::checkTables(TableView & view,
                                    const component_id_t componentId,
                                    const uint32_t row,
                                    bool mutate)
    {
        void * ptr;

        if (mutate) {
            ptr = view.getUpdate(componentId, row);
        } else {
            ptr = const_cast<void *>(view.get(componentId, row));
        }

        return ptr;
    }

    void * QueryResult::checkSingletons(const component_id_t componentId, bool mutate) const
    {
        if (singletons.contains(componentId)) {
            if (mutate) {
                const auto singleton_ptr = world->getSingletonUpdate(componentId);
                return singleton_ptr;
            }
            const auto singleton_ptr = world->getSingleton(componentId);
            return const_cast<void *>(singleton_ptr);
        }
        return nullptr;
    }

    void * QueryResult::checkRelations(const TableView & view,
                                       const uint32_t row,
                                       const component_id_t componentId,
                                       bool mutate) const
    {
        auto it = relationLookup.find(componentId);
        if (it == relationLookup.end()) {
            return nullptr;
        }
        auto relation = it->second;

        const auto relation_ptr = view.get(relation, row);
        if (relation_ptr) {
            auto rp = static_cast<const Relation *>(relation_ptr);
            if (!world->isAlive(rp->entity)) {
                return nullptr;
            }
            if (mutate) {
                auto related_component_ptr = world->getUpdate(rp->entity, componentId);
                if (related_component_ptr) {
                    return related_component_ptr;
                }
            } else {
                auto related_component_ptr = world->get(rp->entity, componentId);
                if (related_component_ptr) {
                    return const_cast<void *>(related_component_ptr);
                }
            }
        }
        return nullptr;
    }

    const void * QueryResult::checkInstancing(entity_t entity,
                                              const component_id_t componentId,
                                              bool mutate) const
    {
        (void) mutate;
        if (world->has<InstanceOf>(entity)) {
            return world->get(entity, componentId, true);
        }
        return nullptr;
    }
}
