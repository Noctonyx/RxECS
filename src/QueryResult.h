#pragma once
#include <cstdint>
#include <set>
#include <vector>
#include <array>

#include "Column.h"
#include "Entity.h"
#include "Table.h"

#include "Jobs/JobManager.hpp"

template <class T>
concept mutable_parameter = (std::is_reference_v<T> || std::is_pointer_v<T>)
&& !std::is_const_v<std::remove_reference_t<std::remove_pointer_t<T>>>;

template <class R, class... A>
constexpr auto get_mutable_parameters(R (*)(A ...))
{
    return std::array{mutable_parameter<A>...};
}

template <class R, class O, class... A>
constexpr auto get_mutable_parameters(R (O::*)(A ...))
{
    return std::array{mutable_parameter<A>...};
}

template <class R, class O, class... A>
constexpr auto get_mutable_parameters(R (O::*)(A ...) const)
{
    return std::array{mutable_parameter<A>...};
}

template <class T>
constexpr auto get_mutable_parameters(T)
{
    return get_mutable_parameters(&T::operator());
}

namespace ecs
{
    struct TableView;
    struct QueryResult;


    struct TableViewRowIterator
    {
        size_t row;
        const TableView * view;

        bool operator !=(const TableViewRowIterator & rhs) const
        {
            return row != rhs.row;
        }

        TableViewRowIterator & operator++();
        uint32_t operator*() const;
    };

    struct TableView
    {
        friend struct QueryResult;
    private:
        World * world{};
        Table * table{};
        Timestamp tableUpdateTimestamp;

        size_t startRow{};
        size_t count{};

    public:
        void checkIndex(uint32_t rowIndex) const;
        [[nodiscard]] entity_t entity(uint32_t rowIndex) const;
        [[nodiscard]] const void * get(component_id_t comp, uint32_t row) const;
        void checkValidity() const;

        [[nodiscard]] void * getUpdate(component_id_t comp, uint32_t row) const;

        template <typename T>
        std::span<const T> getColumn() const
        {
            return table->columns[world->getComponentId<T>()]->getComponentData();
        }

        template <typename ... U>
        std::array<Column *, sizeof...(U)> getColumns(
            const std::array<component_id_t, sizeof...(U)> & comps) const
        {
            std::array<Column *, sizeof...(U)> r;

            std::transform(comps.begin(), comps.end(), r.begin(), [this](auto & comp) -> Column*
            {
                if (auto it = table->columns.find(comp); it == table->columns.end()) {
                    return nullptr;
                } else {
                    return it->second.get();
                }
            });

            return r;
        }

        template <typename T>
        const T * get(const uint32_t row) const
        {
            return static_cast<const T *>(get(world->getComponentId<T>(), row));
        }

        template <typename T>
        T * getUpdate(const uint32_t row) const
        {
            return static_cast<T *>(getUpdate(world->getComponentId<T>(), row));
        }

        template <typename T>
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

    struct TableViewIterator
    {
        uint32_t row;
        const QueryResult * result;

        bool operator !=(const TableViewIterator & rhs) const
        {
            return row != rhs.row;
        }

        void operator++()
        {
            row++;
        }

        const TableView & operator*() const;
    };

    struct QueryResult
    {
        friend struct TableViewIterator;

    private:
        World * world;
        std::set<component_id_t> components;
        std::vector<TableView> tableViews;
        //std::set<std::pair<component_id_t, std::set<component_id_t>>> relations;
        robin_hood::unordered_map<component_id_t, component_id_t> relationLookup;
        std::set<component_id_t> singletons;
        bool inheritance;
        bool thread;

    public:
        uint32_t total;
        //bool valid;

        QueryResult(World * world,
                    const std::vector<Table *> & tableList,
                    const std::set<component_id_t> & with,
                    const std::set<std::pair<component_id_t, std::set<component_id_t>>> & relations,
                    const std::set<component_id_t> & singletons,
                    bool inheritance,
                    bool thread
        );

        [[nodiscard]] uint32_t count() const
        {
            return total;
        }

        [[nodiscard]] TableViewIterator begin() const
        {
            return TableViewIterator{0, this};
        }

        [[nodiscard]] TableViewIterator end() const
        {
            return TableViewIterator{static_cast<uint32_t>(tableViews.size()), this};
        }

        [[nodiscard]] size_t size() const
        {
            return tableViews.size();
        }

        TableView & operator[](size_t ix)
        {
            return tableViews[ix];
        }

        template <typename Func>
        void iter(Func && f) const
        {
            for (auto & ch: *this) {
                f(world, ch);
            }
        }

