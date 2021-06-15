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
#include <map>

#include "ArchetypeManager.h"
#include "Entity.h"
//#include "EntityHandle.h"
#include <chrono>

#include "robin_hood.h"
#include "TableIterator.h"
#include "Column.h"

namespace ecs
{
    struct Table;
    struct QueryResult;
    class World;

    using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;

    struct Table
    {
        World * world;
        uint32_t archetypeId;
        std::vector<entity_t> entities;

        Timestamp lastUpdateTimestamp;

        std::map<component_id_t, std::unique_ptr<Column>> columns;

        Table(World * world, uint16_t archetypeId);

        TableIterator begin();
        TableIterator end();

        void addEntity(entity_t id);
        void removeEntity(entity_t id);

        bool hasComponent(component_id_t componentId) const;
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
