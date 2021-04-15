#pragma once

struct TestComponent
{
    //static int c;
    uint32_t x;
    /*
    TestComponent()
    {
        c++;
    }

    TestComponent(uint32_t a)
        : x(a)
    {
        c++;
    }

    ~TestComponent()
    {
        c--;
    }

    TestComponent(const TestComponent & other)
        : x(other.x)
    {
        c++;
    }

    TestComponent(TestComponent && other) noexcept
        : x(other.x)
    {
        c++;
    }

    TestComponent & operator=(const TestComponent & other)
    {
        if (this == &other)
            return *this;
        x = other.x;
        return *this;
    }

    TestComponent & operator=(TestComponent && other) noexcept
    {
        if (this == &other)
            return *this;
        x = other.x;
        return *this;
    }
    */
};



struct TestComponent2
{
    uint32_t y;
    std::string z;
};

struct TestComponent3
{
    uint32_t w;
};

struct TestTag
{
    
};

struct TestRelation: ecs::Relation
{
    
};