#pragma once

#include <functional>
#include <mutex>
#include <span>
#include <vector>

#include "Column.h"
#include "Entity.h"
#include "World.h"

namespace ecs
{
    struct Stream
    {
        component_id_t componentId;
        World * world;

        Column * column;
        std::vector<bool> active;

        std::mutex mut;

        Stream(component_id_t componentId, World * world);
        ~Stream();

        template <typename T>
        void add(T && value);

        void clear();

        template <typename U, typename Func>
        void each(Func && f);
    };

    template <typename T>
    void Stream::add(T && value)
    {
        std::lock_guard guard(mut);

        auto c = column->count;
        size_t ix = active.size();

        (void) ix;
        (void) c;
        assert(ix == c);
        column->addMoveEntry(&value);
        active.push_back(true);
    }

    template <typename U, typename Func>
    void Stream::each(Func && f)
    {
        std::lock_guard guard(mut);

        //static_assert(std::is_const_v<U>, "Parameter must be const");
        std::tuple<World *, const U *> result;

        std::get<0>(result) = world;
        uint32_t ix = 0;
        for (auto ac: active) {
            if (ac) {
                void * ptr = column->getEntry(ix);
                std::get<1>(result) = static_cast<const U *>(ptr);
                if (std::apply(f, result)) {
                    active[ix] = false;
                }
            }
            ix++;
        }
    }

    struct StreamComponent
    {
        Stream* ptr;
    };
}
