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
        QueryResultChunk * chunk;

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

    public:
        void checkIndex(uint32_t rowIndex) const;
        [[nodiscard]] entity_t entity(uint32_t rowIndex) const;
        [[nodiscard]] const void * get(component_id_t comp, uint32_t row) const;
        void checkValidity() const;

        [[nodiscard]] void * getUpdate(component_id_t comp, uint32_t row) const;

        template <typename T>
        std::span<const T> getColumn()
        {
            return table->columns[world->getComponentId<T>()]->getComponentData();
        }

        template <typename T>
        const T * get(const uint32_t row)
        {
            return static_cast<const T *>(get(world->getComponentId<T>(), row));
        }

        template <typename T>
        T * getUpdate(const uint32_t row)
        {
            return static_cast<T *>(getUpdate(world->getComponentId<T>(), row));
        }

        template <typename T> //, typename U, typename=std::enable_if_t<!std::is_const<U>::value>>
        T * rowComponent(const uint32_t row)
        {
            return getUpdate<T>(row);
        }

        QueryResultChunkRowIterator begin()
        {
            return QueryResultChunkRowIterator{0, this};
        }

        QueryResultChunkRowIterator end()
        {
            return QueryResultChunkRowIterator{static_cast<uint32_t>(table->entities.size()), this};
        }
    };

    struct QueryResultChunkIterator
    {
        uint32_t row;
        QueryResult * result;

        bool operator !=(const QueryResultChunkIterator & rhs) const
        {
            return row != rhs.row;
        }

        void operator++()
        {
            row++;
        }

        QueryResultChunk & operator*() const;
    };

    struct QueryResult
    {
        friend struct QueryResultChunkIterator;

    private:
        World * world;
        std::set<component_id_t> components;
        std::vector<QueryResultChunk> chunks;
        std::set<std::pair<component_id_t, std::set<component_id_t>>> relations;
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

        QueryResultChunkIterator begin()
        {
            return QueryResultChunkIterator{0, this};
        }

        QueryResultChunkIterator end()
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
        void iter(Func && f)
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
        void * checkRelations(QueryResultChunk & chunk,
                              uint32_t row,
                              component_id_t componentId,
                              bool mutate);

        const void * checkInstancing(entity_t entity,
                                     component_id_t componentId,
                                     bool mutate);

        template <typename Tuple, std::size_t I>
        void setResultValue(entity_t entity,
                            QueryResultChunk & chunk,
                            Tuple & t,
                            const uint32_t row,
                            const component_id_t componentId,
                            bool mutate);

        template <std::size_t Fixed, typename Tuple, typename Comps, typename Muts, std::size_t ... I>
        void populateResult(entity_t entity,
                            QueryResultChunk & chunk,
                            Tuple & result,
                            Comps & comps,
                            Muts & muts,
                            const uint32_t row,
                            std::index_sequence<I...>);

        template <typename ... U, typename Func>
        void each(Func && f);
    };

    template <typename Tuple, std::size_t I>
    void QueryResult::setResultValue(entity_t entity,
                                     QueryResultChunk & chunk,
                                     Tuple & t,
                                     const uint32_t row,
                                     const component_id_t componentId,
                                     bool mutate)
    {
        void * result_ptr = checkTables(chunk, componentId, row, mutate);

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

    template <std::size_t Fixed, typename Tuple, typename Comps, typename Muts, std::size_t... I>
    void QueryResult::populateResult(entity_t entity,
                                     QueryResultChunk & chunk,
                                     Tuple & result,
                                     Comps & comps,
                                     Muts & muts,
                                     const uint32_t row,
                                     std::index_sequence<I...>)
    {
        (setResultValue<Tuple, I + Fixed>(entity, chunk, result, row, comps[I], muts[I + Fixed]), ...);
    }

    template <typename ... U, typename Func>
    void QueryResult::each(Func && f)
    {
        std::array<component_id_t, sizeof...(U)> comps = {
            world->getComponentId<std::remove_const_t<U>>()...
        };
        auto mp = get_mutable_parameters(f);

        std::tuple<EntityBuilder, U *...> result;

        //std::get<0>(result) = world;

        for (auto & chunk: *this) {
            for (auto row: chunk) {
                std::get<0>(result) = EntityBuilder{ chunk.entity(row), world };
                populateResult<std::tuple_size<decltype(result)>() - sizeof...(U)>(
                    chunk.entity(row), chunk, result, comps, mp, row,
                    std::make_index_sequence<sizeof...(U)>()
                );
                std::apply(f, result);
            }
        }
    }
}
