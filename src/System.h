#pragma once
#include "Entity.h"
#include "QueryResult.h"
//#include "QueryResult.h"

namespace ecs
{
    class World;

    struct SystemSet
    {
        bool enabled = true;
    };

    struct SetForSystem : Relation
    {

    };

    struct System
    {
        queryid_t query;
        World* world;
        std::function<void(QueryResult&)> queryProcessor;
        std::function<void()> executeProcessor;
        bool enabled = true;
        std::set<entity_t> labels;
        std::set<entity_t> befores;
        std::set<entity_t> afters;
        //bool dirtyOrder = true;
    };

}
