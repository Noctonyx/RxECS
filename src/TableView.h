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

#ifndef RXECS_TABLEVIEW_H
#define RXECS_TABLEVIEW_H

#include <cstdint>
#include <set>
#include <unordered_set>
#include <vector>
#include <array>

#include "Entity.h"
#include "Table.h"
#include "World.h"

namespace ecs
{
    struct TableView;
    
    struct TableViewRowIterator
    {
        size_t row{};
        const TableView * view{};

        bool operator!=(const TableViewRowIterator & rhs) const
        {
            return row != rhs.row;
        }

        TableViewRowIterator & operator++();
        uint32_t operator*() const;
    };

    struct TableView
    {
        World * world{};
        Table * table{};
        Timestamp tableUpdateTimestamp;

        size_t startRow{};
        size_t count{};

        void checkIndex(uint32_t rowIndex) const;
        [[nodiscard]] entity_t entity(uint32_t rowIndex) const;
        [[nodiscard]] const void * get(component_id_t comp, uint32_t row) const;
        void checkValidity() const;

        //bool passesFilters(const std::unordered_set <component_id_t> & filters) const;

        [[nodiscard]] void * getUpdate(component_id_t comp, uint32_t row) const;

        template<typename T>
        std::span<const T> getColumn() const;

        template<typename ... U>
        std::array<Column *, sizeof...(U)> getColumns(
            const std::array<component_id_t, sizeof...(U)> & comps) const
        {
            std::array < Column * , sizeof...(U) > r;

            std::transform(
                comps.begin(), comps.end(), r.begin(), [this](auto & comp) -> Column * {
                    if (auto it = table->columns.find(comp); it == table->columns.end()) {
                        return nullptr;
                    } else {
                        return it->second.get();
                    }
                }
            );

            return r;
        }

        template<typename T>
        const T * get(const uint32_t row) const
        {
            return static_cast<const T *>(get(world->getComponentId<T>(), row));
        }

        template<typename T>
        T * getUpdate(const uint32_t row) const
        {
            return static_cast<T *>(getUpdate(world->getComponentId<T>(), row));
        }

        template<typename T>
        T * rowComponent(const uint32_t row) const
        {
            return getUpdate<T>(row);
        }

        [[nodiscard]] TableViewRowIterator begin() const
        {
            return TableViewRowIterator{startRow, this};
        }

        [[nodiscard]] TableViewRowIterator end() const
        {
            return TableViewRowIterator{startRow + count, this};
        }
    };
}

#endif //RXECS_TABLEVIEW_H
