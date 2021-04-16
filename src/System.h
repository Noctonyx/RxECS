#pragma once
#include "Entity.h"
#include "QueryResult.h"
//#include "QueryResult.h"

namespace ecs
{
    class World;

    struct Pipeline
    {
        struct StartInit{};
        struct EndInit{};

        struct StartMain {};
        struct EndMain {};

        struct StartFinal {};
        struct EndFinal {};
    };

    struct SystemSet
    {
        bool enabled = true;
    };

    struct SetForSystem : Relation
    {

    };

    struct System
    {
        queryid_t query = 0;
        World* world;
        std::function<void(QueryResult&)> queryProcessor;
        std::function<void()> executeProcessor;
        std::function<void(Stream *)> streamProcessor;
        bool enabled = true;
        std::set<entity_t> labels;
        std::set<entity_t> befores;
        std::set<entity_t> afters;

        streamid_t stream = 0;
        //bool dirtyOrder = true;
    };

}
