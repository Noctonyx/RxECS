#pragma once
#include <set>
#include <vector>

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

        ArchetypeTransition & addComponentToArchetype(uint16_t at, component_id_t componentId)
        {
            if (!addCache.contains({ at, componentId })) {

                ArchetypeTransition trans;
                trans.from_at = at;

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
                trans.to_at = newId;
                trans.addComponents.push_back(componentId);
                trans.preserveComponents.resize(oldAt.components.size());
                std::copy(oldAt.components.begin(), oldAt.components.end(),
                          trans.preserveComponents.begin());

                addCache.insert_or_assign({ at, componentId }, std::move(trans));
            }
            return addCache[{at, componentId}];
        }

        ArchetypeTransition & removeComponentFromArchetype(uint16_t at, component_id_t componentId)
        {
            if (!removeCache.contains({at, componentId})) {

                ArchetypeTransition trans;
                trans.from_at = at;

                auto old_archetype = archetypes[at];
                Archetype new_archetype = old_archetype;
                new_archetype.components.erase(componentId);
                Hasher h;
                for (auto & v: new_archetype.components) {
                    h.u64(v);
                }
                new_archetype.hash_value = h.get();

                uint16_t newid;

                if (archetypeMap.find(new_archetype.hash_value) != archetypeMap.end()) {
                    newid = archetypeMap[new_archetype.hash_value];
                    trans.to_at = newid;
                } else {
                    auto ix = static_cast<uint16_t>(archetypes.size());
                    //emptyArchetype = ix;
                    archetypeMap.emplace(new_archetype.hash_value, ix);
                    new_archetype.id = ix;
                    archetypes.push_back(new_archetype);

                    //removeCache.insert_or_assign({at, componentId}, ix);

                    newid = ix;
                }
                trans.to_at = newid;
                trans.removeComponents.push_back(componentId);
                trans.preserveComponents.resize(new_archetype.components.size());
                std::copy(new_archetype.components.begin(), new_archetype.components.end(),
                          trans.preserveComponents.begin());

                removeCache.insert_or_assign({at, componentId}, std::move(trans));
            }

            return removeCache[{at, componentId}];
        }

        Archetype & getArchetypeDetails(uint16_t id)
        {
            return archetypes[id];
        }

        std::vector<Archetype> archetypes{};
        robin_hood::unordered_map<Hash, uint16_t> archetypeMap;

        robin_hood::unordered_map<robin_hood::pair<uint16_t, component_id_t>, ArchetypeTransition>
        addCache;
        robin_hood::unordered_map<robin_hood::pair<uint16_t, component_id_t>, ArchetypeTransition>
        removeCache;

        uint16_t emptyArchetype;
    };
}
