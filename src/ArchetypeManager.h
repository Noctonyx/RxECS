#pragma once
#include <set>
#include <vector>
#include <cassert>

#include "Entity.h"
#include "Hasher.h"
#include "robin_hood.h"

namespace robin_hood
{
    template <typename T, typename U>
    struct hash<robin_hood::pair<T, U>>
    {
        size_t operator()(robin_hood::pair<T, U> const & p) const noexcept
        {
            auto a = hash<T>{}(p.first);
            auto b = hash<U>{}(p.second);
            return hash_combine(a, b);
        }

        static size_t hash_combine(size_t seed, size_t v) noexcept
        {
            // see https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
            seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}

namespace ecs
{
    class World;

    struct Archetype
    {
        std::set<component_id_t> components;
        Hash hash_value;
        uint16_t id;
        //World* world;

        void generateHash()
        {
            Hasher h;
            for (auto & v: components) {
                h.u64(v);
            }
            hash_value = h.get();
        }
    };

    struct ArchetypeTransition
    {
        uint16_t from_at{};
        uint16_t to_at{};

        std::vector<component_id_t> removeComponents{};
        std::vector<component_id_t> addComponents{};
        std::vector<component_id_t> preserveComponents{};
    };

    struct ArchetypeManager
    {
        ArchetypeManager()
        {
            Archetype e;
            e.generateHash();

            auto ix = archetypes.size();
            e.id = static_cast<uint16_t>(ix);
            emptyArchetype = static_cast<uint16_t>(ix);
            archetypeMap.emplace(e.hash_value, emptyArchetype);
            archetypes.push_back(e);
        }

        ArchetypeTransition startTransition(uint16_t at)
        {
            ArchetypeTransition trans;
            trans.from_at = at;
            trans.to_at = at;
            auto a = archetypes[at];
            for (auto s: a.components) {
                trans.preserveComponents.push_back(s);
            }

            return trans;
        }

        void createRemoveCache(uint16_t at, component_id_t componentId)
        {
            auto old_archetype = archetypes[at];
            Archetype new_archetype = old_archetype;

            assert(new_archetype.components.find(componentId) != new_archetype.components.end());

            new_archetype.components.erase(componentId);
            Hasher h;
            for (auto & v: new_archetype.components) {
                h.u64(v);
            }
            new_archetype.hash_value = h.get();

            uint16_t newid;

            if (archetypeMap.find(new_archetype.hash_value) != archetypeMap.end()) {
                newid = archetypeMap[new_archetype.hash_value];
            } else {
                auto ix = static_cast<uint16_t>(archetypes.size());
                archetypeMap.emplace(new_archetype.hash_value, ix);
                new_archetype.id = ix;
                archetypes.push_back(new_archetype);

                newid = ix;
            }

            removeCache.insert_or_assign({at, componentId}, newid);
        }

        void createAddCache(uint16_t at, component_id_t componentId)
        {
            auto oldAt = archetypes[at];
            Archetype newA = oldAt;
            newA.components.insert(componentId);
            Hasher h;
            for (auto& v : newA.components) {
                h.u64(v);
            }

            uint16_t newId;

            newA.hash_value = h.get();
            if (archetypeMap.contains(newA.hash_value)) {
                newId = archetypeMap[newA.hash_value];
            }
            else {

                auto ix = static_cast<uint16_t>(archetypes.size());
                newA.id = ix;
                //emptyArchetype = ix;
                archetypeMap.emplace(newA.hash_value, ix);
                archetypes.push_back(newA);

                newId = ix;
            }

            addCache.insert_or_assign({ at, componentId }, newId);
        }

        void removeComponentFromArchetype(component_id_t componentId, ArchetypeTransition & trans)
        {
            if (!removeCache.contains({trans.to_at, componentId})) {

                createRemoveCache(trans.to_at, componentId);
            }

            trans.to_at = removeCache[{trans.to_at, componentId}];

            auto j = std::find(trans.preserveComponents.begin(), trans.preserveComponents.end(), componentId);

            if (j != trans.preserveComponents.end()) {
                trans.preserveComponents.erase(j);
                trans.removeComponents.push_back(componentId);
            } else {
                auto k = std::find(trans.addComponents.begin(), trans.addComponents.end(), componentId);
                assert(k != trans.addComponents.end());
                trans.addComponents.erase(k);
            }
        }

        void addComponentToArchetype(component_id_t componentId, ArchetypeTransition& trans)
        {
            if (!addCache.contains({ trans.to_at, componentId })) {

                createAddCache(trans.to_at, componentId);
            }

            trans.to_at = addCache[{trans.to_at, componentId}];

            auto j = std::find(trans.removeComponents.begin(), trans.removeComponents.end(), componentId);

            if (j != trans.removeComponents.end()) {
                trans.removeComponents.erase(j);
            }
            else {
                auto k = std::find(trans.preserveComponents.begin(), trans.preserveComponents.end(), componentId);
                (void)k;
                assert(k == trans.preserveComponents.end());
                trans.addComponents.push_back(componentId);
            }
        }

        Archetype & getArchetypeDetails(uint16_t id)
        {
            return archetypes[id];
        }

        std::vector<Archetype> archetypes{};
        robin_hood::unordered_map<Hash, uint16_t> archetypeMap;

        robin_hood::unordered_map<robin_hood::pair<uint16_t, component_id_t>, uint16_t> addCache;
        robin_hood::unordered_map<robin_hood::pair<uint16_t, component_id_t>, uint16_t> removeCache;

        uint16_t emptyArchetype;
    };
}
