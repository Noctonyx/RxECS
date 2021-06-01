#include "Table.h"
#include "World.h"
#include "Column.h"
#include "QueryResult.h"

namespace ecs
{
    Table::Table(World * world, uint16_t archetypeId)
        : world(world)
        , archetypeId(archetypeId)
    {
        auto at = world->am.getArchetypeDetails(archetypeId);
        for (auto & componentId: at.components) {
            columns.emplace(componentId, std::make_unique<Column>(componentId, world));
        }
    }

    TableIterator Table::begin()
    {
        return TableIterator{entities.begin(), world};
    }

    TableIterator Table::end()
    {
        return TableIterator{entities.end(), world};
    }

    void Table::addEntity(const entity_t id)
    {
        auto ix = static_cast<uint32_t>(entities.size());
        world->entities[index(id)].row = ix;
        entities.push_back(id);
        for (auto & [k, v]: columns) {
            v->addEntry();
        }
        stampUpdateTime();
    }

    void Table::removeEntity(entity_t id)
    {
        const uint32_t row = getEntityRow(id);
        for (auto & [k, v]: columns) {
            v->removeEntry(row, true);
        }

        const auto last_entity = entities.back();

        entities[row] = last_entity;
        world->entities[index(last_entity)].row = row;

        entities.pop_back();
        stampUpdateTime();
    }

    bool Table::hasComponent(component_id_t componentId) const
    {
        auto it = columns.find(componentId);

        return (it != columns.end());
    }

    const void * Table::getComponent(entity_t id, component_id_t componentId)
    {
        auto it = columns.find(componentId);

        if (it == columns.end()) {
            return nullptr;
        }
        const uint32_t row = getEntityRow(id);
        auto & c = it->second;

        return c->getEntry(row);
    }

    void * Table::getUpdateComponent(entity_t id, component_id_t componentId)
    {
        if (!columns.contains(componentId)) {
            return nullptr;
        }
        const uint32_t row = getEntityRow(id);
        auto & c = columns[componentId];

        return c->getEntry(row);
    }

    void Table::setComponent(entity_t id, component_id_t componentId, const void * ptr)
    {
        const uint32_t row = getEntityRow(id);
        auto & c = columns[componentId];

        c->setEntry(row, ptr);
    }

    std::string Table::description() const
    {
        std::string r = "";
        for (auto & [x, y]: columns) {
            if (r != "") {
                r += "|";
            }
            r += world->description(x);
        }
        if (r == "") {
            r = "Empty";
        }
        return r;
    }

    uint32_t Table::getEntityRow(entity_t id) const
    {
        return world->entities[index(id)].row; // entitiesIndex.at(id);
    }

    void Table::moveEntity(World * world,
                           Table * fromTable,
                           Table * toTable,
                           entity_t id,
                           const ArchetypeTransition & trans)
    {
        const auto source_row = fromTable->getEntityRow(id);

        auto new_index = static_cast<uint32_t>(toTable->entities.size());

        world->entities[index(id)].row = new_index;
        toTable->entities.push_back(id);

        for (auto & r: trans.removeComponents) {
            fromTable->columns[r]->removeEntry(source_row, true);
        }

        for (auto & a: trans.addComponents) {
            toTable->columns[a]->addEntry();
        }

        for (auto & e: trans.preserveComponents) {
            const auto ptr1 = fromTable->columns[e]->getEntry(source_row);
            toTable->columns[e]->addMoveEntry(ptr1);
            fromTable->columns[e]->removeEntry(source_row, true);
        }

        const auto last_entity = fromTable->entities.back();

        if (source_row != fromTable->entities.size() - 1) {
            fromTable->entities[source_row] = last_entity;
            world->entities[index(last_entity)].row = source_row;
        }
        fromTable->entities.pop_back();
        fromTable->stampUpdateTime();
        toTable->stampUpdateTime();
    }

    void Table::copyEntity(World * world,
                           const Table * fromTable,
                           Table * toTable,
                           entity_t id,
                           entity_t newEntity,
                           const ArchetypeTransition & trans)
    {
        const auto source_row = fromTable->getEntityRow(id);

        auto new_index = static_cast<uint32_t>(toTable->entities.size());

        world->entities[index(newEntity)].row = new_index;
        toTable->entities.push_back(newEntity);

        for (auto & a: trans.addComponents) {
            toTable->columns[a]->addEntry();
        }

        for (auto & e: trans.preserveComponents) {
            const auto ptr1 = fromTable->columns.at(e)->getEntry(source_row);
            toTable->columns[e]->addCopyEntry(ptr1);
        }
        toTable->stampUpdateTime();
    }
}
