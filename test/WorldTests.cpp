#include <random>

#include "doctest.h"
#include "World.h"
#include "QueryResult.h"
#include "TestComponents.h"
#include "QueryImpl.h"

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
            ecs::World * w = new ecs::World;
            auto e = w->newEntity();
            w->add<TestComponent>(e);
            CHECK(w->has<TestComponent>(e));
            delete w;
        }
    }

    TEST_CASE("Component basics")
    {
        ecs::World w;

        auto e = w.newEntity();
        auto e2 = w.newEntity();

        w.add<TestComponent>(e);
        CHECK(w.has<TestComponent>(e));

        w.set<TestComponent>(e, {12});
        CHECK(w.get<TestComponent>(e)->x == 12);

        w.set<TestComponent>(e2, {11});
        w.set<ecs::Name>(e2, {.name = "Fred"});
        CHECK(w.has<TestComponent>(e2));

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

        w.destroy(e2);
        w.destroy(e);
    }

    TEST_CASE("Component lifetime")
    {
        ecs::World w;

        struct Payload
        {
            uint32_t h;
        };

        std::shared_ptr<Payload> ptr = std::make_shared<Payload>();
        ptr->h = 12;

        struct Carrier
        {
            std::shared_ptr<Payload> p;
        };

        std::weak_ptr<Payload> wp = ptr;

        Carrier c{ptr};

        ptr.reset();

        CHECK(c.p.use_count() == 1);
        CHECK(!wp.expired());

        SUBCASE("Set") {
            auto px = c.p;
            auto e = w.newEntity();
            w.set<Carrier>(e, c);
            c.p.reset();
            CHECK(!wp.expired());

            SUBCASE("And Destroy") {
                px.reset();
                CHECK(!wp.expired());
                w.destroy(e);
                CHECK(wp.expired());
            }
            SUBCASE("And Add") {
                px.reset();
                CHECK(!wp.expired());
                w.add<TestComponent2>(e);
                CHECK(!wp.expired());
                w.destroy(e);
                CHECK(wp.expired());
            }
            SUBCASE("And Get") {
                px.reset();
                auto gg = w.get<Carrier>(e);
                CHECK(gg->p.use_count() == 1);
                CHECK(!wp.expired());
                auto gg2 = w.getUpdate<Carrier>(e);
                CHECK(gg2->p.use_count() == 1);
                CHECK(!wp.expired());
                gg2->p.reset();
                CHECK(wp.expired());
                w.destroy(e);
                CHECK(wp.expired());
            }
        }
        SUBCASE("Bulk") {
            Carrier c2(nullptr);
            auto e = w.newEntity();
            w.set<Carrier>(e, c);
            CHECK(c.p.use_count() == 2);
            CHECK(!wp.expired());

            for (int i = 0; i < 200; i++) {
                auto e1 = w.newEntity();
                w.set<Carrier>(e1, c2);
            }
            CHECK(c.p.use_count() == 2);
            CHECK(!wp.expired());
            c.p.reset();
            for (int i = 0; i < 200; i++) {
                auto e1 = w.newEntity();
                w.set<Carrier>(e1, c2);
            }
            //CHECK(c.p.use_count() == 1);
            CHECK(!wp.expired());
            w.destroy(e);
            CHECK(wp.expired());
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
        std::vector<ecs::entity_t> e;
        for (uint32_t i = 0; i < c; i++) {
            auto x = w.newEntity();
            w.set<TestComponent>(x, {i});
            e.push_back(x);
        }

        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(e), std::end(e), rng);

        for (auto & xx: e) {
            w.add<TestComponent2>(xx);
        }

        std::shuffle(std::begin(e), std::end(e), rng);

        for (auto & xx: e) {
            w.remove<TestComponent>(xx);
        }

        std::shuffle(std::begin(e), std::end(e), rng);

        for (auto & xx: e) {
            w.destroy(xx);
        }
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

    TEST_CASE("Prefabs")
    {
        struct BloatBoss
        {
            uint32_t b;
        };

        struct Bloat
        {
            uint32_t a;
        };

        struct BloatWith : ecs::Relation { };

        ecs::World world;
        auto prefab = world.newEntity();
        auto prefabBoss = world.newEntity();
        world.set<BloatBoss>(prefabBoss, {.b = 2});
        world.set<Bloat>(prefab, {.a = 11});
        world.set<BloatWith>(prefab, {{.entity = prefabBoss}});
        world.add<ecs::Prefab>(prefab);

        SUBCASE("Instantiate") {
            auto i = world.instantiate(prefab);
            CHECK(world.has<Bloat>(i));
            CHECK(world.has<BloatWith>(i));
            CHECK(world.get<Bloat>(i)->a == 11);
            CHECK(world.get<BloatWith>(i)->entity == prefabBoss);

            auto j = world.instantiate(prefab);
            CHECK(world.has<Bloat>(j));
            CHECK(world.has<BloatWith>(j));
            CHECK(world.get<Bloat>(j)->a == 11);
            CHECK(world.get<BloatWith>(j)->entity == prefabBoss);
        }
        SUBCASE("Prefabs not in Query 1") {
            auto q = world.createQuery<Bloat>().id;

            CHECK(world.getResults(q).count() == 0);
        }
        SUBCASE("Prefabs not in Query 2") {
            auto q = world.createQuery<Bloat>().id;
            world.instantiate(prefab);
            auto r = world.getResults(q);
            CHECK(r.count() == 1);
        }
        SUBCASE("Prefabs can be in query") {
            auto q = world.createQuery<Bloat>().withPrefabs().id;
            world.instantiate(prefab);
            auto r = world.getResults(q);
            CHECK(r.count() == 2);
        }
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
                auto f = [&e, &w]()
                {
                    w.setDeferred<TestComponent>(e, {.x=5});
                };

                f();
            }
            CHECK(!w.has<TestComponent>(e));
            w.executeDeferred();
            CHECK(w.has<TestComponent>(e));
            CHECK(w.get<TestComponent>(e)->x == 5);
            w.destroy(e);
        }
    }
}
