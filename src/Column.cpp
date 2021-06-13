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

#include <cassert>
#include "Column.h"
#include "World.h"

namespace ecs
{
    Column::Column(component_id_t componentId, World * world)
        : componentId(componentId)
        , world(world)
    {
        const auto cd = world->getComponentDetails(componentId);

        componentSize = cd->size;
        alignment = cd->alignment;
        componentConstructor = cd->componentConstructor;
        componentDestructor = cd->componentDestructor;
        componentCopier = cd->componentCopier;
        componentMover = cd->componentMover;
        componentAllocator = cd->allocator;
        componentDeallocator = cd->deallocator;

        auto const initial_size = 10;

        ptr = static_cast<std::byte *>(cd->allocator(initial_size));
        count = 0;
        allocated = initial_size;
    }

    Column::~Column()
    {
        componentDestructor(ptr, componentSize, count);
        componentDeallocator(ptr, allocated);
        ptr = nullptr;
    }

    void Column::enlargeMemory()
    {
        const size_t new_size = allocated * 3 / 2 + 1;
        const auto new_ptr = componentAllocator(new_size);

        componentMover(ptr, new_ptr, componentSize, count);
        componentDestructor(ptr, componentSize, count);
        componentDeallocator(ptr, allocated);

        allocated = new_size;
        ptr = static_cast<std::byte *>(new_ptr);
    }

    size_t Column::addMoveEntry(void * srcPtr)
    {
        assert(count <= allocated);

        if (count == allocated) {
            enlargeMemory();
        }

        void * dest_ptr = ptr + count * componentSize;
        componentMover(srcPtr, dest_ptr, componentSize, 1);
        return count++;
    }

    size_t Column::addCopyEntry(void * srcPtr)
    {
        assert(count <= allocated);

        if (count == allocated) {
            enlargeMemory();
        }

        void * dest_ptr = ptr + count * componentSize;
        componentCopier(srcPtr, dest_ptr, componentSize, 1);
        return count++;
    }

    size_t Column::addEntry()
    {
        assert(count <= allocated);

        if (count == allocated) {
            enlargeMemory();
        }

        void * dest_ptr = ptr + count * componentSize;

        componentConstructor(dest_ptr, componentSize, 1);
        return count++;
    }

    void Column::removeEntry(const uint32_t row, const bool destroy)
    {
        assert(row < count);
        if (row == count - 1) {
            void * dest_ptr = ptr + row * componentSize;
            if (destroy) {
                componentDestructor(dest_ptr, componentSize, 1);
            }
            count--;
            return;
        }

        void * dest_ptr = ptr + row * componentSize;
        void * src_ptr = ptr + (count - 1) * componentSize;

        if (destroy) {
            componentDestructor(dest_ptr, componentSize, 1);
        }
        componentMover(src_ptr, dest_ptr, componentSize, 1);
        componentDestructor(src_ptr, componentSize, 1);
        count--;
    }

    void Column::clear()
    {
        componentDestructor(ptr, componentSize, count);
        count = 0;
    }

    void * Column::getEntry(const uint32_t row) const
    {
        assert(row < count);

        void * dest_ptr = ptr + row * componentSize;
        return dest_ptr;
    }

    void Column::setEntry(const uint32_t row, const void * srcPtr) const
    {
        assert(row < count);

        void * dest_ptr = ptr + row * componentSize;
        componentCopier(srcPtr, dest_ptr, componentSize, 1);
    }
}