        void * checkTables(TableView & view,
                           component_id_t componentId,
                           uint32_t row,
                           bool mutate);
        void * checkSingletons(component_id_t componentId, bool mutate) const;
        void * checkRelations(const TableView & view,
                              uint32_t row,
                              component_id_t componentId,
                              bool mutate) const;

        const void * checkInstancing(entity_t entity,
                                     component_id_t componentId,
                                     bool mutate) const;

        template <typename Tuple, std::size_t I>
        void setResultValue(entity_t entity,
                            const TableView & view,
                            Column * col,
                            Tuple & t,
                            const uint32_t row,
                            const component_id_t componentId,
                            bool mutate) const;

        template <std::size_t Fixed, typename Tuple, typename Comps, typename Muts, std::size_t ...
                  I>
        void populateResult(entity_t entity,
                            const TableView & view,
                            std::array<Column *, sizeof...(I)> columns,
                            Tuple & result,
                            Comps & comps,
                            Muts & muts,
                            const uint32_t row,
                            std::index_sequence<I...>) const;

        template <typename ... U, typename Func>
        void each(Func && f) const;

        template <typename ... U, typename Func>
        void eachView(Func && f,
                      const std::array<component_id_t, sizeof...(U)> & comps,
                      const std::array<bool, sizeof...(U) + 1> & mp,
                      const TableView & view) const;
    };

    template <typename Tuple, std::size_t I>
    void QueryResult::setResultValue(entity_t entity,
                                     const TableView & view,
                                     Column * col,
                                     Tuple & t,
                                     const uint32_t row,
                                     const component_id_t componentId,
                                     bool mutate) const
    {
        void * result_ptr = nullptr;

        if (col) {
            std::get<I>(t) = static_cast<std::tuple_element_t<I, Tuple>>(col->getEntry(row));
            return;
        }

        if (!result_ptr) {
            result_ptr = checkRelations(view, row, componentId, mutate);
        }
        if (!result_ptr && !mutate && inheritance) {
            result_ptr = const_cast<void *>(checkInstancing(entity, componentId, mutate));
        }
        if (!result_ptr) {
            result_ptr = checkSingletons(componentId, mutate);
        }

        std::get<I>(t) = static_cast<std::tuple_element_t<I, Tuple>>(result_ptr);
    }

    template <std::size_t Fixed, typename Tuple, typename Comps, typename Muts, std::size_t...
              I>
    void QueryResult::populateResult(entity_t entity,
                                     const TableView & view,
                                     std::array<Column *, sizeof...(I)> columns,
                                     Tuple & result,
                                     Comps & comps,
                                     Muts & muts,
                                     const uint32_t row,
                                     std::index_sequence<I...>) const
    {
        (setResultValue<Tuple, I + Fixed>(entity, view, columns[I], result, row, comps[I],
                                          muts[I + Fixed]), ...);
    }

    template <typename ... U, typename Func>
    void QueryResult::eachView(Func && f,
                               const std::array<component_id_t, sizeof...(U)> & comps,
                               const std::array<bool, sizeof...(U) + 1> & mp,
                               const TableView & view) const
    {
        auto columns = view.getColumns<U...>(comps);

        for (auto row: view) {
            //for(size_t row = view.startRow; row < view.count + view.startRow; row++){
            std::tuple<EntityHandle, U *...> result;
            std::get<0>(result) = EntityHandle{view.entity(row), world};
            populateResult<std::tuple_size<decltype(result)>() - sizeof...(U)>(
                view.entity(row), view, columns, result, comps, mp, row,
                std::make_index_sequence<sizeof...(U)>()
            );
            std::apply(f, result);
        }
    }

    template <typename ... U, typename Func>
    void QueryResult::each(Func && f) const
    {
        std::array<component_id_t, sizeof...(U)> comps = {
            world->getComponentId<std::remove_const_t<U>>()...
        };
        auto mp = get_mutable_parameters(f);

        if (thread && tableViews.size() > 2 && total > 1000) {
            std::vector<std::shared_ptr<RxCore::Job<void>>> jobs;

            for (auto & view: *this) {
                auto j = RxCore::CreateJob<void>([=]()
                    {
                        eachView<U...>(f, comps, mp, view);
                    }
                );
                j->schedule();
                jobs.push_back(j);
            }

            for (auto & jj: jobs) {
                jj->waitComplete();
            }

            jobs.clear();
        } else {
            for (auto & view: *this) {
                eachView<U...>(f, comps, mp, view);
            }
        }
    }
}
