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
                auto gg2 = e.get<Carrier>();
                CHECK(gg2->p.use_count() == 1);
                CHECK(!wp.expired());
                e.update<Carrier>([](Carrier * c){
                    c->p.reset();
                });
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
        int c = 0;
        w.update<TestComponent>(e, [&](TestComponent * ){
            c++;
        });
        CHECK(c == 0);
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

        auto dce = w.newEntity("TestName");
        dce.setAsParent();
        //const auto dc = w.createDynamicComponent(dce);

        SUBCASE("World Interface") {
            const auto e = w.newEntity();
            w.add(e, dce);

            assert(w.has(e, dce));

            w.remove(e, dce);
            assert(!w.has(e, dce));
        }

        SUBCASE("EntityHandle Interface")
        {
            auto e = w.newEntity();
            e.addParent(dce);
            assert(e.hasParent(dce));
            e.removeParent(dce);
            assert(!e.hasParent(dce));
        }
    }

    TEST_CASE("Component Description"){
        ecs::World w;

        auto e = w.newEntity("Fred");

        CHECK(e.description() == "Fred");
    }

    TEST_CASE("Entity operator bool") {
        ecs::World w;

        auto e = w.newEntity("Fred");

        CHECK(e);
    }

    TEST_CASE("Entity Iterator")
    {
        ecs::World w;

        struct C1{};
        struct C2{};

        auto e = w.newEntity().add<C1>(). add<C2>();
        int j = 0;
        for(auto c: e) {
            j++;
            (void)c;
        }
        CHECK(j == 2);
    }

    TEST_CASE("Delete with name")
    {
        ecs::World w;

        auto e1 = w.newEntity("Name1");
        auto e2 = w.newEntityReplace("Name1");

        CHECK(!e1);
        CHECK(e2);
        CHECK(w.lookup("Name1") == e2);
    }

    TEST_CASE("Remove non-existent component")
    {
        ecs::World w;

        auto e = w.newEntity();
        e.remove<TestComponent>();

        CHECK(!e.has<TestComponent>());
    }

    TEST_CASE("Get from dead entity")
    {
        ecs::World w;

        auto e = w.newEntity();
        e.remove<TestComponent>();

        e.destroy();

        CHECK(e.get<TestComponent>() == nullptr);
        int c = 0;
        e.update<TestComponent>( [&](TestComponent * ){
            c++;
        });
        CHECK(c == 0);
    }

    TEST_CASE("Singletons")
    {
        struct C1 {int x;};
        ecs::World w;

        w.setSingleton<C1>({9});
        w.addSingleton<C1>();

        CHECK(w.getSingleton<C1>()->x == 9);

        w.removeSingleton<C1>();
        CHECK(!w.hasSingleton<C1>());
        w.removeSingleton<C1>();

        CHECK(w.getSingletonUpdate<C1>() == nullptr);
    }

    TEST_CASE("Get Related")
    {
        struct C1 {int x;};
        struct C2: ecs::Relation {

        };

        ecs::World w;

        auto e1 = w.newEntity();
        auto e2 = w.newEntity();
        e2.set<C1>({7});
        e1.set<C2>({{e2.id}});

        CHECK(e1.getRelated<C2, C1>()->x == 7);
        CHECK(e1.getRelatedEntity<C2>() == e2);

        e1.remove<C2>();
        CHECK(e1.getRelated<C2, C1>() == nullptr);
        CHECK(!e1.getRelatedEntity<C2>().isAlive());
    }

    TEST_CASE("World Iterator")
    {
        struct C1 {int x;};
        //struct C2 {int y;};
        //struct C3 {int z;};

        ecs::World w;

        auto e1 = w.newEntity().add<C1>();
        //auto e2 = w.newEntity().add<C2>();
        //auto e3 = w.newEntity().add<C3>();

        for(auto & a: w) {
            auto t = w.getTableForArchetype(a.id);
            if(w.getEntityArchetypeDetails(e1).id == a.id){

                CHECK(t->hasComponent(w.getComponentId<C1>()));

                size_t c = 0;
                for(auto e: *t){
                    (void)e;
                    c++;
                }
                CHECK(c == 1);
            }
        }
    }

    TEST_CASE("Table Names")
    {
        ecs::World w;

        auto e = w.newEntity().add<TestComponent2>().add<TestComponent3>();

        auto aid = w.getEntityArchetypeDetails(e).id;
        auto table = w.getTableForArchetype(aid);
        auto table0 = w.getTableForArchetype(0);

        CHECK(table->description() == "TestComponent2|TestComponent3");
        CHECK(table0->description() == "Empty");
    }

    TEST_CASE("Dynamic Components")
    {
        ecs::World w;

        struct C1 {};
        struct C2 {};

        auto dc = w.newEntity("Fred").add<C1>();

        auto e = w.newEntity().add<C2>();
        dc.setAsParent();

        //auto c = w.createDynamicComponent(dc);

        e.addParent(dc);

        auto q = w.createQuery({ dc.id });
        auto res = w.getResults(q.id);
        CHECK(res.count() == 1);
        res.each<>([&](ecs::EntityHandle eq)
            {
                CHECK(eq == e);
            });

        auto ad = w.getEntityArchetypeDetails(e);
        auto tab = w.getTableForArchetype(ad.id);
        (void)tab;
        e.removeParent(dc);
        CHECK(!e.hasParent(dc));
        dc.removeAsParent();
        //w.removeDynamicComponent(dc);

        CHECK(!w.has<ecs::Component>(dc));

        dc.remove<C1>();
        dc.add<C2>();

        CHECK(dc.has<C2>());
        CHECK(!dc.has<C1>());
        CHECK(!e.has<C1>());
        CHECK(e.has<C2>());
    }

    TEST_CASE("Filters") {
        ecs::World w;

        struct C1 {};
        struct C2 {};
        struct C3 {};

        auto e1 = w.newEntity().add<C1>();
        auto e2 = w.newEntity().add<C2>();
        auto e3 = w.newEntity().add<C1>().add<C2>();

        (void)e2;

        SUBCASE("Empty") {
            auto f = w.createFilter();
            CHECK(f.count() > 3);
        }

        SUBCASE("With Single") {
            auto f = w.createFilter(w.makeComponentList<C1>());
            CHECK(f.count() == 2);
            for(auto tv: f) {
                auto c1 = tv.getColumn<C1>();
                auto c2 = tv.getColumn<C2>();
                for(auto row: tv) {
                    auto c3 = tv.rowComponent<C2>(row);
                    (void)c3;
                }
                (void)c1;
                (void)c2;
            }
        }

        SUBCASE("With multiple") {
            auto f2 = w.createFilter(w.makeComponentList<C1, C2>());
            CHECK(f2.count() == 1);
            std::vector<ecs::entity_t > ev;
            f2.toVector(ev);
            CHECK(ev.size() == 1);
            CHECK(ev[0] == e3.id);
        }

        SUBCASE("Using each")
        {
            auto f = w.createFilter(w.makeComponentList<C1>());
            f.each([](ecs::EntityHandle e){
                CHECK(e.isAlive());
            })  ;
        }

        SUBCASE("With without")
        {
            auto f = w.createFilter(w.makeComponentList<C1>(), w.makeComponentList<C2>());
            CHECK(f.count() == 1);
            std::vector<ecs::entity_t > ev;
            f.toVector(ev);
            CHECK(ev.size() == 1);
            CHECK(ev[0] == e1.id);
        }
    }

    TEST_CASE("Children")
    {
        ecs::World w;

        struct C1 {};
        struct C2 {};

        auto e1 = w.newEntity().add<C1>().setAsParent();
        auto e2 = w.newEntity().add<C2>();

        e2.addParent(e1);

        auto f = e1.getChildren();
        CHECK(f.count() == 1);

        e2.destroy();
        f = e1.getChildren();
        CHECK(f.count() == 0);
        e1.destroy();
    }

    TEST_CASE("Triggers")
    {
        ecs::World w;

        struct C1 {};

        auto e = w.newEntity();
        e.add<ecs::EntityQueue>();

        w.addRemoveTrigger<C1>(e);

        auto e1 = w.newEntity();
        e1.add<C1>();

        e1.remove<C1>();

    }
}
