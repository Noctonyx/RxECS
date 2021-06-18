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

#include <cassert>
#include <string>
#include <memory>
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

        bool isRelation;

        std::function<void* (size_t)> allocator;
        std::function<void(void *,size_t)> deallocator;
        //std::allocator<Component> alloc;

        std::vector<entity_t> onAdds{};
        std::vector<entity_t> onUpdates{};
        std::vector<entity_t> onRemove{};
        //std::vector<entity_t> onDelete{};
    };

    template<class T>
    void * componentAllocator(size_t n)
    {
        std::allocator<T> a;
        return a.allocate(n);
        //return std::allocator_traits<std::allocator<T>>::allocate(n);
    }

    template<class T>
    void componentDeallocator(void * p, size_t n)
    {
        std::allocator<T> a;

        return a.deallocate(static_cast<T *>(p), n);
        //return std::allocator_traits<std::allocator<T>>::deallocate(p, n);
    }

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
            std::construct_at<T>(&component);
            //new(&component) T();
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
            std::destroy_at<T>(&component);
            //component.~T();
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
    };

    struct Relation
    {
        entity_t entity;
    };

    struct InstanceOf : Relation
    {
    };

    struct DynamicComponent
    {
        
    };
}
