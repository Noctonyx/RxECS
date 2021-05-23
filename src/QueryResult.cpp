#include "QueryResult.h"

namespace ecs
{
    QueryResultChunkRowIterator & QueryResultChunkRowIterator::operator++()
    {
        chunk->checkValidity();
        row++;
        return *this;
    }

    uint32_t QueryResultChunkRowIterator::operator*() const
    {
        //chunk->checkValidity();
        return row;
    }

    void QueryResultChunk::checkIndex(uint32_t rowIndex) const
    {
        if (rowIndex > table->entities.size()) {
            throw std::range_error("row out of range");
        }
    }

    entity_t QueryResultChunk::entity(uint32_t rowIndex) const
    {
        //checkValidity();
        checkIndex(rowIndex);
        return table->entities[rowIndex];
    }

    const void * QueryResultChunk::get(component_id_t comp, const uint32_t row) const
    {
        return getUpdate(comp, row);
    }

    void QueryResultChunk::checkValidity() const
    {
        if (!result->valid) {
            throw std::runtime_error("Invalidated query results");
        }
    }

    void * QueryResultChunk::getUpdate(component_id_t comp, const uint32_t row) const
    {
        //checkValidity();
        //checkIndex(row);
        auto c = table->columns.find(comp);
        if (c == table->columns.end()) {
            return nullptr;
        }

        return (*c).second->getEntry(row);
    }

    const QueryResultChunk & QueryResultChunkIterator::operator*() const
    {
        return result->chunks.at(row);
    }

    QueryResult::QueryResult(
        World * world,
        const std::vector<Table *> & tableList,
        const std::set<component_id_t> & with,
        const std::set<std::pair<component_id_t, std::set<component_id_t>>> & withRelations,
        const std::set<component_id_t> & withSingletons,
        bool inherit
    )
        : world(world)
        , inheritance(inherit)
    {
        total = 0;
        for (auto t: tableList) {
            size_t st = 0;
            while (st < t->entities.size()) {
                auto& ch = chunks.emplace_back();
                ch.world = world;
                ch.table = t;
                ch.result = this;
                ch.startRow = st;
                ch.count = std::min(t->entities.size() - st, 512ULL);
                for (auto w : with) {
                    auto it = (t->columns.find(w));
                    if (it != t->columns.end()) {
                        ch.columns[w] = it->second;
                    }

                    components.insert(w);
                }
                st += ch.count;
            }
            total += static_cast<uint32_t>(t->entities.size());

            t->addQueryResult(this);
        }
        for (auto w: with) {
            components.insert(w);
        }

        for (auto& [c, v] : withRelations) {
            for (auto vx : v) {
                relationLookup[vx] = c;
            }
        }

//        for (const auto& r: withRelations) {
  //          relations.insert(r);
    //    }

        for (auto r: withSingletons) {
            singletons.insert(r);
        }

        valid = true;
    }

    QueryResult::~QueryResult()
    {
        for (auto t: chunks) {
            t.table->removeQueryResult(this);
        }
    }

    void * QueryResult::checkTables(QueryResultChunk & chunk,
                                    const component_id_t componentId,
                                    const uint32_t row,
                                    bool mutate)
    {
        void * ptr;

        if (mutate) {
            ptr = chunk.getUpdate(componentId, row);
        } else {
            ptr = const_cast<void *>(chunk.get(componentId, row));
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

    void * QueryResult::checkRelations(const QueryResultChunk & chunk,
                                       const uint32_t row,
                                       const component_id_t componentId,
                                       bool mutate) const
    {
        auto it = relationLookup.find(componentId);
        if(it == relationLookup.end()) {
            return nullptr;
        }
        auto relation = it->second;

        const auto relation_ptr = chunk.get(relation, row);
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
