#pragma once
#include <vector>

#include "ArchetypeManager.h"
#include "Entity.h"
//#include "EntityHandle.h"
#include "robin_hood.h"
#include "TableIterator.h"

namespace ecs
{
    struct Table;
    struct QueryResult;
    class World;
    struct Column;

    struct Table
    {
        World * world;
        uint32_t archetypeId;
        std::vector<entity_t> entities;
        std::set<QueryResult *> results;

        robin_hood::unordered_map<component_id_t, Column *> columns;

        Table(World * world, uint32_t archetypeId);
        ~Table();

        TableIterator begin();
        TableIterator end();

        void addEntity(entity_t id);
        void removeEntity(entity_t id);

        const void * getComponent(entity_t id, component_id_t componentId);
        void * getUpdateComponent(entity_t id, component_id_t componentId);
        void setComponent(entity_t id, component_id_t componentId, const void * ptr);

        void addQueryResult(QueryResult * i);
        void removeQueryResult(QueryResult * i);
        void invalidateQueryResults();

        uint32_t getEntityRow(entity_t id) const;
        static void moveEntity(World * world,
                               Table * fromTable,
                               Table * toTable,
                               entity_t id,
                               const ArchetypeTransition & trans);

        static void copyEntity(World * world,
                               Table * fromTable,
                               Table * toTable,
                               entity_t id,
                               entity_t newEntity,
                               const ArchetypeTransition & trans);
    };
}
