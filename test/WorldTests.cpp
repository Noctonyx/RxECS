#include <random>

#include "doctest.h"
#include "World.h"
#include "QueryResult.h"
#include "TestComponents.h"

//int TestComponent::c = 0;

TEST_SUITE("World")
{
    TEST_CASE("Entity basics")
    {
        ecs::World w;

        auto e = w.newEntity();
        CHECK(w.isAlive(e));
        w.destroy(e);
        CHECK(!w.isAlive(e));
        auto e2 = w.newEntity();
        CHECK(w.isAlive(e2));
        CHECK(!w.isAlive(e));

        CHECK(!w.isAlive(0));
        CHECK(!w.isAlive(9999999));
    }

    TEST_CASE("Component Add")
    {
        {
            ecs::World*  w = new ecs::World;
            auto e = w->newEntity();
            w->add<TestComponent>(e);
            CHECK(w->has<TestComponent>(e));
            delete w;
        }
    }

    TEST_CASE("Component basics")
    {
        ecs::World w;
        //TestComponent::c = 0;

        auto e = w.newEntity();
        auto e2 = w.newEntity();

        w.add<TestComponent>(e);
        CHECK(w.has<TestComponent>(e));

        w.set<TestComponent>(e, {12});
        CHECK(w.get<TestComponent>(e)->x == 12);

        //w.add<TestComponent>(e2);
        w.set<TestComponent>(e2, {11});
        w.set<ecs::Name>(e2, {.name = "Fred"});
        CHECK(w.has<TestComponent>(e2));

        //CHECK(TestComponent::c == 2);

        w.set<TestComponent2>(e, {.y=14});
        CHECK(w.has<TestComponent2>(e));
        CHECK(w.has<TestComponent>(e));
        CHECK(w.get<TestComponent2>(e)->y == 14);

        w.remove<TestComponent>(e);
        CHECK(!w.has<TestComponent>(e));
        CHECK(w.has<TestComponent2>(e));
        CHECK(w.get<TestComponent2>(e)->y == 14);
        CHECK(w.has<TestComponent>(e2));
        CHECK(w.get<TestComponent>(e2)->x == 11);

        //CHECK(TestComponent::c == 1);

        w.destroy(e2);
        w.destroy(e);

        //CHECK(TestComponent::c == 0);
    }

    TEST_CASE("Component lifetime")
    {
        ecs::World w;

        auto e = w.newEntity();

        //TestComponent::c = 0;
        //CHECK(TestComponent::c == 0);

        SUBCASE("Set") {
            w.set<TestComponent>(e, {4});
            CHECK(w.has<TestComponent>(e));
            //CHECK(TestComponent::c == 1);
            w.destroy(e);
            //CHECK(TestComponent::c == 0);
        }

        SUBCASE("Set and Add") {
            w.set<TestComponent>(e, {4});
            CHECK(w.has<TestComponent>(e));
            //CHECK(TestComponent::c == 1);
            w.add<TestComponent2>(e);
            //CHECK(TestComponent::c == 1);
            w.destroy(e);
            //CHECK(TestComponent::c == 0);
        }

        SUBCASE("Add") {
            w.add<TestComponent>(e);
            CHECK(w.has<TestComponent>(e));
            //CHECK(TestComponent::c == 1);
            w.destroy(e);
            //CHECK(TestComponent::c == 0);
        }
    }

    TEST_CASE("Missing component should return nullptr")
    {
        ecs::World w;

        auto e = w.newEntity();

        CHECK(w.get<TestComponent>(e) == nullptr);
        CHECK(w.getUpdate<TestComponent>(e) == nullptr);
    }

    TEST_CASE("Many Entities")
    {
        ecs::World w;

        const auto c = 1000;
        //TestComponent::c = 0;

        std::vector<ecs::entity_t> e;
        for (uint32_t i = 0; i < c; i++) {
            auto x = w.newEntity();
            w.set<TestComponent>(x, {i});
            e.push_back(x);
        }

        //CHECK(TestComponent::c == c);

        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(e), std::end(e), rng);

        for (auto & xx: e) {
            w.add<TestComponent2>(xx);
        }

        std::shuffle(std::begin(e), std::end(e), rng);

        for (auto & xx: e) {
            w.remove<TestComponent>(xx);
        }

        //CHECK(TestComponent::c == 0);

        std::shuffle(std::begin(e), std::end(e), rng);

        for (auto & xx: e) {
            w.destroy(xx);
        }

        //CHECK(TestComponent::c == 0);
    }

    TEST_CASE("Tag Components")
    {
        ecs::World w;

        auto e = w.newEntity();
        w.add<TestComponent>(e);

        w.add<TestTag>(e);

        CHECK(w.has<TestTag>(e));
        CHECK(w.has<TestComponent>(e));
    }

    TEST_CASE("Singleton support")
    {
        ecs::World w;

        w.addSingleton<TestComponent>();
        w.setSingleton<TestComponent2>({.y=4, .z = "Hello"});

        CHECK(w.hasSingleton<TestComponent>());
        CHECK(w.hasSingleton<TestComponent2>());

        CHECK(w.getSingleton<TestComponent2>()->y == 4);

        w.removeSingleton<TestComponent2>();
        CHECK(!w.hasSingleton<TestComponent2>());
    }

    TEST_CASE("Instancing")
    {
        ecs::World world;

        auto x = world.newEntity();
        auto y = world.newEntity();
        auto z = world.newEntity();

        world.set<TestComponent>(x, {1});
        world.set<TestComponent2>(y, {.y=5});
        world.set<TestComponent3>(z, {.w=11});

        world.set<ecs::InstanceOf>(x, {{.entity = y}});
        world.set<ecs::InstanceOf>(y, {{.entity = z}});

        const TestComponent2 * p = world.get<TestComponent2>(x);
        CHECK(p == nullptr);
        p = world.get<TestComponent2>(y);
        CHECK(p != nullptr);
        p = world.get<TestComponent2>(x, true);
        CHECK(p != nullptr);
        CHECK(p->y == 5);
        auto p2 = world.get<TestComponent3>(x, true);
        CHECK(p2 != nullptr);
        CHECK(p2->w == 11);

        auto p3 = world.get<TestComponent>(z, true);
        CHECK(p3 == nullptr);
    }

    TEST_CASE("Deferred Commands")
    {
        ecs::World w;

        auto e = w.newEntity();
        SUBCASE("Add") {
            w.addDeferred<TestComponent>(e);
            CHECK(!w.has<TestComponent>(e));

            w.executeDeferred();
            CHECK(w.has<TestComponent>(e));
        }

        SUBCASE("Remove") {
            w.add<TestComponent>(e);
            CHECK(w.has<TestComponent>(e));
            w.removeDeferred<TestComponent>(e);
            CHECK(w.has<TestComponent>(e));

            w.executeDeferred();
            CHECK(!w.has<TestComponent>(e));
        }

        SUBCASE("Destroy") {
            w.destroyDeferred(e);
            CHECK(w.isAlive(e));

            w.executeDeferred();
            CHECK(!w.isAlive(e));
        }

        SUBCASE("Set") {
            {
                auto f = [&e, &w]()
                {
                    w.setDeferred<TestComponent2>(e, {.y = 5, .z = "Hello"});
                };

                f();
            }
            CHECK(!w.has<TestComponent2>(e));
            w.executeDeferred();
            CHECK(w.has<TestComponent2>(e));
            CHECK(w.get<TestComponent2>(e)->y == 5);
            CHECK(w.get<TestComponent2>(e)->z == "Hello");
        }

        SUBCASE("Set 2") {
            {
                //TestComponent::c = 0;

                //CHECK(TestComponent::c == 0);

                auto f = [&e, &w]()
                {
                    w.setDeferred<TestComponent>(e, {.x=5});
                    //CHECK(TestComponent::c == 1);
                };

                f();
                //CHECK(TestComponent::c == 1);
            }
            CHECK(!w.has<TestComponent>(e));
            w.executeDeferred();
            CHECK(w.has<TestComponent>(e));
            CHECK(w.get<TestComponent>(e)->x == 5);
            //CHECK(TestComponent::c == 1);
            w.destroy(e);
            //CHECK(TestComponent::c == 0);
        }
    }
}
