#pragma once
#include <vector>

#include "ArchetypeManager.h"
#include "Entity.h"
//#include "EntityHandle.h"
#include <chrono>

#include "robin_hood.h"
#include "TableIterator.h"

namespace ecs
{
    struct Table;
    struct QueryResult;
    class World;
    struct Column;

    using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;

    struct Table
    {
        World * world;
        uint32_t archetypeId;
        std::vector<entity_t> entities;

        Timestamp lastUpdateTimestamp;

        robin_hood::unordered_map<component_id_t, std::unique_ptr<Column>> columns;

        Table(World * world, uint16_t archetypeId);

        TableIterator begin();
        TableIterator end();

        void addEntity(entity_t id);
        void removeEntity(entity_t id);

        const void * getComponent(entity_t id, component_id_t componentId);
        void * getUpdateComponent(entity_t id, component_id_t componentId);
        void setComponent(entity_t id, component_id_t componentId, const void * ptr);

        std::string description() const;

        uint32_t getEntityRow(entity_t id) const;
        static void moveEntity(World * world,
                               Table * fromTable,
                               Table * toTable,
                               entity_t id,
                               const ArchetypeTransition & trans);

        static void copyEntity(World * world,
                               const Table * fromTable,
                               Table * toTable,
                               //Table * existingTable,
                               entity_t id,
                               entity_t newEntity,
                               const ArchetypeTransition & trans);

        void stampUpdateTime()
        {
            lastUpdateTimestamp = std::chrono::high_resolution_clock::now();
        }
    };
}
