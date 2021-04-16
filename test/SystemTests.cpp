#include <random>

#include "doctest.h"
#include "RxECS.h"
#include "TestComponents.h"

TEST_SUITE("Systems")
{
    TEST_CASE("Basic")
    {
        ecs::World world;

        world.newEntity().set<TestComponent>({2});

        uint32_t zz = 0;

        auto system = world.createSystem().withQuery<TestComponent>()
                           .without<TestComponent2>()
                           .each<TestComponent>(
                               [&zz](ecs::EntityBuilder, const TestComponent * xy)
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

        struct Label1 {};
        struct Label2 {};
        struct Label3 {};

        int c = 0;

        world.createSystem().withQuery<TestComponent>()
             .label<Label1>()
             .label<Label3>()
             .after<Label2>()
             .each<TestComponent>([&c](ecs::EntityBuilder, TestComponent *)
             {
                 c++;
                 CHECK(c == 3);
             });
        world.createSystem().withQuery<TestComponent>()
             .label<Label2>()
             .each<TestComponent>([&c](ecs::EntityBuilder, TestComponent *)
             {
                 c++;
                 CHECK(c == 1);
             });
        world.createSystem().withQuery<TestComponent>()
             .label<Label3>()
             .each<TestComponent>([&c](ecs::EntityBuilder, TestComponent *)
             {
                 c++;
                 CHECK(c == 4);
             });
        world.createSystem().withQuery<TestComponent>()
             .before<Label3>()
             .before<Label1>()
             .each<TestComponent>([&c](ecs::EntityBuilder, TestComponent *)
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

        uint32_t zz = 0;

        auto system = world.createSystem().withQuery<TestComponent>()
                           .without<TestComponent2>()
                           .withSet(sete.id)
                           .each<TestComponent>(
                               [&zz](ecs::EntityBuilder, const TestComponent * xy)
                               {
                                   zz = xy->x;
                               });

        world.step(0.01f);

        CHECK(zz == 2);
        zz = 0;
        sete.getUpdate<ecs::SystemSet>()->enabled = false;
        world.step(0.01f);

        CHECK(zz == 0);

        world.deleteSystem(system.id);
    }

    TEST_CASE("System Disabling")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});

        uint32_t zz = 0;

        auto system = world.createSystem().withQuery<TestComponent>()
                           .without<TestComponent2>()
                           .each<TestComponent>(
                               [&zz](ecs::EntityBuilder, const TestComponent * xy)
                               {
                                   zz = xy->x;
                               });

        world.step(0.01f);

        CHECK(zz == 2);
        zz = 0;
        world.getUpdate<ecs::System>(system.id)->enabled = false;
        world.step(0.01f);

        CHECK(zz == 0);

        world.deleteSystem(system.id);
    }

    TEST_CASE("Execute System")
    {
        ecs::World world;
        world.newEntity().set<TestComponent>({2});

        uint32_t zz = 0;

        auto system = world.createSystem()
                           .execute([&zz](ecs::World *)
                           {
                               zz = 2;
                           });

        world.step(0.01f);

        CHECK(zz == 2);

        world.deleteSystem(system.id);
    }
}
