#pragma once
#include <vector>
#include <set>
#include <typeinfo>
#include <typeindex>

#include "robin_hood.h"

#include "ArchetypeManager.h"
#include "Component.h"
#include "Entity.h"
#include "EntityHandle.h"
#include "QueryBuilder.h"
#include "Table.h"
#include "SystemBuilder.h"


namespace ecs
{
    struct Stream;
    struct Query;

    struct EntityEntry
    {
        uint32_t version;
        uint32_t archetype;
        uint32_t row;
        bool alive;
    };

    struct EntityArchetypeEntry
    {
        uint32_t archetype;
        uint32_t row;
    };

    struct Prefab {};

    struct WorldIterator
    {
        World* world;
        std::vector<Archetype>::iterator it;

        WorldIterator & operator++()
        {
            it++;
            return *this;
        }

        bool operator!=(const WorldIterator & other) const
        {
            return it != other.it;
        }

        Archetype & operator*()
        {
            return *it;
        }
    };

    enum class DeferredCommandType:  uint8_t
    {
        Destroy,
        Add,
        Remove,
        Set
    };

    struct DeferredCommand
    {
        DeferredCommandType type;
        entity_t entity;
        component_id_t component;
        void* ptr;
//        void 
    };

    class World
    {
        friend struct Table;

    public:
        World();
        ~World();

        EntityHandle newEntity(const char * name = nullptr );
        //entity_t newEntity();
        EntityHandle instantiate(entity_t prefab);

        EntityHandle lookup(const char* name);
        EntityHandle lookup(const std::string & name);

        [[nodiscard]] bool isAlive(entity_t id) const;
        void destroy(entity_t id);
        void destroyDeferred(entity_t id);

        template <typename T>
        void add(entity_t id);
        void add(entity_t id, component_id_t componentId);

        template <typename T>
        void addDeferred(entity_t id);
        void addDeferred(entity_t id, component_id_t componentId);

        template <typename T>
        bool has(entity_t id);
        bool has(entity_t id, component_id_t componentId);

        template <typename T>
        void remove(entity_t id);
        void remove(entity_t id, component_id_t componentId);

        template <typename T>
        void removeDeferred(entity_t id);
        void removeDeferred(entity_t id, component_id_t componentId);

        template <typename T>
        const T * get(entity_t id, bool inherit = false);
        template <typename T>
        T * getUpdate(entity_t id);

        const void * get(entity_t id, component_id_t componentId, bool inherited = false);
        void * getUpdate(entity_t id, component_id_t componentId);

        template <typename T>
        void set(entity_t id, const T & value);
        void set(entity_t id, component_id_t componentId, const void * ptr);

        template <typename T>
        void setDeferred(entity_t id, T&& value);
        void setDeferred(entity_t id, component_id_t componentId, void* ptr);       

        template <typename T>
        void addSingleton();
        void addSingleton(component_id_t componentId);

        template <typename T>
        bool hasSingleton();
        bool hasSingleton(component_id_t componentId);

        template <typename T>
        void removeSingleton();
        void removeSingleton(component_id_t componentId);

        template <typename T>
        void setSingleton(const T& value);
        void setSingleton(component_id_t componentId, const void* ptr);

        template <typename T>
        const T* getSingleton();
        template <typename T>
        T* getSingletonUpdate();

        const void* getSingleton(component_id_t componentId);
        void* getSingletonUpdate(component_id_t componentId);

        template <typename T>
        Stream * getStream();
        Stream * getStream(component_id_t id);

//        template <typename T>
  //      streamid_t createStream();
    //    streamid_t createStream(component_id_t id);

//        void deleteStream(streamid_t id);
  //      Stream * getStream(streamid_t id);    

        const Component * getComponentDetails(component_id_t id);

        template <typename T>
        component_id_t getComponentId();

        QueryBuilder createQuery(const std::set<component_id_t> & with);

        template <class ... TArgs>
        QueryBuilder createQuery();

        SystemBuilder createSystem(const char * name);

        void deleteQuery(queryid_t q);
        void deleteSystem(systemid_t s);
        QueryResult getResults(queryid_t q);

        void step(float delta);

        WorldIterator begin();
        WorldIterator end();

        Table * getTableForArchetype(uint32_t t)
        {
            return tables[t];
        }

        void executeDeferred();

        std::string description(entity_t id);
        Archetype& getEntityArchetypeDetails(entity_t id);
       

