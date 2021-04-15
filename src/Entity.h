#pragma once

#include <cstdint>

namespace ecs
{
    using entity_t = uint64_t;
    using component_id_t = uint64_t;
    using queryid_t = uint64_t;
    using systemid_t  = uint64_t;
    using streamid_t = uint64_t;

    inline uint32_t version(const entity_t id)
    {
        return ((id & 0xffffffff00000000ull) >> 32);
    }

    inline uint32_t index(const entity_t id)
    {
        return id & 0xffffffffull;
    }

    inline entity_t makeId(uint32_t ix, uint32_t v)
    {
        uint64_t id = v;
        id <<= 32;
        return  id | ix;
    }
}
