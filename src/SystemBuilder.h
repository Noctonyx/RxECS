#pragma once

#include <string>
#include "Entity.h"

namespace ecs
{
    class World;

    struct SystemBuilder
    {
        systemid_t id;
        queryid_t q;
        World* world;

        template<class ... TArgs>
        SystemBuilder& withQuery();

        template<class ... TArgs>
        SystemBuilder& without();

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
        SystemBuilder& removeSet();

        template <typename ... U, typename Func>
        SystemBuilder & each(Func&& f);

        template <typename Func>
        SystemBuilder& execute(Func&& f);
    };
}
