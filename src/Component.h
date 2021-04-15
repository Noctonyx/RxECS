#pragma once

#include <cassert>
#include <string_view>
#include <functional>
#include <span>

#include "Entity.h"

namespace ecs
{
    struct Component
    {
        std::string name;
        uint32_t size;
        uint16_t alignment;

        std::function<void(void *, size_t, uint32_t)> componentConstructor;
        std::function<void(void *, size_t, uint32_t)> componentDestructor;
        std::function<void(const void *, void *, size_t, uint32_t)> componentCopier;
        std::function<void(void *, void *, size_t, uint32_t)> componentMover;
    };

    template <typename T>
    void componentConstructor(
        void * ptr,
        const size_t size,
        const uint32_t count
    )
    {
        (void) size;

        assert(size == sizeof(T));

        std::span<T> components(static_cast<T *>(ptr), count);
        for (auto & component: components) {
            new(&component) T();
        }
    }

    template <typename T>
    void componentDestructor(
        void * ptr,
        const size_t size,
        const uint32_t count
    )
    {
        (void) size;
        assert(size == sizeof(T));
        std::span<T> components(static_cast<T *>(ptr), count);
        for (auto & component: components) {
            component.~T();
        }
    }

    template <typename T>
    void componentCopy(
        const void * src,
        void * dest,
        const size_t size,
        const uint32_t count
    )
    {
        (void) size;
        assert(size == sizeof(T));

        std::span<T> dest_components(static_cast<T *>(dest), count);
        std::span<const T> source_components(static_cast<const T *>(src), count);

        for (uint32_t i = 0; i < dest_components.size(); i++) {
            dest_components[i] = source_components[i];
        }
    }

    template <typename T>
    void componentMove(
        void * src,
        void * dest,
        const size_t size,
        const uint32_t count
    )
    {
        (void) size;
        assert(size == sizeof(T));

        std::span<T> dest_components(static_cast<T *>(dest), count);
        std::span<T> source_components(static_cast<T *>(src), count);

        for (uint32_t i = 0; i < dest_components.size(); i++) {
            new(&(dest_components[i])) T(std::move(source_components[i]));
        }
    }

    struct Name
    {
        std::string name;
        //const char* name;
    };

    struct Relation
    {
        entity_t entity;
    };

    struct InstanceOf : Relation
    {
    };
}