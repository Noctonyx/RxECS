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

//
// Created by shane on 18/06/2021.
//

#ifndef INDUSTRONAUT_ENTITYQUEUE_H
#define INDUSTRONAUT_ENTITYQUEUE_H

#include <functional>
#include <mutex>
#include "Entity.h"
#include "EntityHandle.h"

namespace ecs {
    class World;

    struct EntityQueue {

        struct Entry {
            entity_t entity;
            bool removed;
        };

        World * world;
        std::vector<Entry> entries{};
        std::mutex mutex{};

        void add(entity_t id);
#if 0
        void remove(entity_t id);
#endif
        void each(std::function<bool(EntityHandle)> && f);

        EntityQueue(World * world);
    };
}
#endif //INDUSTRONAUT_ENTITYQUEUE_H
