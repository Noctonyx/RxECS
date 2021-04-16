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

        auto s = world.createStream<TestComponent2>();
        world.deleteStream(s);

        s = world.createStream<TestComponent3>();

        auto str = world.getStream(s);

        for (uint32_t i = 0; i < 10; i++) {
            str->add<TestComponent3>({.w = i});
        }

        struct Label1 {};
        struct Label2 {};

        int c1 = 0;
        int c2 = 0;

        world.createSystem()
             .withStream(s)
             .label<Label1>()
             .execute<TestComponent3>([&c1](ecs::World *, const TestComponent3 *)
             {
                 c1++;
                 if (c1 % 2)
                     return true;
                 return false;
             });

        world.createSystem()
             .withStream(s)
             .after<Label1>()
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