    protected:
        uint32_t getEntityArchetype(entity_t id) const;
        void moveEntity(entity_t id, uint32_t from, const ArchetypeTransition & trans);
        void addTableToActiveQueries(Table * table, uint32_t aid);
        void ensureTableForArchetype(uint32_t);

        void recalculateSystemOrder();

    private:
        std::vector<EntityEntry> entities{};

        size_t recycleStart;
        robin_hood::unordered_map<std::type_index, component_id_t> componentMap{};

        Component componentBootstrap;
        component_id_t componentBootstrapId;

        robin_hood::unordered_map<uint32_t, Table *> tables;
        //robin_hood::unordered_map<component_id_t> streams;

        float deltaTime;

        queryid_t systemQuery;
        queryid_t queryQuery = 0;
        queryid_t streamQuery = 0;

        std::vector<DeferredCommand> deferredCommands;

        std::vector<entity_t> systemOrder{};
        robin_hood::unordered_map<std::string, entity_t> nameIndex{};

    public:
        ArchetypeManager am;
    };

    template <typename T>
    void World::add(entity_t id)
    {
        auto componentId = getComponentId<T>();
        add(id, componentId);
    }

    template <typename T>
    void World::addDeferred(entity_t id)
    {
        addDeferred(id, getComponentId<T>());
    }

    template <typename T>
    bool World::has(const entity_t id)
    {
        return has(id, getComponentId<T>());
    }

    template <typename T>
    void World::remove(entity_t id)
    {
        remove(id, getComponentId<T>());
    }

    template <typename T>
    void World::removeDeferred(entity_t id)
    {
        removeDeferred(id, getComponentId<T>());
    }

    template <typename T>
    const T * World::get(entity_t id, bool inherit)
    {
        auto ptr = static_cast<const T *>(get(id, getComponentId<T>(), inherit));
        return ptr;
    }

    template <typename T>
    T * World::getUpdate(entity_t id)
    {
        auto ptr = static_cast<T *>(getUpdate(id, getComponentId<T>()));
        return ptr;
    }

    template <typename T>
    void World::set(entity_t id, const T & value)
    {
        set(id, getComponentId<T>(), &value);
    }

    template <typename T>
    void World::setDeferred(entity_t id, T && value)
    {
        auto c = getComponentId<T>();

        auto cd = getComponentDetails(c);
        char* cp = new char[cd->size];

        T* p = new (cp) T(std::move(value));
        setDeferred(id, getComponentId<T>(), p);
    }

    template <typename T>
    void World::addSingleton()
    {
        addSingleton(getComponentId<T>());
    }

    template <typename T>
    bool World::hasSingleton()
    {
        return hasSingleton(getComponentId<T>());
    }

    template <typename T>
    void World::removeSingleton()
    {
        removeSingleton(getComponentId<T>());
    }

    template <typename T>
    void World::setSingleton(const T & value)
    {
        setSingleton(getComponentId<T>(), &value);
    }

    template <typename T>
    const T * World::getSingleton()
    {
        return static_cast<const T*>(getSingleton(getComponentId<T>()));
    }

    template <typename T>
    T * World::getSingletonUpdate()
    {
        return getSingletonUpdate(getComponentId<T>());
    }

    template <typename T>
    Stream * World::getStream()
    {
        return getStream(getComponentId<T>());
    }

    template <typename T>
    component_id_t World::getComponentId()
    {
        static_assert(std::is_copy_constructible<T>(), "Cannot be a component");
        //static_assert(std::is_trivially_copyable<T>(), "Cannot be a component");
        static_assert(std::is_move_constructible<T>(), "Cannot be a component");
        static_assert(std::is_default_constructible<T>(), "Cannot be a component");
       // static_assert(std::is_standard_layout<T>(), "Cannot be a component");

        auto v = std::type_index(typeid(T));
        if (componentMap.find(v) != componentMap.end()) {
            return componentMap[v];
        }

        component_id_t id = newEntity().id;
        set<Component>(id, {
                           typeid(T).name(),
                           sizeof(T), alignof(T),
                           componentConstructor<T>,
                           componentDestructor<T>,
                           componentCopy<T>,
                           componentMove<T>
                       });

        componentMap.emplace(v, id);
        return id;
    }

    template <class ... TArgs>
    QueryBuilder World::createQuery()
    {
        std::set<component_id_t> with = { getComponentId<TArgs>()... };
        return createQuery(with);
    }
}
