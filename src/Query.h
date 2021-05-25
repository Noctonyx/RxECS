#pragma once

#include <iterator>
#include <vector>
#include "Entity.h"
#include "World.h"

namespace ecs
{
    struct Query
    {
        std::set<component_id_t> with;
        std::set<component_id_t> without;
        std::set<component_id_t> optional{};
        std::set<component_id_t> singleton{};
        std::set<std::pair<component_id_t, std::set<component_id_t>>> relations{};
        std::unordered_map<component_id_t, component_id_t> relationLookup;

        bool inheritance = false;
        bool thread = false;

        std::vector<Table *> tables{};

        bool interestedInArchetype(Archetype & ad)
        {
            std::vector<component_id_t> with_overlap;
            std::vector<component_id_t> without_overlap;

            std::ranges::set_intersection(ad.components, with, std::back_inserter(with_overlap));

            if (with_overlap.size() < with.size()) {
                return false;
            }

            std::ranges::set_intersection(ad.components, without,
                                          std::back_inserter(without_overlap));
            if (without_overlap.empty()) {
                return true;
            }

            return false;
        }

        void recalculateQuery(World * world)
        {
            tables.clear();

            for (auto & i: *world) {
                if (interestedInArchetype(i)) {
                    tables.push_back(world->getTableForArchetype(i.id));
                }
            }

            relationLookup.clear();

            for(auto & [c, v]: relations) {
                for(auto vx: v) {
                    relationLookup[vx] = c;
                }
            }
        }
    };
}
