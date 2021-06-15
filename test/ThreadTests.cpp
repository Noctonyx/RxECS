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

//
// Created by shane on 12/06/2021.
//

#include "RxECS.h"
#include "doctest.h"
#include "doctest/trompeloeil.hpp"

#if 0
struct JI : ecs::JobInterface {
    JobHandle create(std::function<void()> function) override
    {

    }

    void schedule(JobHandle handle) override
    {

    }

    bool isComplete(JobHandle handle) const override
    {

    }

    void awaitCompletion(JobHandle handle) override
    {

    }
};
#endif

class J1 : public trompeloeil::mock_interface<ecs::JobInterface>
{
public:
    IMPLEMENT_MOCK1 (create);
    IMPLEMENT_MOCK1 (schedule);
    IMPLEMENT_CONST_MOCK1 (isComplete);
    IMPLEMENT_MOCK1 (awaitCompletion);
    IMPLEMENT_MOCK1 (getJobResult);
};
using trompeloeil::_;

TEST_SUITE("Threads")
{
    TEST_CASE("Simple System")
    {
        ecs::World w;

        w.newEntity("G1").set<ecs::SystemGroup>({1});

        J1 j1;

        w.setJobInterface(&j1);

        int c = 0;
        w.createSystem("S1")
         .inGroup("G1")
         .withJob()
         .execute(
             [&](ecs::World *) {
                 c++;
             }
         );

        std::shared_ptr<int> jh = std::make_shared<int>(4);
        std::function<uint32_t(void)> f;

        ALLOW_CALL(j1, create(_)).LR_SIDE_EFFECT(f = _1).RETURN(jh);
        ALLOW_CALL(j1, schedule(jh)).LR_SIDE_EFFECT(f());
        ALLOW_CALL(j1, awaitCompletion(jh));
        ALLOW_CALL(j1, isComplete(jh)).RETURN(true);

        w.step(0.05f);
        CHECK(c == 1);
    }
}

