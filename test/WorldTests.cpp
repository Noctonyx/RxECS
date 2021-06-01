#include <random>

#include "doctest.h"
#include "RxECS.h"
#include "TestComponents.h"

TEST_SUITE("World")
{
    TEST_CASE("Entity basics")
    {
        ecs::World w;

        auto e = w.newEntity();
        CHECK(e.isAlive());
        e.destroy();
        CHECK(!e.isAlive());
        auto e2 = w.newEntity();
        CHECK(e2.isAlive());
        CHECK(!e.isAlive());

        CHECK(!w.isAlive(0));
        CHECK(!w.isAlive(9999999));
    }

    TEST_CASE("Component Add")
    {
        {
            ecs::World * w = new ecs::World;
            auto e = w->newEntity().add<TestComponent>();
            CHECK(e.has<TestComponent>());
            delete w;
        }
    }

    TEST_CASE("Component basics")
    {
        ecs::World w;

        auto e = w.newEntity();
        auto e2 = w.newEntity();

        e.add<TestComponent>();
        CHECK(e.has<TestComponent>());

        e.set<TestComponent>({12});
        CHECK(e.get<TestComponent>()->x == 12);

        e2.set<TestComponent>({11}).set<ecs::Name>({.name = "Fred"});
        CHECK(e2.has<TestComponent>());

        e.set<TestComponent2>({.y=14});
        CHECK(e.has<TestComponent2>());
        CHECK(e.has<TestComponent>());
        CHECK(e.get<TestComponent2>()->y == 14);

        e.remove<TestComponent>();
        CHECK(!e.has<TestComponent>());
        CHECK(e.has<TestComponent2>());
        CHECK(e.get<TestComponent2>()->y == 14);
        CHECK(e2.has<TestComponent>());
        CHECK(e2.get<TestComponent>()->x == 11);

        e2.destroy();
        e.destroy();
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
            auto e = w.newEntity().set<Carrier>(c);
            c.p.reset();
            CHECK(!wp.expired());

            SUBCASE("And Destroy") {
                px.reset();
                CHECK(!wp.expired());
                e.destroy();
                CHECK(wp.expired());
            }
            SUBCASE("And Add") {
                px.reset();
                CHECK(!wp.expired());
                e.add<TestComponent2>();
                CHECK(!wp.expired());
                e.destroy();
                CHECK(wp.expired());
            }
            SUBCASE("And Get") {
                px.reset();
                auto gg = e.get<Carrier>();
                CHECK(gg->p.use_count() == 1);
                CHECK(!wp.expired());
                auto gg2 = e.getUpdate<Carrier>();
                CHECK(gg2->p.use_count() == 1);
                CHECK(!wp.expired());
                gg2->p.reset();
                CHECK(wp.expired());
                e.destroy();
                CHECK(wp.expired());
            }
        }
        SUBCASE("Bulk") {
            Carrier c2{ nullptr };
            auto e = w.newEntity().set<Carrier>(c);
            CHECK(c.p.use_count() == 2);
            CHECK(!wp.expired());

            for (int i = 0; i < 200; i++) {
                w.newEntity().set<Carrier>(c2);
            }
            CHECK(c.p.use_count() == 2);
            CHECK(!wp.expired());
            c.p.reset();
            for (int i = 0; i < 200; i++) {
                w.newEntity().set<Carrier>(c2);
            }
            //CHECK(c.p.use_count() == 1);
            CHECK(!wp.expired());
            e.destroy();
            CHECK(wp.expired());
        }
    }

    TEST_CASE("Missing component should return nullptr")
    {
        ecs::World w;

        auto e = w.newEntity().id;

        CHECK(w.get<TestComponent>(e) == nullptr);
        CHECK(w.getUpdate<TestComponent>(e) == nullptr);
    }

    TEST_CASE("Many Entities")
    {
        ecs::World w;

        const auto c = 1000;
        std::vector<ecs::entity_t> e;
        for (uint32_t i = 0; i < c; i++) {
            auto x = w.newEntity().set<TestComponent>({i});
            e.push_back(x.id);
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

        auto e = w.newEntity().add<TestComponent>().add<TestTag>();

        CHECK(e.has<TestTag>());
        CHECK(e.has<TestComponent>());
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

        x.set<TestComponent>({1});
        y.set<TestComponent2>({.y=5});
        x.set<TestComponent3>({.w=11});

        x.set<ecs::InstanceOf>({{.entity = y.id}});
        y.set<ecs::InstanceOf>({{.entity = z.id}});

        const TestComponent2 * p = x.get<TestComponent2>();
        CHECK(p == nullptr);
        p = y.get<TestComponent2>();
        CHECK(p != nullptr);
        p = x.get<TestComponent2>(true);
        CHECK(p != nullptr);
        CHECK(p->y == 5);
        auto p2 = x.get<TestComponent3>(true);
        CHECK(p2 != nullptr);
        CHECK(p2->w == 11);

        auto p3 = z.get<TestComponent>(true);
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
        prefabBoss.set<BloatBoss>( {.b = 2});
        prefab.set<Bloat>( {.a = 11});
        prefab.set<BloatWith>( {{.entity = prefabBoss.id}});
        prefab.add<ecs::Prefab>();

        SUBCASE("Instantiate") {
            auto i = world.instantiate(prefab.id);
            CHECK(i.has<Bloat>());
            CHECK(i.has<BloatWith>());
            CHECK(i.get<Bloat>()->a == 11);
            CHECK(i.get<BloatWith>()->entity == prefabBoss.id);

            auto j = world.instantiate(prefab.id);
            CHECK(j.has<Bloat>());
            CHECK(j.has<BloatWith>());
            CHECK(j.get<Bloat>()->a == 11);
            CHECK(j.get<BloatWith>()->entity == prefabBoss.id);
        }
        SUBCASE("Instantiate should remove name") {
            world.set<ecs::Name>(prefab.id, { std::string{"abcd"} });
            auto i = world.instantiate(prefab.id);
            CHECK(i.has<Bloat>());
            CHECK(!i.has<ecs::Name>());
            CHECK(i.has<BloatWith>());
            CHECK(i.get<Bloat>()->a == 11);
            CHECK(i.get<BloatWith>()->entity == prefabBoss.id);

            auto j = world.instantiate(prefab.id);
            CHECK(j.has<Bloat>());
            CHECK(j.has<BloatWith>());
            CHECK(j.get<Bloat>()->a == 11);
            CHECK(j.get<BloatWith>()->entity == prefabBoss.id);
        }
        SUBCASE("Prefabs not in Query 1") {
            auto q = world.createQuery<Bloat>().id;

            CHECK(world.getResults(q).count() == 0);
        }
        SUBCASE("Prefabs not in Query 2") {
            auto q = world.createQuery<Bloat>().id;
            world.instantiate(prefab.id);
            auto r = world.getResults(q);
            CHECK(r.count() == 1);
        }
        SUBCASE("Prefabs can be in query") {
            auto q = world.createQuery<Bloat>().withPrefabs().id;
            world.instantiate(prefab.id);
            auto r = world.getResults(q);
            CHECK(r.count() == 2);
        }
    }

    TEST_CASE("Deferred Commands")
    {
        ecs::World w;

        auto e = w.newEntity();
        SUBCASE("Add") {
            e.addDeferred<TestComponent>();
            CHECK(!e.has<TestComponent>());

            w.executeDeferred();
            CHECK(e.has<TestComponent>());
        }

        SUBCASE("Remove") {
            e.add<TestComponent>();
            CHECK(e.has<TestComponent>());
            e.removeDeferred<TestComponent>();
            CHECK(e.has<TestComponent>());

            w.executeDeferred();
            CHECK(!e.has<TestComponent>());
        }

        SUBCASE("Destroy") {
            e.destroyDeferred();
            CHECK(e.isAlive());

            w.executeDeferred();
            CHECK(!e.isAlive());
        }

        SUBCASE("Set") {
            {
                auto f = [&e]()
                {
                    e.setDeferred<TestComponent2>( {.y = 5, .z = "Hello"});
                };

                f();
            }
            CHECK(!e.has<TestComponent2>());
            w.executeDeferred();
            CHECK(e.has<TestComponent2>());
            CHECK(e.get<TestComponent2>()->y == 5);
            CHECK(e.get<TestComponent2>()->z == "Hello");
        }

        SUBCASE("Set 2") {
            {
                auto f = [&e]()
                {
                    e.setDeferred<TestComponent>( {.x=5});
                };

                f();
            }
            CHECK(!e.has<TestComponent>());
            w.executeDeferred();
            CHECK(e.has<TestComponent>());
            CHECK(e.get<TestComponent>()->x == 5);
            e.destroy();
        }
    }

    TEST_CASE("Name")
    {
        ecs::World w;

        SUBCASE("Create with char *") {
            auto e = w.newEntity("Name1");
            auto j = w.lookup("Name1");

            CHECK(e == j);
        }

        SUBCASE("Lookup with string") {
            std::string x = "A String";

            auto e = w.newEntity("A String");
            auto j = w.lookup(x);

            CHECK(e == j);
        }

        SUBCASE("Set via update ") {
            auto e = w.newEntity().set<ecs::Name>({"A Name"});
            auto j = w.lookup("A Name");

            CHECK(e == j);

            e.remove<ecs::Name>();
            j = w.lookup("A Name");
            CHECK(!j.isAlive());
        }
    }

    TEST_CASE("DynamicComponent")
    {
        ecs::World w;

        const auto dce = w.newEntity("TestName");
        const auto dc = w.createDynamicComponent(dce);

        SUBCASE("World Interface") {
            const auto e = w.newEntity();
            w.add(e, dc);

            assert(w.has(e, dc));

            w.remove(e, dc);
            assert(!w.has(e, dc));
        }

        SUBCASE("EntityHandle Interface")
        {
            auto e = w.newEntity();
            e.addDynamic(dc);
            assert(e.hasDynamic(dc));
            e.removeDynamic(dc);
            assert(!e.hasDynamic(dc));
        }
    }
}
