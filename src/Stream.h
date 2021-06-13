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

#pragma once

#include <functional>
#include <mutex>
#include <span>
#include <vector>

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
