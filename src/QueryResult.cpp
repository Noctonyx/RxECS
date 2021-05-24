#include "QueryResult.h"

namespace ecs
{
    TableViewRowIterator & TableViewRowIterator::operator++()
    {
        view->checkValidity();
        row++;
        return *this;
    }

    size_t TableViewRowIterator::operator*() const
    {
        return row;
    }

    void TableView::checkIndex(uint32_t rowIndex) const
    {
        if (rowIndex >= startRow + count || rowIndex < startRow) {
            throw std::range_error("row out of range");
        }
    }

    entity_t TableView::entity(uint32_t rowIndex) const
    {
        checkIndex(rowIndex);
        return table->entities[rowIndex];
    }

    const void * TableView::get(component_id_t comp, const uint32_t row) const
    {
        return getUpdate(comp, row);
    }

    void TableView::checkValidity() const
    {
        if (tableUpdateTimestamp != table->lastUpdateTimestamp) {
            throw std::runtime_error("Invalidated query results");
        }
    }

    void * TableView::getUpdate(component_id_t comp, const uint32_t row) const
    {
        auto c = table->columns.find(comp);
        if (c == table->columns.end()) {
            return nullptr;
        }

        return (*c).second->getEntry(row);
    }

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
        for (auto t: tableList) {
            size_t st = 0;
            size_t cs = std::max(t->entities.size() / 40, 1024ULL);
            while (st < t->entities.size()) {
                auto& ch = tableViews.emplace_back();
                ch.world = world;
                ch.table = t;
                ch.tableUpdateTimestamp = t->lastUpdateTimestamp;
                ch.startRow = st;
                ch.count = std::min(t->entities.size() - st, cs);
                for (auto w : with) {
                    components.insert(w);
                }
                st += ch.count;
            }
            total += static_cast<uint32_t>(t->entities.size());
        }
        for (auto w: with) {
            components.insert(w);
        }

        for (auto& [c, v] : withRelations) {
            for (auto vx : v) {
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
        if(it == relationLookup.end()) {
            return nullptr;
        }
        auto relation = it->second;

        const auto relation_ptr = view.get(relation, row);
        if (relation_ptr) {
            auto rp = static_cast<const Relation*>(relation_ptr);
            if (!world->isAlive(rp->entity)) {
                return nullptr;
            }
            if (mutate) {
                auto related_component_ptr = world->getUpdate(rp->entity, componentId);
                if (related_component_ptr) {
                    return related_component_ptr;
                }
            }
            else {
                auto related_component_ptr = world->get(rp->entity, componentId);
                if (related_component_ptr) {
                    return const_cast<void*>(related_component_ptr);
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
