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
        std::set<component_id_t> singleton{};
        std::set<std::pair<component_id_t, std::set<component_id_t>>> relations{};
        std::unordered_map<component_id_t, component_id_t> relationLookup;

        std::set<component_id_t> filterComponents;

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
