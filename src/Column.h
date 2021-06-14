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
#include <span>
#include <vector>

#include "Entity.h"
//#include "World.h"

namespace ecs
{
    class World;

    struct Column
    {
        component_id_t componentId;
        World * world;

        uint32_t componentSize;
        uint16_t alignment;

        std::function<void(void *, size_t, uint32_t)> componentConstructor;
        std::function<void(void *, size_t, uint32_t)> componentDestructor;
        std::function<void(const void *, void *, size_t, uint32_t)> componentCopier;
        std::function<void(void *, void *, size_t, uint32_t)> componentMover;

        std::function<void *(size_t)> componentAllocator;
        std::function<void(void *, size_t)> componentDeallocator;

        std::byte * ptr;
        uint32_t count;
        size_t allocated;

        Column(component_id_t componentId, World * world);
        ~Column();
        void enlargeMemory();
        size_t addMoveEntry(void * srcPtr);
        size_t addCopyEntry(void * srcPtr);
        size_t addEntry();
        void removeEntry(uint32_t row, bool destroy);
        void * getEntry(uint32_t row) const;
        void setEntry(uint32_t row, const void * srcPtr) const;

        void clear();

        template<class T>
        std::span<const T> getComponentData() const;
    };

    template<class T>
    std::span<const T> Column::getComponentData() const
    {
        assert(world.getComponentId<T>() == componentId);
        const T * x = reinterpret_cast<const T *>(ptr);
        return std::span<const T>(x, count);
    }
}
