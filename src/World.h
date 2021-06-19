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
#include <deque>
#include <map>
#include <mutex>
#include <vector>
#include <set>
#include <stack>
#include <typeinfo>
#include <unordered_set>

#include "robin_hood.h"

#include "ArchetypeManager.h"
#include "Component.h"
#include "Entity.h"
#include "EntityHandle.h"
#include "QueryBuilder.h"
#include "Table.h"
#include "SystemBuilder.h"
#include "EntityQueueHandle.h"

namespace ecs
{
    struct System;

    namespace type_helper
    {
        template<typename T>
        struct type_id_ptr
        {
            static const T * const id;
            static const entity_t entityId;
        };

        template<typename T>
        const T * const type_id_ptr<T>::id = nullptr;
    }

    using type_id_t = const void *;

    template<typename T>
    constexpr auto type_id() noexcept -> type_id_t
    {
        return &type_helper::type_id_ptr<T>::id;
    }

    struct Stream;
    struct Query;
    struct Filter;
    struct EntityQueue;

    struct EntityEntry
    {
        uint32_t version;
        uint32_t row;
        uint64_t updateSequence;
        uint16_t archetype;
        bool alive;
    };

    struct Prefab
    {
    };

    struct PendingDelete
    {

    };

    struct HasEntityQueue
    {
    };

    struct WorldIterator
    {
        World * world;
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

        Archetype & operator*() const
        {
            return *it;
        }
    };

    struct SingletonIterator
    {
        World * world;
        std::vector<component_id_t>::iterator it;

        SingletonIterator & operator++()
        {
            it++;
            return *this;
        }

        bool operator!=(const SingletonIterator & other) const
        {
            return it != other.it;
        }

        component_id_t operator*() const
        {
            return *it;
        }
    };

