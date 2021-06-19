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

#include <algorithm>
#include "EntityQueue.h"
#include "EntityHandle.h"
#include "EntityQueueHandle.h"
#include "World.h"

namespace ecs
{
    //std::mutex EntityQueue::mutex;

    void EntityQueue::add(entity_t id)
    {
        std::lock_guard g(mutex);

        entries.push_back({id, false});
    }
#if 0
    void EntityQueue::remove(entity_t id)
    {
        std::lock_guard g(mutex);

        auto it = std::find_if(
            entries.begin(), entries.end(), [=](auto i) {
                return i.entity == id;
            }
        );

        if (it != entries.end()) {
            entries.erase(it);
        }
    }
#endif
    void EntityQueue::each(std::function<bool(EntityHandle)> && f)
    {
        std::lock_guard g(mutex);

        for (auto & e: entries) {
            if (!e.removed) {
                auto r = f(EntityHandle{e.entity, world});
                if (r) {
                    e.removed = r;
                }
            }
        }

        if (!entries.empty()) {
            decltype(entries) clean;
            std::copy_if(
                entries.begin(), entries.end(), std::back_inserter(clean), [](auto i) {
                    return !i.removed;
                }
            );
            entries = std::move(clean);
        }
    }

    EntityQueue::EntityQueue(World * world)
        : world(world)
    {}

    void EntityQueueHandle::each(std::function<bool(EntityHandle)> && f)
    {
        auto eq = world->getEntityQueue(id);
        if (eq) {
            eq->each(std::move(f));
        }
    }

    void EntityQueueHandle::destroy() const
    {
        world->destroyEntityQueue(id);
    }

    void EntityQueueHandle::post(entity_t entityId) const
    {
        auto eq = world->getEntityQueue(id);
        if (eq) {
            eq->add(entityId);
        }
    }
}