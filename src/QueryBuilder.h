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
#include "Entity.h"

namespace ecs
{
    class World;

    struct QueryBuilder
    {
        queryid_t id;
        World * world;

        template <class ... TArgs>
        QueryBuilder & without();

#if 0
        template <class ... TArgs>
        QueryBuilder& with();
#endif
        QueryBuilder& withParent(component_id_t id);

        template <class T, class ... U>
        QueryBuilder & withRelation();

        QueryBuilder& withPrefabs();

        template <class ... TArgs>
        QueryBuilder& withOptional();

        template <class ... TArgs>
        QueryBuilder& withSingleton();

        QueryBuilder& withInheritance(bool inherit);

        QueryBuilder& withJob();
    };
}
