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
#include "Entity.h"
#include "QueryBuilder.h"

namespace ecs
{
    class World;

    struct SystemBuilder
    {
        systemid_t id;
        queryid_t q = 0;
        component_id_t stream = 0;
        World* world;

        QueryBuilder qb;

        template<class ... TArgs>
        SystemBuilder& withQuery();

        template<class T>
        SystemBuilder& withStream();

        template<class ... TArgs>
        SystemBuilder& without();

        template<class ... TArgs>
        SystemBuilder& with();

        template <class T, class ... U>
        SystemBuilder& withRelation();

        template <class ... TArgs>
        SystemBuilder& withOptional();

        template <class ... TArgs>
        SystemBuilder& withSingleton();

        SystemBuilder& withInheritance(bool inherit);

        template<typename T>
        SystemBuilder& label();
        SystemBuilder& label(const component_id_t label);

        template<typename T>
        SystemBuilder& before();
        SystemBuilder& before(const component_id_t label);

        template<typename T>
        SystemBuilder& after();
        SystemBuilder& after(const component_id_t label);

        SystemBuilder& withSet(entity_t setId);
#if 0
        SystemBuilder& removeSet();
#endif
        template <class ... TArgs>
        SystemBuilder& withRead();

        template <class ... TArgs>
        SystemBuilder& withWrite();

        template <class ... TArgs>
        SystemBuilder& withStreamRead();

        template <class ... TArgs>
        SystemBuilder& withStreamWrite();

        SystemBuilder& inGroup(entity_t group);
        SystemBuilder& inGroup(const char * name);

        SystemBuilder& withJob();

        template <typename ... U, typename Func>
        SystemBuilder & each(Func&& f);

        template <typename Func>
        SystemBuilder& execute(Func&& f);

        template <typename Func>
        SystemBuilder& executeIfNone(Func&& f);

        template <typename U, typename Func>
        SystemBuilder& execute(Func&& f);
    };
}
