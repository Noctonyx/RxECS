#pragma once
#include "Entity.h"
#include "QueryResult.h"
//#include "QueryResult.h"

namespace ecs
{
    class World;
#if 0
    struct Pipeline
    {
        struct StartInit{};
        struct Init{};
        struct EndInit{};

        struct StartMain {};
        struct Main {};
        struct EndMain {};

        struct StartFinal {};
        struct Final{};
        struct EndFinal {};

    };

    namespace PipelineGroup
    {
        struct PreFrame {};
        struct Early {};
        struct FixedUpdate {};
        struct Update {};
        struct PreRender {};
        struct PostRender {};
        struct Final {};
    };
#endif
    struct SystemSet
    {
        bool enabled = true;
    };

    struct SetForSystem : Relation
    {

    };

    struct SystemGroup
    {
        uint32_t sequence;

        bool fixed;

        float delta;
        float rate;
    };

    struct System
    {
        queryid_t query = 0;
        World* world;
        std::function<void(QueryResult&)> queryProcessor;
        std::function<void(World *)> executeProcessor;
        std::function<void(Stream *)> streamProcessor;
        bool enabled = true;
        std::set<entity_t> labels;
        std::set<entity_t> befores;
        std::set<entity_t> afters;

        entity_t groupId = 0;

        bool thread = false;

        component_id_t stream = 0;
        //bool dirtyOrder = true;
    };

}
