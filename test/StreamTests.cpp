#include <random>

#include "doctest.h"
#include "RxECS.h"
#include "TestComponents.h"

TEST_SUITE("Streams")
{
    TEST_CASE("Basic")
    {
        CHECK(_CrtCheckMemory());

        ecs::World world;

        auto s = world.getStream<TestComponent2>();
        world.newEntity("Group:1").set<ecs::SystemGroup>({1, false, 0.f, 0.f});

        s = world.getStream<TestComponent3>();

        for (uint32_t i = 0; i < 10; i++) {
            s->add<TestComponent3>({.w = i});
        }

        int c1 = 0;
        int c2 = 0;

        world.createSystem("Stream1")
             .withStream<TestComponent3>()
             .inGroup("Group:1")
             .execute<TestComponent3>([&c1](ecs::World *, const TestComponent3 *)
             {
                 c1++;
                 if (c1 % 2)
                     return true;
                 return false;
             });

        world.createSystem("Stream3")
             .withStream<TestComponent3>()        
             .inGroup("Group:1")
             .execute<TestComponent3>([&c2](ecs::World *, const TestComponent3 *)
             {
                 c2++;
                 return true;
             });

        world.step(0.01f);
        CHECK(c1 == 10);
        CHECK(c2 == 5);
        world.step(0.01f);
        CHECK(c1 == 10);
        CHECK(c2 == 5);

        CHECK(_CrtCheckMemory());
    }
}
