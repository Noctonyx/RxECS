#pragma once

#include <functional>
#include <span>
#include <vector>

#include "Entity.h"
#include "World.h"

namespace ecs
{
    class World;

    struct Column
    {
        component_id_t componentId;
        World* world;

        uint32_t componentSize;
        uint16_t alignment;

        std::function<void(void*, size_t, uint32_t)> componentConstructor;
        std::function<void(void*, size_t, uint32_t)> componentDestructor;
        std::function<void(const void*, void*, size_t, uint32_t)> componentCopier;
        std::function<void(void*, void*, size_t, uint32_t)> componentMover;

        std::function<void* (size_t)> componentAllocator;
        std::function<void(void*, size_t)> componentDeallocator;

        std::byte * ptr;
        uint32_t count;
        size_t allocated;

        Column(component_id_t componentId, World * world);
        ~Column();
        void enlargeMemory();
        size_t addMoveEntry(void* srcPtr);
        size_t addCopyEntry(void* srcPtr);
        size_t addEntry();
        void removeEntry(uint32_t row, bool destroy);
        void* getEntry(uint32_t row) const;
        void setEntry(uint32_t row, const void * srcPtr) const;

        void clear();
    };
}
