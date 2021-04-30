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

        SystemBuilder& inGroup(entity_t group);
        SystemBuilder& inGroup(const char * name);

        SystemBuilder& withJob();

        template <typename ... U, typename Func>
        SystemBuilder & each(Func&& f);

        template <typename Func>
        SystemBuilder& execute(Func&& f);

        template <typename U, typename Func>
        SystemBuilder& execute(Func&& f);
    };
}
