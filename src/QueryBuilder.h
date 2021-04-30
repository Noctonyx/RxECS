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

        template <class ... TArgs>
        QueryBuilder& with();

        template <class T, class ... U>
        QueryBuilder & withRelation();

        QueryBuilder& withPrefabs();

        template <class ... TArgs>
        QueryBuilder& withOptional();

        template <class ... TArgs>
        QueryBuilder& withSingleton();

        QueryBuilder& withInheritance(bool inherit);
    };
}
