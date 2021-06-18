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

#include "doctest.h"
#include "RxECS.h"

TEST_SUITE("Modules")
{
    TEST_CASE("Module Basics")
    {
        ecs::World w;

        w.newEntity("G").set<ecs::SystemGroup>({1});

        class M: public ecs::ModuleBase
        {
        public:
            M(ecs::World * world, unsigned long long int moduleId)
                : ModuleBase(world, moduleId)
            {}
        };

        auto m = w.createModule<M>();
        CHECK(w.getModule<M>() == m);

        std::unique_ptr<M> p = std::make_unique<M>(&w, m);

        w.setModuleObject(m, p.get());

        CHECK(w.getModuleObject<M>() == p.get());

        w.pushModuleScope(m);

        int c = 0;

        auto s = w.createSystem("Test").inGroup("G").execute([&](ecs::World *){
            c++;
        });
        (void)s;

        w.popModuleScope();

        p->disable();

        w.step(0.05f);
        CHECK(c == 0);

        p->enable();

        w.step(0.05f);
        CHECK(c == 1);

        CHECK(p->getModuleId() == m);

        ecs::ModuleBase * x = p.get();
        auto pp = x->getObject<M>();
        //w.getM
        CHECK(pp == p.get());
        p.reset();
    }

    TEST_CASE("Module pointers on wrong entity")
    {
        ecs::World w;

        struct C2 {};
        C2 c2;

        auto e = w.newEntity().add<C2>();

        w.setModuleObject(e, &c2);
        CHECK(w.getModuleObject<C2>() == nullptr);
    }
}