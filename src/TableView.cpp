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

#include "TableView.h"

namespace ecs
{
    TableViewRowIterator & TableViewRowIterator::operator++()
    {
        view->checkValidity();
        row++;
        return *this;
    }

    uint32_t TableViewRowIterator::operator*() const
    {
        return static_cast<uint32_t>(row);
    }

    void TableView::checkIndex(uint32_t rowIndex) const
    {
        if (rowIndex >= startRow + count || rowIndex < startRow) {
            throw std::range_error("row out of range");
        }
    }

    entity_t TableView::entity(uint32_t rowIndex) const
    {
        checkIndex(rowIndex);
        return table->entities[rowIndex];
    }

    const void * TableView::get(component_id_t comp, const uint32_t row) const
    {
        return getUpdate(comp, row);
    }

    void TableView::checkValidity() const
    {
        if (tableUpdateTimestamp != table->lastUpdateTimestamp) {
            throw std::runtime_error("Invalidated query results");
        }
    }
#if 0
    bool TableView::passesFilters(const std::unordered_set <component_id_t> & filters) const
    {
        for (auto f: filters) {
            if (!table->hasComponent(f)) {
                return false;
            }
        }
        return true;
    }
#endif
    void * TableView::getUpdate(component_id_t comp, const uint32_t row) const
    {
        auto c = table->columns.find(comp);
        if (c == table->columns.end()) {
            return nullptr;
        }

        return (*c).second->getEntry(row);
    }
}