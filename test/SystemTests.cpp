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

#include <random>

#include "doctest.h"
#include "RxECS.h"
#include "TestComponents.h"
#include "EntityImpl.h"

TEST_SUITE("Systems")
{
    TEST_CASE("Basic")
    {
        ecs::World world;

        world.newEntity().set<TestComponent>({2});

        uint32_t zz = 0;

        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        auto system = world.createSystem("Basic").withQuery<TestComponent>()
                           .without<TestComponent2>()
                           .inGroup("Group:1")
                           .each<TestComponent>(
                               [&zz](ecs::EntityHandle, const TestComponent * xy)
                               {
                                   zz = xy->x;
                               });

        world.step(0.01f);

        CHECK(zz == 2);

        world.deleteSystem(system.id);

        //auto sp = w.get<ecs::System>(s);
    }

    TEST_CASE("System Ordering")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});

        struct Label1 { };
        struct Label2 { };
        struct Label3 { };

        int c = 0;
        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        world.createSystem("Ordering").withQuery<TestComponent>()
             .label<Label1>()
             .label<Label3>()
             .inGroup("Group:1")
             .after<Label2>()
             .each<TestComponent>([&c](ecs::EntityHandle, const TestComponent *)
             {
                 c++;
                 CHECK(c == 3);
             });
        world.createSystem("Ordering2").withQuery<TestComponent>()
             .label<Label2>()
             .inGroup("Group:1")
             .each<TestComponent>([&c](ecs::EntityHandle, const TestComponent *)
             {
                 c++;
                 CHECK(c == 1);
             });
        world.createSystem("Ordering3").withQuery<TestComponent>()
             .label<Label3>()
             .inGroup("Group:1")
             .each<TestComponent>([&c](ecs::EntityHandle, const TestComponent *)
             {
                 c++;
                 CHECK(c == 4);
             });
        world.createSystem("Ordering4").withQuery<TestComponent>()
             .before<Label3>()
             .before<Label1>()
             .inGroup("Group:1")
             .each<TestComponent>([&c](ecs::EntityHandle, const TestComponent *)
             {
                 c++;
                 CHECK(c == 2);
             });
        world.step(0.01f);
        c = 0;
        world.step(0.01f);
    }

    TEST_CASE("System Set")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});

        auto sete = world.newEntity().set<ecs::SystemSet>({true});
        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        uint32_t zz = 0;

        auto system = world.createSystem("Set").withQuery<TestComponent>()
                           .without<TestComponent2>()
                           .withSet(sete.id)
                           .inGroup("Group:1")
                           .each<TestComponent>(
                               [&zz](ecs::EntityHandle, const TestComponent * xy)
                               {
                                   zz = xy->x;
                               });

        world.step(0.01f);

        CHECK(zz == 2);
        zz = 0;
        sete.update<ecs::SystemSet>([](ecs::SystemSet * ss){
            ss->enabled = false;
        });
        world.markSystemsDirty();
        world.step(0.01f);

        CHECK(zz == 0);

        world.deleteSystem(system.id);
    }

    TEST_CASE("System Disabling")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});
        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        uint32_t zz = 0;

        auto system = world.createSystem("Disabling").withQuery<TestComponent>()
                           .without<TestComponent2>()
                           .inGroup("Group:1")
                           .each<TestComponent>(
                               [&zz](ecs::EntityHandle, const TestComponent * xy)
                               {
                                   zz = xy->x;
                               });

        world.step(0.01f);

        CHECK(zz == 2);
        zz = 0;
        world.update<ecs::System>(system.id, [](ecs::System * ss){
            ss->enabled = false;
        });
        world.markSystemsDirty();
        world.step(0.01f);

        CHECK(zz == 0);

        world.deleteSystem(system.id);
    }

    TEST_CASE("Execute System")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});
        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        uint32_t zz = 0;
        {
            world.createSystem("Execute")
                 .inGroup("Group:1")
                 .execute([&zz, &world](ecs::World * w)
                 {
                     zz = 2;
                     CHECK(w == &world);
                 });
        }
        world.step(0.01f);

        CHECK(zz == 2);

        //world.deleteSystem(system.id);
    }

    TEST_CASE("Multiple Groups")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});
        world.newEntity("Group:1").set<ecs::SystemGroup>({2, false, 0.f, 0.f});
        world.newEntity("Group:2").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        uint32_t zz = 0;

        auto system = world.createSystem("Execute")
                           .inGroup("Group:1")
                           .execute([&zz](ecs::World *)
                           {
                               CHECK(zz == 1);
                               zz++;
                           });
        world.createSystem("Execute 2")
             .inGroup("Group:2")
             .execute([&zz](ecs::World *)
             {
                 CHECK(zz == 0);
                 zz = 1;
             });

        world.step(0.01f);

        CHECK(zz == 2);
        zz = 0;
        world.step(0.01f);

        world.deleteSystem(system.id);
    }

    TEST_CASE("Fixed Rate")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});
        world.newEntity("Group:1").set<ecs::SystemGroup>({2, false, 0.f, 0.f});
        world.newEntity("Group:2").set<ecs::SystemGroup>({1, true, 0.0f, 0.1f});

        uint32_t z1 = 0;
        uint32_t z2 = 0;

        auto system = world.createSystem("Execute")
                           .inGroup("Group:1")
                           .execute([&z1](ecs::World * w)
                           {
                               CHECK(w->deltaTime() != 0.1f);
                               z1++;
                           });
        world.createSystem("Execute 2")
             .inGroup("Group:2")
             .execute([&z2](ecs::World * w)
             {
                 CHECK(w->deltaTime() == 0.1f);
                 z2++;
             });

        world.step(0.02f);
        CHECK(z1 == 1);
        CHECK(z2 == 0);
        world.step(0.02f);
        CHECK(z1 == 2);
        CHECK(z2 == 0);
        world.step(0.02f);
        CHECK(z1 == 3);
        CHECK(z2 == 0);
        world.step(0.02f);
        CHECK(z1 == 4);
        CHECK(z2 == 0);
        world.step(0.021f);
        CHECK(z1 == 5);
        CHECK(z2 == 1);
        world.step(0.21f);
        CHECK(z1 == 6);
        CHECK(z2 == 3);

        world.deleteSystem(system.id);
    }

    TEST_CASE("Auto System Ordering")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});

        int c = 0;

        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        world.createSystem("Ordering2").withQuery<TestComponent2>()
             .inGroup("Group:1")
             .each<TestComponent2>([&c](ecs::EntityHandle, const TestComponent2 *)
             {
                 c++;
                 CHECK(c == 2);
             });

        world.createSystem("Ordering").withQuery<TestComponent>()
             .inGroup("Group:1")
             .withWrite<TestComponent2>()
             .each<TestComponent>([&c](ecs::EntityHandle, const TestComponent *)
             {
                 c++;
                 CHECK(c == 1);
             });

        world.step(0.01f);
    }

    TEST_CASE("Auto System Ordering 2")
    {
        ecs::World world;
        struct C1 {};
        struct C2 {};
        struct C3 {};

        world.newEntity().add<C1>();

        int c = 0;

        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        world.createSystem("Ordering2")
             .withQuery<C1>()
             .inGroup("Group:1")
             .each<C1, C2>([&c](ecs::EntityHandle, C1 *, C2 *)
             {
                 c++;
                 CHECK(c == 1);
             });

        world.step(0.01f);
        CHECK(c == 1);
    }

    TEST_CASE("System Modules")
    {
        ecs::World world;

        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        auto mod = world.newEntity().add<ecs::ModuleComponent>();

        uint32_t c = 0;
        {
            world.pushModuleScope(mod);
            world.createSystem("Test1")
                .inGroup("Group:1")
                .execute([&](ecs::World*)
                    {
                        c++;
                    });
            world.popModuleScope();
        }

        world.setModuleEnabled(mod, false);
        world.step(0.0f);
        CHECK(c == 0);

        world.setModuleEnabled(mod, true);
        world.step(0.0f);
        CHECK(c == 1);
    }
}
