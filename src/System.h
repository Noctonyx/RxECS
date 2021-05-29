#pragma once
#include <deque>
#include <unordered_set>
#include <unordered_map>

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

        bool fixed = false;

        float delta = 0.f;
        float rate = 0.f;

        float lastTime = 0.f;
        float deferredTime = 0.f;
        size_t deferredCount = 0LL;

        std::unordered_map<component_id_t, uint32_t> writeCounts{};
        std::unordered_map<entity_t, uint32_t> labelCounts{};
        std::unordered_map<entity_t, uint32_t> labelPreCounts{};

        std::vector<systemid_t> systems{};
        std::vector<systemid_t> executionSequence{};
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

        std::unordered_set<component_id_t> reads;
        std::unordered_set<component_id_t> writes;

        entity_t groupId = 0;

        bool complete = false;
        bool ready = false;

        bool thread = false;

        component_id_t stream = 0;
        size_t count = 0;
        float executionTime;
        //bool dirtyOrder = true;
    };

}
