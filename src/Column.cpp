#include "Column.h"

namespace ecs
{
    Column::Column(component_id_t componentId, World * world)
        : componentId(componentId)
        , world(world)
    {
        auto cd = world->getComponentDetails(componentId);
        componentSize = cd->size;
        alignment = cd->alignment;
        componentConstructor = cd->componentConstructor;
        componentDestructor = cd->componentDestructor;
        componentCopier = cd->componentCopier;
        componentMover = cd->componentMover;

        auto const initial_size = 100;

        ptr = new std::byte[initial_size * cd->size];
        count = 0;
        allocated = initial_size;
    }

    Column::~Column()
    {
        componentDestructor(ptr, componentSize, count);
        delete ptr;
        ptr = nullptr;
    }

    void Column::enlargeMemory()
    {
        const size_t new_size = allocated * 3 / 2 + 1;
        const auto new_ptr = new std::byte[new_size * componentSize];

        componentMover(ptr, new_ptr, componentSize, count);
        componentDestructor(ptr, componentSize, count);
        allocated = new_size;
        delete ptr;
        ptr = new_ptr;
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
