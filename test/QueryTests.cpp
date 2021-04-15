#include <random>

#include "doctest.h"
#include "World.h"
#include "QueryResult.h"
#include "TestComponents.h"
#include "QueryImpl.h"

TEST_SUITE("Queries")
{
    TEST_CASE("Query Test 1")
    {
        ecs::World w;

        const auto c = 100;

        std::vector<ecs::entity_t> e;
        for (uint32_t i = 0; i < c; i++) {
            auto x = w.newEntity();
            w.set<TestComponent>(x, {i});
            e.push_back(x);
        }

       // CHECK(TestComponent::c == c);

        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(e), std::end(e), rng);

        for (uint32_t i = 0; i < c; i++) {
            if (!(i % 3)) {
                w.add<TestComponent2>(e[i]);
            }
        }
        auto q1 = w.createQuery<TestComponent>().id;
        {
            auto query_result = w.getResults(q1);
            CHECK(query_result.count() == c);

            for (auto & ch: query_result) {
                for (auto row: ch) {
                    auto ex = ch.entity(row);
                    CHECK(w.has<TestComponent>(ex));
                }
            }
        }

        auto q = w.createQuery<TestComponent>().without<TestComponent2>().id;
        {
            auto r = w.getResults(q);

            for (auto & ch: r) {
                for (auto row: ch) {
                    auto ex = ch.entity(row);
                    CHECK(w.has<TestComponent>(ex));
                    CHECK(!w.has<TestComponent2>(ex));
                }
            }
        }
        w.deleteQuery(q);
    }

    TEST_CASE("New Table adds to query")
    {
        ecs::World w;

        const auto c = 100;
        //TestComponent::c = 0;

        std::vector<ecs::entity_t> e;
        for (uint32_t i = 0; i < c; i++) {
            auto x = w.newEntity();
            w.set<TestComponent>(x, {i});
            e.push_back(x);
        }

        //CHECK(TestComponent::c == c);

        auto q1 = w.createQuery<TestComponent>().id;

        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(e), std::end(e), rng);

        for (uint32_t i = 0; i < c; i++) {
            if (!(i % 3)) {
                w.add<TestComponent2>(e[i]);
            }
        }
        {
            auto r = w.getResults(q1);
            CHECK(r.count() == c);
        }
        w.deleteQuery(q1);
    }

    TEST_CASE("New Table adds to query")
    {
        ecs::World w;

        const auto c = 100;
        //TestComponent::c = 0;

        std::vector<ecs::entity_t> e;
        for (uint32_t i = 0; i < c; i++) {
            auto x = w.newEntity();
            w.set<TestComponent>(x, {i});
            e.push_back(x);
        }

        //CHECK(TestComponent::c == c);

        auto q1 = w.createQuery<TestComponent>().id;

        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(e), std::end(e), rng);

        for (uint32_t i = 0; i < c; i++) {
            if (!(i % 3)) {
                w.add<TestComponent2>(e[i]);
            }
        }
        {
            auto r = w.getResults(q1);
            CHECK(r.count() == c);
        }
        w.deleteQuery(q1);
    }

    TEST_CASE("Query Processing")
    {
        ecs::World w;

        const auto c = 100;
        //TestComponent::c = 0;

        std::vector<ecs::entity_t> e;
        for (uint32_t i = 0; i < c; i++) {
            auto x = w.newEntity();
            w.set<TestComponent>(x, {i});
            e.push_back(x);
        }

        //CHECK(TestComponent::c == c);

        auto q1 = w.createQuery<TestComponent>().id;

        SUBCASE("Query Result Invalidation") {
            auto r = w.getResults(q1);
            for (auto & ch: r) {
                for (auto row = ch.begin(); row != ch.end();) {
                    auto ex = ch.entity(*row);
                    w.add<TestComponent2>(ex);
                    CHECK_THROWS_AS(++row, std::runtime_error);
                    break;
                }
            }
        }

        SUBCASE("Results Bound check") {
            auto r = w.getResults(q1);
            ecs::entity_t e3;
            CHECK_THROWS_AS(e3 = r[0].entity(10000), std::range_error);
        }
        w.deleteQuery(q1);
    }

    TEST_CASE("Query returns components")
    {
        ecs::World w;

        //const auto c = 6;

        auto x = w.newEntity();
        w.set<TestComponent>(x, {2});

        auto q1 = w.createQuery<TestComponent>().id;
        {
            auto r = w.getResults(q1);
            for (auto & chunk: r) {
                for (auto row: chunk) {
                    CHECK(chunk.entity(row) == x);
                    auto c1 = chunk.get<TestComponent>(row);
                    CHECK(c1->x == 2);
                }
            }
        }
        w.deleteQuery(q1);
    }

    TEST_CASE("Query can update components")
    {
        ecs::World w;

        //const auto c = 6;

        auto x = w.newEntity();
        w.set<TestComponent>(x, {2});

        auto q1 = w.createQuery<TestComponent>().id;
        {
            auto r = w.getResults(q1);
            for (auto & chunk: r) {
                for (auto row: chunk) {
                    CHECK(chunk.entity(row) == x);
                    auto c1 = chunk.getUpdate<TestComponent>(row);
                    CHECK(c1->x == 2);
                    c1->x = 3;
                }
            }
        }
        {
            auto r = w.getResults(q1);
            for (auto & chunk: r) {
                for (auto row: chunk) {
                    CHECK(chunk.entity(row) == x);
                    auto c1 = chunk.get<TestComponent>(row);
                    CHECK(c1->x == 3);
                }
            }
        }
        w.deleteQuery(q1);
    }

    TEST_CASE("Query Has iter callback")
    {
        ecs::World w;

        //const auto c = 6;

        auto x = w.newEntity();
        w.set<TestComponent>(x, {2});

        auto q1 = w.createQuery<TestComponent>().id;
        {
            auto r = w.getResults(q1);
            r.iter(
                [](ecs::World * w, ecs::QueryResultChunk & ch)
                {
                    for (auto r: ch) {
                        auto e = ch.entity(r);
                        CHECK(w->has<TestComponent>(e));
                    }
                });
        }
        w.deleteQuery(q1);
    }

    TEST_CASE("Query Has each callback")
    {
        ecs::World world;

        //const auto c = 6;

        auto x = world.newEntity();
        world.set<TestComponent>(x, {2});

        auto q1 = world.createQuery<TestComponent>().id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent>(
                [](ecs::World * w, ecs::entity_t e, const TestComponent * c)
                {
                    CHECK(c->x == 2);
                    CHECK(w->has<TestComponent>(e));
                });
        }
        world.deleteQuery(q1);
    }

    TEST_CASE("Query performance")
    {
        ecs::World world;
        const auto c = 100000;

        std::vector<ecs::entity_t> e;
        for (uint32_t i = 0; i < c; i++) {
            auto x = world.newEntity();
            world.set<TestComponent>(x, {i});
            e.push_back(x);
        }

        auto q1 = world.createQuery<TestComponent>().id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent>(
                [](ecs::World *, ecs::entity_t, const TestComponent * c1)
                {
                    auto j = c1->x;
                    (void) j;
                });
        }
        world.deleteQuery(q1);
    }

    TEST_CASE("Query with optional")
    {
        ecs::World world;

        //const auto c = 6;

        auto x = world.newEntity();
        world.set<TestComponent>(x, {2});

        auto y = world.newEntity();
        //world.set<TestRelation>(x, { y });
        world.set<TestComponent>(y, {3});
        world.set<TestComponent2>(y, {7, ""});

        int c1 = 0;
        int c2 = 0;

        auto q1 = world.createQuery<TestComponent>().id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2>(
                [&c1, &c2](
                ecs::World *,
                ecs::entity_t,
                const TestComponent *,
                const TestComponent2 * p)
                {
                    c2++;
                    if (p != nullptr) {
                        c1++;
                    }
                });
        }
        CHECK(c1 == 1);
        CHECK(c2 == 2);
        c1 = 0;
        c2 = 0;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2>(
                [&c1, &c2](
                ecs::World *,
                ecs::entity_t,
                const TestComponent *,
                TestComponent2 * p)
                {
                    c2++;
                    if (p != nullptr) {
                        c1++;
                    }
                });
        }
        CHECK(c1 == 1);
        CHECK(c2 == 2);

        world.deleteQuery(q1);
    }

    TEST_CASE("Query with relation")
    {
        ecs::World world;

        //const auto c = 6;

        auto x = world.newEntity();
        world.set<TestComponent>(x, {2});

        auto y = world.newEntity();
        world.set<TestRelation>(x, {{y}});
        world.set<TestComponent2>(y, {7, ""});

        auto q1 = world.createQuery<TestComponent>()
                       .withRelation<TestRelation, TestComponent2>().id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2, TestRelation>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                const TestComponent * c,
                const TestComponent2 * p,
                const TestRelation * r)
                {
                    CHECK(w->isAlive(r->entity));
                    CHECK(c->x == 2);
                    CHECK(p->y == 7);
                    CHECK(w->has<TestComponent>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }

        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2, TestRelation>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                const TestComponent * c,
                TestComponent2 * p,
                TestRelation * r)
                {
                    CHECK(w->isAlive(r->entity));
                    CHECK(c->x == 2);
                    CHECK(p->y == 7);
                    CHECK(w->has<TestComponent>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }

        world.deleteQuery(q1);
    }

    TEST_CASE("Query with dead relation")
    {
        ecs::World world;

        //const auto c = 6;

        auto x = world.newEntity();
        world.set<TestComponent>(x, {2});

        auto y = world.newEntity();
        world.set<TestRelation>(x, {{y}});
        world.set<TestComponent2>(y, {7, ""});

        world.destroy(y);

        auto q1 = world.createQuery<TestComponent>()
                       .withRelation<TestRelation, TestComponent2>().id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2, TestRelation>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                const TestComponent * c,
                const TestComponent2 * p,
                const TestRelation *)
                {
                    CHECK(p == nullptr);
                    CHECK(c->x == 2);
                    CHECK(w->has<TestComponent>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }
        world.deleteQuery(q1);
    }

    TEST_CASE("Query with singleton")
    {
        ecs::World world;

        //const auto c = 6;

        auto x = world.newEntity();
        world.set<TestComponent>(x, {2});
        world.setSingleton<TestComponent2>({7, ""});

        auto q1 = world
                  .createQuery<TestComponent>()
                  .withSingleton<TestComponent2>().id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                const TestComponent * c,
                TestComponent2 * p)
                {
                    CHECK(p != nullptr);
                    CHECK(p->y == 7);
                    CHECK(c->x == 2);
                    CHECK(w->has<TestComponent>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }

        world.removeSingleton<TestComponent2>();

        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                const TestComponent * c,
                const TestComponent2 * p)
                {
                    CHECK(p == nullptr);
                    CHECK(c->x == 2);
                    CHECK(w->has<TestComponent>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }
        world.deleteQuery(q1);
    }

    TEST_CASE("Query with inheritance")
    {
        ecs::World world;

        //const auto c = 6;

        auto x = world.newEntity();
        auto y = world.newEntity();
        auto z = world.newEntity();

        world.set<TestComponent>(x, {1});
        world.set<TestComponent2>(y, {.y = 5});
        world.set<TestComponent3>(z, {.w = 11});

        world.set<ecs::InstanceOf>(x, {{y}});
        world.set<ecs::InstanceOf>(y, {{z}});

        auto q1 = world
                  .createQuery<TestComponent>()
                  .withInheritance(true).id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                TestComponent * c,
                const TestComponent2 * p)
                {
                    CHECK(p != nullptr);
                    CHECK(p->y == 5);
                    CHECK(c->x == 1);
                    CHECK(w->has<TestComponent>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }
        q1 = world
             .createQuery<TestComponent>()
             .withInheritance(true).id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                TestComponent * c,
                TestComponent2 * p)
                {
                    CHECK(p == nullptr);
                    CHECK(c->x == 1);
                    CHECK(w->has<TestComponent>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }

        q1 = world
             .createQuery<TestComponent>()
             .withInheritance(false).id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent, TestComponent2>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                TestComponent * c,
                const TestComponent2 * p)
                {
                    CHECK(p == nullptr);
                    CHECK(c->x == 1);
                    CHECK(w->has<TestComponent>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }

        q1 = world
             .createQuery<TestComponent3>()
             .withInheritance(true).id;
        {
            auto query_result = world.getResults(q1);
            query_result.each<TestComponent3, TestComponent2>(
                [](
                ecs::World * w,
                ecs::entity_t e,
                TestComponent3 * c,
                const TestComponent2 * p)
                {
                    CHECK(p == nullptr);
                    CHECK(c->w == 11);
                    CHECK(w->has<TestComponent3>(e));
                    CHECK(!w->has<TestComponent2>(e));
                });
        }

        world.deleteQuery(q1);
    }
}