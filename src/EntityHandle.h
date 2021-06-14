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
#include <string>
#include <vector>

#include "ComponentIterator.h"
#include "Entity.h"

namespace ecs
{
    class World;
    struct Filter;

    struct EntityHandle
    {
        entity_t id;
        World * world;

        template <typename T>
        EntityHandle & set(const T & v);

        template <typename T>
        EntityHandle & add();
        //EntityHandle & addDynamic(component_id_t id);
        EntityHandle & addParent(component_id_t id);

        EntityHandle & setAsParent();
        EntityHandle & removeAsParent();

        template <typename T>
        T * addAndUpdate();

        template <typename T>
        EntityHandle & addDeferred();

        template <typename T>
        EntityHandle & removeDeferred();

        EntityHandle & destroyDeferred();

        template <typename T>
        EntityHandle & setDeferred(const T & value);

        template <typename T>
        bool has();
        bool hasParent(component_id_t parentId) const;

        template <typename T>
        T * getUpdate();

        template <typename T>
        const T * get(bool inherit = false);

        template <typename U, typename T>
        const T * getRelated();

        template <typename T>
        EntityHandle getRelatedEntity();

        template <typename T>
        EntityHandle & remove();
        EntityHandle & removeParent(component_id_t parentId);

        bool isAlive() const;
        EntityHandle & destroy();

        EntityHandle instantiate(const char * name = nullptr);

        EntityHandle getHandle(entity_t o)
        {
            return EntityHandle{o, world};
        }

        bool operator==(const EntityHandle & o) const
        {
            return o.id == id && o.world == world;
        }

        World * getWorld() const
        {
            return world;
        }

        operator bool() const;
        operator entity_t() const;

        std::string description() const;

        ComponentIterator begin();
        ComponentIterator end();

        Filter getChildren(const std::vector<component_id_t> & with = {}, const std::vector<component_id_t> & without = {} );
    };
}
