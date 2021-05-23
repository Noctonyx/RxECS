#pragma once
#include <cstdint>
#include <set>
#include <vector>
#include <array>

#include "Column.h"
#include "Entity.h"
#include "Table.h"

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
    struct QueryResultChunk;
    struct QueryResult;


    struct QueryResultChunkRowIterator
    {
        uint32_t row;
        const QueryResultChunk * chunk;

        bool operator !=(const QueryResultChunkRowIterator & rhs) const
        {
            return row != rhs.row;
        }

        QueryResultChunkRowIterator & operator++();
        uint32_t operator*() const;
    };

    struct QueryResultChunk
    {
        friend struct QueryResult;
    private:
        World * world;
        QueryResult * result;
        Table * table;

        size_t startRow;
        size_t count;

        std::unordered_map<component_id_t, Column *> columns;

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
            std::array<component_id_t, sizeof...(U)> comps) const
        {
            std::array<Column *, sizeof...(U)> r;

            std::transform(comps.begin(), comps.end(), r.begin(), [this](auto & comp) -> Column*
            {
                if (auto it = table->columns.find(comp); it == table->columns.end()) {
                    return nullptr;
                } else {
                    return it->second;
                }
            });

#if 0
            uint32_t x = 0;
            for(auto & c: comps) {
                auto it = table->columns.find(c);

                if(it == table->columns.end()) {
                    r[x] = nullptr;
                } else {
                    r[x] = it->second;
                }
                x++;
            }
#endif
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

        template <typename T> //, typename U, typename=std::enable_if_t<!std::is_const<U>::value>>
        T * rowComponent(const uint32_t row) const
        {
            return getUpdate<T>(row);
        }

        [[nodiscard]] QueryResultChunkRowIterator begin() const
        {
            return QueryResultChunkRowIterator{0, this};
        }

        [[nodiscard]] QueryResultChunkRowIterator end() const
        {
            return QueryResultChunkRowIterator{static_cast<uint32_t>(table->entities.size()), this};
        }
    };

    struct QueryResultChunkIterator
    {
        uint32_t row;
        const QueryResult * result;

        bool operator !=(const QueryResultChunkIterator & rhs) const
        {
            return row != rhs.row;
        }

        void operator++()
        {
            row++;
        }

        const QueryResultChunk & operator*() const;
    };

    struct QueryResult
    {
        friend struct QueryResultChunkIterator;

    private:
        World * world;
        std::set<component_id_t> components;
        std::vector<QueryResultChunk> chunks;
        //std::set<std::pair<component_id_t, std::set<component_id_t>>> relations;
        robin_hood::unordered_map<component_id_t, component_id_t> relationLookup;
        std::set<component_id_t> singletons;
        bool inheritance;

    public:
        uint32_t total;
        bool valid;

        QueryResult(World * world,
                    const std::vector<Table *> & tableList,
                    const std::set<component_id_t> & with,
                    const std::set<std::pair<component_id_t, std::set<component_id_t>>> & relations,
                    const std::set<component_id_t> & singletons,
                    bool inheritance
        );

        ~QueryResult();

        [[nodiscard]] uint32_t count() const
        {
            return total;
        }

        [[nodiscard]] QueryResultChunkIterator begin() const
        {
            return QueryResultChunkIterator{0, this};
        }

        [[nodiscard]] QueryResultChunkIterator end() const
        {
            return QueryResultChunkIterator{static_cast<uint32_t>(chunks.size()), this};
        }

        [[nodiscard]] size_t size() const
        {
            return chunks.size();
        }

        QueryResultChunk & operator[](size_t ix)
        {
            return chunks[ix];
        }

        template <typename Func>
        void iter(Func && f) const
        {
            for (auto & ch: *this) {
                f(world, ch);
            }
        }

        void * checkTables(QueryResultChunk & chunk,
                           component_id_t componentId,
                           uint32_t row,
                           bool mutate);
        void * checkSingletons(component_id_t componentId, bool mutate) const;
        void * checkRelations(const QueryResultChunk & chunk,
                              uint32_t row,
                              component_id_t componentId,
                              bool mutate) const;

        const void * checkInstancing(entity_t entity,
                                     component_id_t componentId,
                                     bool mutate) const;

        template <typename Tuple, std::size_t I>
        void setResultValue(entity_t entity,
                            const QueryResultChunk & chunk,
                            Column * col,
                            Tuple & t,
                            const uint32_t row,
                            const component_id_t componentId,
                            bool mutate) const;

        template <std::size_t Fixed, typename Tuple, typename Comps, typename Muts, std::size_t ...
                  I>
        void populateResult(entity_t entity,
                            const QueryResultChunk & chunk,
                            std::array<Column *, sizeof...(I)> columns,
                            Tuple & result,
                            Comps & comps,
                            Muts & muts,
                            const uint32_t row,
                            std::index_sequence<I...>) const;

        template <typename ... U, typename Func>
        void each(Func && f) const;

        template <typename ... U, typename Func>
        void eachChunk(Func && f,
                       const std::array<component_id_t, sizeof...(U)> & comps,
                       const std::array<bool, sizeof...(U) + 1> & mp,
                       const QueryResultChunk & chunk) const;
    };

    template <typename Tuple, std::size_t I>
    void QueryResult::setResultValue(entity_t entity,
                                     const QueryResultChunk & chunk,
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
            result_ptr = checkRelations(chunk, row, componentId, mutate);
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
                                     const QueryResultChunk & chunk,
                                     std::array<Column *, sizeof...(I)> columns,
                                     Tuple & result,
                                     Comps & comps,
                                     Muts & muts,
                                     const uint32_t row,
                                     std::index_sequence<I...>) const
    {
        //chunk.checkIndex(row);
        (setResultValue<Tuple, I + Fixed>(entity, chunk, columns[I], result, row, comps[I],
                                          muts[I + Fixed]), ...);
    }

    template <typename ... U, typename Func>
    void QueryResult::eachChunk(Func && f,
                                const std::array<component_id_t, sizeof...(U)> & comps,
                                const std::array<bool, sizeof...(U) + 1> & mp,
                                const QueryResultChunk & chunk) const
    {
        auto columns = chunk.getColumns<U...>(comps);

        for(size_t row = chunk.startRow; row < chunk.count + chunk.startRow; row++){
            std::tuple<EntityHandle, U *...> result;
            std::get<0>(result) = EntityHandle{chunk.entity(row), world};
            populateResult<std::tuple_size<decltype(result)>() - sizeof...(U)>(
                chunk.entity(row), chunk, columns, result, comps, mp, row,
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

        //std::get<0>(result) = world;

        for (auto & chunk: *this) {

            eachChunk<U...>(f, comps, mp, chunk);
        }
    }
}