    enum class DeferredCommandType : uint8_t
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
        void * ptr;
    };

    struct JobInterface
    {
        using JobHandle = std::shared_ptr<void>;

        virtual JobHandle create(std::function<uint32_t()>) = 0;
        virtual void schedule(JobHandle) = 0;
        virtual bool isComplete(JobHandle) const = 0;
        virtual void awaitCompletion(JobHandle) = 0;
        virtual uint32_t getJobResult(JobHandle) = 0;
    };

    struct ModuleComponent
    {
        friend class World;
    private:
        bool enabled = false;
        void * modulePtr;
    };

    struct HasModule : Relation
    {
    };

    class World
    {
        friend struct Table;
        friend struct QueryResult;
        friend class ActiveSystem;

    public:
        World();
        ~World();

        EntityHandle newEntity(const char * name = nullptr);
        EntityHandle newEntityReplace(const char * name);
        //entity_t newEntity();
        EntityHandle instantiate(entity_t prefab);

        EntityHandle lookup(const char * name);
        EntityHandle lookup(const std::string & name);

        [[nodiscard]] bool isAlive(entity_t id) const;
        void destroy(entity_t id);
        void destroyDeferred(entity_t id);

        template<typename T>
        void add(entity_t id);
        void add(entity_t id, component_id_t componentId);

        template<typename T>
        void addDeferred(entity_t id);
        void addDeferred(entity_t id, component_id_t componentId);

        template<typename T>
        bool has(entity_t id);
        bool has(entity_t id, component_id_t componentId);

        template<typename T>
        void remove(entity_t id);
        void remove(entity_t id, component_id_t componentId);

        template<typename T>
        void removeDeferred(entity_t id);
        void removeDeferred(entity_t id, component_id_t componentId);

        template<typename T>
        const T * get(entity_t id, bool inherit = false);
        template<class T>
        void update(entity_t id, std::function<void(T *)> && f);

        template<typename T, typename U>
        const U * getRelated(entity_t id);

        template<typename T>
        EntityHandle getRelatedEntity(entity_t id);

        const void * get(entity_t id, component_id_t componentId, bool inherited = false);

        template<typename T>
        void set(entity_t id, const T & value);
        void set(entity_t id, component_id_t componentId, const void * ptr);

        template<typename T>
        void setDeferred(entity_t id, const T & value);
        void setDeferred(entity_t id, component_id_t componentId, void * ptr);

        template<typename T>
        void addSingleton();
        void addSingleton(component_id_t componentId);

        template<typename T>
        bool hasSingleton();
        bool hasSingleton(component_id_t componentId) const;

        template<typename T>
        void removeSingleton();
        void removeSingleton(component_id_t componentId);

        template<typename T>
        void setSingleton(const T & value);
        void setSingleton(component_id_t componentId, const void * ptr);

        template<typename T>
        const T * getSingleton();
        template<typename T>
        T * getSingletonUpdate();

        const void * getSingleton(component_id_t componentId);
        void * getSingletonUpdate(component_id_t componentId);

        template<typename T>
        Stream * getStream();
        Stream * getStream(component_id_t id);

        const Component * getComponentDetails(component_id_t id);

        template<typename T>
        component_id_t getComponentId();

        QueryBuilder createQuery(const std::set<component_id_t> & with);

        template<class ... TArgs>
        QueryBuilder createQuery();

        SystemBuilder createSystem(const char * name = nullptr);

        EntityQueueHandle createEntityQueue(const char * name = nullptr);
        void destroyEntityQueue(entity_t eq);

        void deleteQuery(queryid_t q);
        void deleteSystem(systemid_t s);
        QueryResult getResults(queryid_t q);
        std::optional<JobInterface::JobHandle> executeSystem(systemid_t sys);
        void executeGroupsSystems(entity_t systemGroup);
        void executeSystemGroup(entity_t systemGroup);

        void step(float delta);

        WorldIterator begin();
        WorldIterator end();

        std::unordered_map<component_id_t, void *> & allSingletons()
        {
            return singletons;
        }

        Table * getTableForArchetype(uint16_t t)
        {
            return tables[t].get();
        }

        void executeDeferred();

        std::string description(entity_t id);
        Archetype & getEntityArchetypeDetails(entity_t id);

        [[nodiscard]] float deltaTime() const
        {
            return deltaTime_;
        }

        void markSystemsDirty()
        {
            systemOrderDirty = true;
        }

        void setJobInterface(JobInterface * jobi)
        {
            this->jobInterface = jobi;
        }

        template<class T>
        entity_t createModule();

        template<class T>
        entity_t getModule();

        void setModuleObject(entity_t module, void * ptr);
        template<class T>
        T * getModuleObject();

        void pushModuleScope(entity_t module);
        void popModuleScope();

        void setModuleEnabled(entity_t module, bool enabled);

        template<class ... Comp>
        std::vector<component_id_t> makeComponentList();

        void setAsParent(entity_t id);
        void removeAsParent(entity_t id);

        Filter createFilter(std::vector<component_id_t> with = {}, std::vector<component_id_t> without = {});

        template<class T>
        void addRemoveTrigger(entity_t id);
        template<class T>
        void addAddTrigger(entity_t id);
        template<class T>
        void addUpdateTrigger(entity_t id);
        template<class T>
        void removeRemoveTrigger(entity_t id);
        template<class T>
        void removeAddTrigger(entity_t id);

        void addRemoveTrigger(component_id_t componentId, entity_t entity);
        void addAddTrigger(component_id_t componentId, entity_t entity);
        void addUpdateTrigger(component_id_t componentId, entity_t entity);
        void removeRemoveTrigger(component_id_t componentId, entity_t entity);
        void removeAddTrigger(component_id_t componentId, entity_t entity);
        void removeUpdateTrigger(component_id_t componentId, entity_t entity);

        EntityQueue * getEntityQueue(entity_t id) const;

    protected:
        uint16_t getEntityArchetype(entity_t id) const;
        void moveEntity(entity_t id, uint16_t from, const ArchetypeTransition & trans);
        void addTableToActiveQueries(Table * table, uint16_t aid);
        void removeTableFromActiveQueries(Table * table);
        void ensureTableForArchetype(uint16_t);

        void recalculateSystemOrder();
        void recalculateGroupSystemOrder(entity_t group, std::vector<systemid_t> systems);

        static std::string trimName(const char * n);

        void setEntityUpdateSequence(entity_t id);

        component_id_t createDynamicComponent(entity_t entityId);
        void removeDynamicComponent(entity_t entityId);

        template<typename T>
        T * getUpdate(entity_t id);
        void * getUpdate(entity_t id, component_id_t componentId);

        void postEntity(entity_t id, std::vector<entity_t> & posts);

    public:
        [[nodiscard]] std::vector<entity_t> getPipelineGroupSequence() const
        {
            return pipelineGroupSequence;
        }

    private:
        std::vector<EntityEntry> entities{};

        size_t recycleStart;
        robin_hood::unordered_flat_map<type_id_t, component_id_t> componentMap{};

        Component componentBootstrap;
        component_id_t componentBootstrapId;

        robin_hood::unordered_flat_map<uint16_t, std::unique_ptr<Table>> tables;
        //robin_hood::unordered_map<component_id_t> streams;

        std::unordered_map<entity_t, std::unique_ptr<EntityQueue> > queues;

        float deltaTime_;

        queryid_t systemQuery;
        queryid_t systemGroupQuery;
        queryid_t queryQuery = 0;
        queryid_t streamQuery = 0;

        std::vector<DeferredCommand> deferredCommands;

        std::vector<entity_t> pipelineGroupSequence;
        std::map<std::string, entity_t> nameIndex{};

        std::unordered_map<component_id_t, void *> singletons;

        bool systemOrderDirty = true;

        thread_local inline static System * activeSystem = nullptr;
        thread_local inline static Query * activeQuery = nullptr;

        std::mutex deferredMutex;
        JobInterface * jobInterface = nullptr;

        std::stack<entity_t> moduleScope;

        uint64_t updateSequence = 1;

    public:
        ArchetypeManager am;
    };

    template<typename T>
    void World::add(entity_t id)
    {
        auto componentId = getComponentId<T>();
        add(id, componentId);
    }

    template<typename T>
    void World::addDeferred(entity_t id)
    {
        addDeferred(id, getComponentId<T>());
    }

    template<typename T>
    bool World::has(const entity_t id)
    {
        return has(id, getComponentId<T>());
    }

    template<typename T>
    void World::remove(entity_t id)
    {
        remove(id, getComponentId<T>());
    }

    template<typename T>
    void World::removeDeferred(entity_t id)
    {
        removeDeferred(id, getComponentId<T>());
    }

    template<typename T>
    const T * World::get(entity_t id, bool inherit)
    {
        auto ptr = static_cast<const T *>(get(id, getComponentId<T>(), inherit));
        return ptr;
    }

    template<typename T>
    T * World::getUpdate(entity_t id)
    {
        auto ptr = static_cast<T *>(getUpdate(id, getComponentId<T>()));
        return ptr;
    }

    template<typename T, typename U>
    const U * World::getRelated(entity_t id)
    {
        static_assert(std::is_base_of_v<Relation, T>);
        auto g = get<T>(id);
        if (!g) {
            return nullptr;
        }

        return get<U>(g->entity);
    }

    template<typename T>
    EntityHandle World::getRelatedEntity(entity_t id)
    {
        static_assert(std::is_base_of_v<Relation, T>);
        auto g = get<T>(id);
        if (!g) {
            return EntityHandle{0, this};
        }

        return EntityHandle{g->entity, this};
    }

    template<typename T>
    void World::set(entity_t id, const T & value)
    {
        set(id, getComponentId<T>(), &value);
    }

    template<typename T>
    void World::setDeferred(entity_t id, const T & value)
    {
        auto c = getComponentId<T>();

        auto cd = getComponentDetails(c);
        char * cp = new char[cd->size];

        T * p = new(cp) T(value);
        setDeferred(id, getComponentId<T>(), p);
    }

    template<typename T>
    void World::addSingleton()
    {
        addSingleton(getComponentId<T>());
    }

    template<typename T>
    bool World::hasSingleton()
    {
        return hasSingleton(getComponentId<T>());
    }

    template<typename T>
    void World::removeSingleton()
    {
        removeSingleton(getComponentId<T>());
    }

    template<typename T>
    void World::setSingleton(const T & value)
    {
        setSingleton(getComponentId<T>(), &value);
    }

    template<typename T>
    const T * World::getSingleton()
    {
        return static_cast<const T *>(getSingleton(getComponentId<T>()));
    }

    template<typename T>
    T * World::getSingletonUpdate()
    {
        return static_cast<T *>(getSingletonUpdate(getComponentId<T>()));
    }

    template<typename T>
    Stream * World::getStream()
    {
        return getStream(getComponentId<T>());
    }

    template<typename T>
    component_id_t World::getComponentId()
    {
        static_assert(
            std::is_copy_constructible<std::remove_reference_t<T>>(),
            "Cannot be a component"
        );
        //static_assert(std::is_trivially_copyable<T>(), "Cannot be a component");
        static_assert(
            std::is_move_constructible<std::remove_reference_t<T>>(),
            "Cannot be a component"
        );
        static_assert(
            std::is_default_constructible<std::remove_reference_t<T>>(),
            "Cannot be a component"
        );
        // static_assert(std::is_standard_layout<T>(), "Cannot be a component");

        constexpr bool is_relation = std::is_base_of<Relation, std::remove_reference_t<T>>();

        auto v = type_id<std::remove_reference_t<T>>();

        auto it = componentMap.find(v);
        if (it != componentMap.end()) {
            return it->second;
        }

        component_id_t id = newEntity().id;
        set<Component>(
            id, {
                World::trimName(typeid(std::remove_reference_t<T>).name()),
                sizeof(std::remove_reference_t<T>), alignof(std::remove_reference_t<T>),
                componentConstructor<std::remove_reference_t<T>>,
                componentDestructor<std::remove_reference_t<T>>,
                componentCopy<std::remove_reference_t<T>>,
                componentMove<std::remove_reference_t<T>>,
                is_relation,
                componentAllocator<std::remove_reference_t<T>>,
                componentDeallocator<std::remove_reference_t<T>>
            }
        );

        componentMap.emplace(v, id);
        if (id > 2)
            set<Name>(id, {.name = trimName(typeid(std::remove_reference_t<T>).name())});
        return id;
    }

    template<class ... TArgs>
    QueryBuilder World::createQuery()
    {
        std::set<component_id_t> with = {getComponentId<TArgs>()...};
        return createQuery(with);
    }

    template<class T>
    entity_t World::createModule()
    {
        auto name = trimName(typeid(std::remove_reference_t<T>).name());
        auto modId = newEntity(name.c_str()).add<ecs::ModuleComponent>();

        return modId;
    }

    template<class T>
    entity_t World::getModule()
    {
        auto name = trimName(typeid(std::remove_reference_t<T>).name());
        auto mod = lookup(name);
        return mod.id;
    }

    template<class T>
    T * World::getModuleObject()
    {
        entity_t module = getModule<T>();

        if (!has<ModuleComponent>(module)) {
            return nullptr;
        }
        auto m = getUpdate<ModuleComponent>(module);

        return static_cast<T *>(m->modulePtr);
    }

    class ActiveSystem
    {
        World * world;
    public:
        ActiveSystem(World * w, System * s)
            : world(w)
        {
            w->activeSystem = s;
        }

        ~ActiveSystem()
        {
            world->activeSystem = nullptr;
        }
    };

    class ModuleBase
    {
    protected:
        World * world_;
        const entity_t moduleId;

    public:
        explicit ModuleBase(World * world, entity_t moduleId)
            : world_(world)
              , moduleId(moduleId)
        {
            assert(world_->isAlive(moduleId));
            assert(world_->has<ModuleComponent>(moduleId));
            world->setModuleObject(moduleId, this);
        }

        virtual ~ModuleBase()
        {
            world_->destroy(moduleId);
        }

        void enable()
        {
            world_->setModuleEnabled(moduleId, true);
            onEnabled();
        }

        void disable()
        {
            world_->setModuleEnabled(moduleId, false);
            onDisabled();
        };

        virtual void onDisabled()
        {}

        virtual void onEnabled()
        {}

        entity_t getModuleId() const
        {
            return moduleId;
        }

        template<class T>
        T * getObject()
        {
            auto p = world_->getModuleObject<T>();
            assert(p);
            return p;
        }
    };

    template<class... Comp>
    std::vector<component_id_t> World::makeComponentList()
    {
        return std::vector<component_id_t>{getComponentId<Comp>()...};
    }

    template<class T>
    void World::update(entity_t id, std::function<void(T *)> && f)
    {
        T * v = getUpdate<T>(id);
        if (v == nullptr) {
            return;
        }
        f(v);

        auto cd = getUpdate<Component>(getComponentId<T>());
        postEntity(id, cd->onUpdates);
    }

    template<class T>
    void World::addRemoveTrigger(entity_t id)
    {
        auto compId = getComponentId<T>();
        addRemoveTrigger(compId, id);
    }

    template<class T>
    void World::addAddTrigger(entity_t id)
    {
        auto compId = getComponentId<T>();
        addAddTrigger(compId, id);
    }

    template<class T>
    void World::addUpdateTrigger(entity_t id)
    {
        auto compId = getComponentId<T>();
        addUpdateTrigger(compId, id);
    }

    template<class T>
    void World::removeRemoveTrigger(entity_t id)
    {
        auto compId = getComponentId<T>();
        removeRemoveTrigger(compId, id);
    }
}
