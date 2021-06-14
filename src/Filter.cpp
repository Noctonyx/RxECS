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

//
// Created by shane on 14/06/2021.
//

#include "Filter.h"

#include <utility>

namespace ecs
{
    void Filter::each(std::function<void(EntityHandle)> && f) const
    {
        for (auto tv: tableViews) {
            for (auto r: tv) {
                f(EntityHandle{tv.entity(r), world});
            }
        }
    }

    size_t Filter::count() const
    {
        size_t result = 0;
        for (auto tv: tableViews) {
            result += tv.count;
        }

        return result;
    }

    void Filter::toVector(std::vector<entity_t> & vec) const
    {
        vec.clear();
        for (auto tv: tableViews) {
            for (auto row: tv) {
                vec.push_back(tv.entity(row));
            }
        }
    }

    Filter::Filter(World * world, std::vector<TableView>  tableViews)
        : world(world)
          , tableViews(std::move(tableViews))
    {}
}