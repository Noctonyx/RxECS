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
        std::unordered_map<component_id_t, uint32_t> streamWriteCounts{};
        std::unordered_map<entity_t, uint32_t> labelCounts{};
        std::unordered_map<entity_t, uint32_t> labelPreCounts{};

        std::vector<systemid_t> systems{};
        std::vector<systemid_t> executionSequence{};

        std::function<void(void)> onBegin{};
        std::function<void(void)> onEnd{};
    };

    struct System
    {
        queryid_t query = 0;
        World* world;
        std::function<void(QueryResult&)> queryProcessor;
        std::function<void(World *)> executeProcessor;
        std::function<void(World *)> executeIfNoneProcessor;
        std::function<void(Stream *)> streamProcessor;
        bool enabled = true;

        std::set<entity_t> labels;
        std::set<entity_t> befores;
        std::set<entity_t> afters;

        std::unordered_set<component_id_t> reads;
        std::unordered_set<component_id_t> writes;

        std::unordered_set<component_id_t> streamReads;
        std::unordered_set<component_id_t> streamWrites;

        entity_t groupId = 0;

        bool complete = false;
        bool ready = false;

        bool thread = false;

        component_id_t stream = 0;
        size_t count = 0;
        float executionTime;
        std::chrono::time_point<std::chrono::steady_clock> startTime;

        uint64_t lastRunSequence = 0;
    };
}
