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

#include "World.h"

#include <cassert>
#include <deque>
#include <atomic>

#include "Query.h"
#include "QueryResult.h"
#include "System.h"
#include "QueryImpl.h"
#include "Stream.h"
#include "EntityHandle.h"
#include "EntityImpl.h"
#include "Filter.h"
#include "EntityQueue.h"

namespace ecs
{
    World::World()
    {
        entities.resize(1);
        entities[0].alive = true;
        recycleStart = 0;
        deltaTime_ = 0.f;
        tables.clear();

        ensureTableForArchetype(am.emptyArchetype);
        tables[0]->addEntity(0);

        componentBootstrapId = newEntity().id;
        auto v = type_id<Component>(); // std::type_index(typeid(Component));
        componentMap.emplace(v, componentBootstrapId);
        componentBootstrap = {
            trimName(typeid(Component).name()),
            sizeof(Component), alignof(Component),
            componentConstructor<Component>,
            componentDestructor<Component>,
            componentCopy<Component>,
            componentMove<Component>,
            false,
            componentAllocator<Component>,
            componentDeallocator<Component>
        };

        set(componentBootstrapId, componentBootstrapId, &componentBootstrap);
        set<Name>(getComponentId<Component>(), {.name = "Component"});
        set<Name>(getComponentId<Name>(), {.name = "Name"});

        // Query to find queries
        queryQuery = createQuery<Query>().id;

        // query to find systems
        systemQuery = createQuery<System>()
                      .withRelation<SetForSystem, SystemSet>()
                      .withRelation<HasModule, ModuleComponent>().id;
        systemGroupQuery = createQuery<SystemGroup>().id;
        streamQuery = createQuery<StreamComponent>().id;
    }

    World::~World()
    {
        getResults(streamQuery).each<StreamComponent>(
            [](EntityHandle, StreamComponent * s)
            {
                delete s->ptr;
            }
        );

        for (auto & [k, v]: singletons) {
            auto cd = getComponentDetails(k);
            cd->componentDestructor(v, cd->size, 1);
            delete[] static_cast<char *>(v);
        }
#if 0
        for (auto &[k, v]: tables) {
            delete v;
        }
#endif
        tables.clear();
    }

    EntityHandle World::newEntity(const char * name)
    {
        while (recycleStart < entities.size()) {
            if (!entities[recycleStart].alive) {
                entities[recycleStart].alive = true;
                entities[recycleStart].archetype = am.emptyArchetype;
                entities[recycleStart].updateSequence = 0;
                auto id = makeId(
                    static_cast<uint32_t>(recycleStart),
                    entities[recycleStart].version
                );
                tables[am.emptyArchetype]->addEntity(id);
                if (name) {
                    nameIndex[name] = id;
                    set<Name>(id, {name});
                }
                return EntityHandle{id, this};
            }
            recycleStart++;
        }

        auto i = static_cast<uint32_t>(entities.size());
        const uint32_t v = 0;

        auto & x = entities.emplace_back();
        x.alive = true;
        x.version = v;
        x.archetype = am.emptyArchetype;
        x.updateSequence = 0;
        auto id = makeId(i, v);
        tables[am.emptyArchetype]->addEntity(id);

        if (name) {
            nameIndex[name] = id;
            set<Name>(id, {name});
        }
        return EntityHandle{id, this};
    }

    EntityHandle World::newEntityReplace(const char * name)
    {
        auto e = lookup(name);
        if (isAlive(e)) {
            e.destroy();
        }
        return newEntity(name);
    }

    EntityHandle World::instantiate(entity_t prefab)
    {
        const auto prefabAt = getEntityArchetype(prefab);
        auto trans = am.startTransition(prefabAt);

        am.removeComponentFromArchetype(getComponentId<Prefab>(), trans);
        if (has<Name>(prefab)) {
            am.removeComponentFromArchetype(getComponentId<Name>(), trans);
        }

        auto e = newEntity();
        tables[getEntityArchetype(e.id)]->removeEntity(e.id);

        ensureTableForArchetype(trans.to_at);
        Table::copyEntity(this, tables[prefabAt].get(), tables[trans.to_at].get(), prefab, e.id,
                          trans);
        entities[index(e.id)].archetype = trans.to_at;
        return e;
    }

    EntityHandle World::lookup(const char * name)
    {
        if (nameIndex.contains(name)) {
            return {nameIndex[name], this};
        }

        return {0, this};
    }

    EntityHandle World::lookup(const std::string & name)
    {
        return lookup(name.c_str());
    }

    bool World::isAlive(const entity_t id) const
    {
        const auto v = version(id);
        const auto i = index(id);

        if (i == 0) {
            return false;
        }
        if (i >= entities.size()) {
            return false;
        }
        if (v != entities[i].version) {
            return false;
        }
        return entities[i].alive;
    }

    void World::destroy(const entity_t id)
    {
        if (has<HasEntityQueue>(id)) {
            destroyEntityQueue(id);
            return;
        }
        if (has<System>(id)) {
            deleteSystem(id);
            return;
        }

        if (has<Component>(id)) {
            removeDynamicComponent(id);
        }

        const auto v = version(id);
        const auto i = index(id);
        (void) v;
        assert(isAlive(id));
        assert(i >= 0);
        assert(i < entities.size());
        assert(entities[i].version == v);
        assert(!has<Component>(id));

        if (has<Name>(id)) {
            auto n = get<Name>(id);
            auto it = nameIndex.find(n->name);
            if (it != nameIndex.end()) {
                nameIndex.erase(it);
            }
        }

        const auto at = getEntityArchetype(id);
        tables[at]->removeEntity(id);

        entities[i].alive = false;
        entities[i].version++;
        if (i < recycleStart) {
            recycleStart = i;
        }
    }

    void World::destroyDeferred(const entity_t id)
    {
        std::lock_guard guard(deferredMutex);
        deferredCommands.push_back({DeferredCommandType::Destroy, id, 0, nullptr});
    }

    void World::add(const entity_t id, const component_id_t componentId)
    {
        if (has(id, componentId)) {
            return;
        }
        const auto at = getEntityArchetype(id);
        auto trans = am.startTransition(at);

        am.addComponentToArchetype(componentId, trans);
        setEntityUpdateSequence(id);
        moveEntity(id, at, trans);

        auto * cd = getUpdate<Component>(componentId);
        postEntity(id, cd->onAdds);
    }

    void World::addDeferred(const entity_t id, const component_id_t componentId)
    {
        std::lock_guard guard(deferredMutex);
        deferredCommands.push_back({DeferredCommandType::Add, id, componentId, nullptr});
    }

    bool World::has(const entity_t id, component_id_t componentId)
    {
        assert(isAlive(id));

        auto at = getEntityArchetype(id);
        return am.archetypes[at].components.find(componentId) != am.archetypes[at].components.end();
    }

    void World::remove(entity_t id, component_id_t componentId)
    {
        if (!has(id, componentId)) {
            return;
        }
        if (componentId == getComponentId<Name>()) {
            auto np = get<Name>(id);
            nameIndex.erase(np->name);
        }

        const auto at = getEntityArchetype(id);
        auto trans = am.startTransition(at);
        am.removeComponentFromArchetype(componentId, trans);
        moveEntity(id, at, trans);
        setEntityUpdateSequence(id);

        auto * cd = getUpdate<Component>(componentId);
        postEntity(id, cd->onRemove);
    }

    void World::removeDeferred(entity_t id, component_id_t componentId)
    {
        std::lock_guard guard(deferredMutex);
        deferredCommands.push_back({DeferredCommandType::Remove, id, componentId, nullptr});
    }

    const void * World::get(entity_t id, component_id_t componentId, bool inherited)
    {
        if (!isAlive(id)) {
            return nullptr;
        }
        auto at = getEntityArchetype(id);
        auto table_it = tables.find(at);
        auto table = table_it->second.get();

        const void * ptr = table->getComponent(id, componentId);
        if (ptr || !inherited) {
            return ptr;
        }

        const auto i = get<InstanceOf>(id);
        if (i) {
            if (isAlive(i->entity)) {
                return get(i->entity, componentId, true);
            }
        }
        return nullptr;
    }

    void * World::getUpdate(entity_t id, component_id_t componentId)
    {
        if (!isAlive(id)) {
            return nullptr;
        }

        if (!has(id, componentId)) {
            return nullptr;
        }

        auto at = getEntityArchetype(id);
        Table * table = tables[at].get();

        void * ptr = table->getUpdateComponent(id, componentId);
        setEntityUpdateSequence(id);
        return ptr;
    }

    void World::set(entity_t id, component_id_t componentId, const void * ptr)
    {
        if (componentId != componentBootstrapId && componentId == getComponentId<Name>()) {
            auto np = static_cast<const Name *>(ptr);
            nameIndex[np->name] = id;
        }

        auto at = getEntityArchetype(id);
        auto & ad = am.getArchetypeDetails(at);
        if (ad.components.find(componentId) == ad.components.end()) {
            add(id, componentId);
            at = getEntityArchetype(id);
        }
        assert(tables.contains(at));
        auto table = tables[at].get();

        table->setComponent(id, componentId, ptr);
        setEntityUpdateSequence(id);

        auto cd = getUpdate<Component>(componentId);
        postEntity(id, cd->onUpdates);
    }

    void World::setDeferred(entity_t id, component_id_t componentId, void * ptr)
    {
        std::lock_guard guard(deferredMutex);
        deferredCommands.push_back({DeferredCommandType::Set, id, componentId, ptr});
    }

    void World::addSingleton(const component_id_t componentId)
    {
        if (hasSingleton(componentId)) {
            return;
        }
        auto cd = getComponentDetails(componentId);
        char * cp = new char[cd->size];
        cd->componentConstructor(cp, cd->size, 1);
        singletons[componentId] = cp;
    }

    bool World::hasSingleton(const component_id_t componentId) const
    {
        if (singletons.contains(componentId)) {
            return true;
        }
        return false;
    }

    void World::removeSingleton(const component_id_t componentId)
    {
        if (!hasSingleton(componentId)) {
            return;
        }

        auto cd = getComponentDetails(componentId);
        auto ptr = singletons[componentId];

        cd->componentDestructor(ptr, cd->size, 1);

        delete[] static_cast<char *>(ptr);

        singletons.erase(componentId);
    }

    void World::setSingleton(const component_id_t componentId, const void * ptr)
    {
        addSingleton(componentId);

        const auto cd = getComponentDetails(componentId);
        const auto p = singletons[componentId];

        cd->componentCopier(ptr, p, cd->size, 1);
    }

    const void * World::getSingleton(const component_id_t componentId)
    {
        if (!hasSingleton(componentId)) {
            return nullptr;
        }
        return singletons[componentId];
    }

    void * World::getSingletonUpdate(const component_id_t componentId)
    {
        if (!hasSingleton(componentId)) {
            return nullptr;
        }
        return singletons[componentId];
    }

    Stream * World::getStream(component_id_t id)
    {
        if (has<StreamComponent>(id)) {
            return get<StreamComponent>(id)->ptr;
        }

        auto s = new Stream(id, this);

        set<StreamComponent>(id, {s});

        return s;
    }

    const Component * World::getComponentDetails(component_id_t id)
    {
        if (id == componentBootstrapId) {
            return &componentBootstrap;
        }
        return get<Component>(id);
    }

    component_id_t World::createDynamicComponent(entity_t entityId)
    {
        set<Component>(
            entityId, {
                description(entityId),
                sizeof(std::remove_reference_t<DynamicComponent>),
                alignof(std::remove_reference_t<DynamicComponent>),
                componentConstructor<std::remove_reference_t<DynamicComponent>>,
                componentDestructor<std::remove_reference_t<DynamicComponent>>,
                componentCopy<std::remove_reference_t<DynamicComponent>>,
                componentMove<std::remove_reference_t<DynamicComponent>>,
                false,
                componentAllocator<std::remove_reference_t<DynamicComponent>>,
                componentDeallocator<std::remove_reference_t<DynamicComponent>>
            }
        );

        return entityId;
    }

    void World::removeDynamicComponent(entity_t entityId)
    {
        if (!has<Component>(entityId)) {
            return;
        }

        auto q = createQuery({entityId});
        auto res = getResults(q.id);
        assert(res.count() == 0);
        deleteQuery(q.id);

        std::vector<uint16_t> removeList;

        for (auto & [k, t]: tables) {

            if (t == nullptr || t->hasComponent(entityId)) {
                removeList.push_back(k);
            }
        }

        for (auto & r: removeList) {
            auto at = am.archetypes[r];
            am.archetypeMap.erase(at.hash_value);
            //auto tab = tables[r];
            //delete tab;
            removeTableFromActiveQueries(tables[r].get());
            tables.erase(r);
        }
        am.addCache.clear();
        am.removeCache.clear();

        remove<Component>(entityId);
    }

    QueryBuilder World::createQuery(const std::set<component_id_t> & with)
    {
        auto q = newEntity();
        q.set<Query>(Query{
            .with = with, .without = {getComponentId<Prefab>(), getComponentId<PendingDelete>()}
        });

        update<Query>(
            q.id, [this](Query * aq)
            {
                aq->recalculateQuery(this);
            }
        );
        return QueryBuilder{q.id, this};
    }

    SystemBuilder World::createSystem(const char * name)
    {
        if (lookup(name).isAlive()) {
            auto e = lookup(name);
            if (e.has<System>()) {
                deleteSystem(e);
            } else {
                destroy(e);
            }
        }
        auto s = newEntity();
        s.set<System>(System{.query = 0, .world = this});
        if (name) {
            s.set<Name>({.name = name});
            nameIndex[name] = s.id;
        }

        if (!moduleScope.empty()) {
            auto module = moduleScope.top();
            s.set<HasModule>({{module}});
        }

        markSystemsDirty();

        return SystemBuilder{SystemType::None, s.id, 0, 0, this, {}, {}};
    }

    void World::deleteQuery(queryid_t q)
    {
        destroy(q);
    }

    void World::deleteSystem(systemid_t s)
    {
        if (!isAlive(s)) {
            return;
        }
        markSystemsDirty();

        auto sys = get<System>(s);
        if (isAlive(sys->query)) {
            deleteQuery(sys->query);
        }
        remove<System>(s);
        destroy(s);
    }

    QueryResult World::getResults(queryid_t q)
    {
        assert(isAlive(q));
        assert(has<Query>(q));

        auto aq = get<Query>(q);

        return QueryResult(
            this, aq->tables, aq->with, aq->relations, aq->singleton,
            aq->inheritance, aq->thread
        );
    }

    std::optional<JobInterface::JobHandle> World::executeSystem(systemid_t sys)
    {
        if (isAlive(sys)) {
            if (auto system = getUpdate<System>(sys); system) {

                if (system->interval > 0.01f) {
                    system->intervalElapsed += deltaTime_;
                    if (system->intervalElapsed < system->interval) {
                        system->count = 0;
                        system->executionTime = 0.f;
                        return std::nullopt;
                    }
                    system->intervalElapsed -= system->interval;
                }
                system->startTime = std::chrono::high_resolution_clock::now();
                //ActiveSystem as(this, system);

                if (system->query) {
                    auto res = getResults(system->query);
                    system->count = res.count();
                    if (system->queryProcessor && system->count > 0) {
                        if (system->updatesOnly) {
                            res.onlyUpdatedAfter(system->lastRunSequence);
                        }
                        system->count = system->queryProcessor(res);
                    }
                    if (system->executeIfNoneProcessor && system->count == 0) {
                        if (system->thread) {
                            return jobInterface->create(
                                [=, this]()
                                {
                                    system->executeIfNoneProcessor(this);
                                    const auto end = std::chrono::high_resolution_clock::now();
                                    system->executionTime = system->executionTime * 0.9f + 0.1f *
                                        std::chrono::duration<
                                            float>(end - system->startTime).count();
                                    return 0;
                                }
                            );
                        } else {
                            system->executeIfNoneProcessor(this);
                        }
                    }
                } else if (system->stream) {
                    auto str = getStream(system->stream);
                    system->count = str->active.size();
                    system->streamProcessor(getStream(system->stream));
                } else if (system->entityQueue) {
                    auto eq = getEntityQueue(system->entityQueue);
                    system->count = eq->entries.size();
                    eq->each([&](EntityHandle e)
                    {
                        return system->queueProcessor(e);
                    });
                } else {
                    system->count = 1;
                    if (system->thread) {
                        return jobInterface->create(
                            [=, this]()
                            {
                                system->executeProcessor(this);
                                const auto end = std::chrono::high_resolution_clock::now();
                                system->executionTime = system->executionTime * 0.9f + 0.1f *
                                    std::chrono::duration<
                                        float>(end - system->startTime).count();
                                return 0;
                            }
                        );
                    } else {
                        system->executeProcessor(this);
                    }
                }
                const auto end = std::chrono::high_resolution_clock::now();
                system->executionTime = system->executionTime * 0.9f + 0.1f * std::chrono::duration<
                    float>(end - system->startTime).count();
            }
        }
        return std::nullopt;
    }

    void World::executeGroupsSystems(entity_t systemGroup)
    {
        using BackgroundSystem = std::tuple<JobInterface::JobHandle, System *, entity_t>;

        std::unordered_map<component_id_t, uint32_t> writeCounts{};
        std::unordered_map<component_id_t, uint32_t> streamWriteCounts{};
        std::unordered_map<entity_t, uint32_t> labelCounts{};
        std::unordered_map<entity_t, uint32_t> labelPreCounts{};

        std::deque<systemid_t> systemsToRun;

        auto grp = getUpdate<SystemGroup>(systemGroup);

        for (auto & [k, v]: grp->writeCounts) {
            writeCounts[k] = v;
        }
        for (auto & [k, v]: grp->streamWriteCounts) {
            streamWriteCounts[k] = v;
        }
        for (auto & [k, v]: grp->labelPreCounts) {
            labelPreCounts[k] = v;
        }
        for (auto & [k, v]: grp->labelCounts) {
            labelCounts[k] = v;
        }
        for (auto & s: grp->systems) {
            if (get<System>(s)->thread) {
                systemsToRun.push_back(s);
            }
        }
        for (auto & s: grp->systems) {
            if (!get<System>(s)->thread) {
                systemsToRun.push_back(s);
            }
        }
        if (systemsToRun.empty()) {
            grp->executionSequence.clear();
            return;
        }
        uint32_t sentinel = 0;

        std::deque<BackgroundSystem> inFlights{};
        std::vector<systemid_t> sequence;

        while (!systemsToRun.empty()) {
            auto system_entity = systemsToRun.front();
            auto system = getUpdate<System>(system_entity);

            bool canProcess = true;

            if (sentinel > systemsToRun.size()) {
                if (inFlights.empty()) {
                    for (auto xx: systemsToRun) {
                        printf("%s\n", description(xx).c_str());
                    }
                    throw std::runtime_error("Systems define a cycle and cannot run");
                } else {
                    //                    YieldProcessor();
                    std::this_thread::yield();
                }
            }

            if (!inFlights.empty()) {
                auto it = inFlights.begin();
                while (it != inFlights.end()) {
                    auto & [j, sys, syse] = *it;

                    if (jobInterface->isComplete(j)) {
                        for (auto & af: sys->labels) {
                            labelCounts[af]--;
                        }
                        for (auto & bf: sys->befores) {
                            labelPreCounts[bf]--;
                        }
                        for (auto & bf: sys->writes) {
                            writeCounts[bf]--;
                        }
                        for (auto & bf: sys->streamWrites) {
                            streamWriteCounts[bf]--;
                        }
                        inFlights.erase(it);
                        break;
                    }
                    it++;
                }
            }

            for (auto & af: system->afters) {
                if (labelCounts[af] > 0) {
                    canProcess = false;
                }
            }

            for (auto & af: system->labels) {
                if (labelPreCounts[af] > 0) {
                    canProcess = false;
                }
            }

            for (auto & af: system->reads) {
                if (!system->writes.contains(af) && writeCounts[af] > 0) {
                    canProcess = false;
                }
            }

            for (auto & af: system->writes) {
                for (auto & [j, s, e]: inFlights) {
                    if (s->writes.contains(af)) {
                        canProcess = false;
                    }
                }
            }

            for (auto & af: system->streamReads) {
                if (streamWriteCounts[af] > 0) {
                    canProcess = false;
                }
            }

            if (!canProcess) {
                auto ns = systemsToRun.front();
                systemsToRun.pop_front();
                systemsToRun.push_back(ns);
                sentinel++;
                continue;
            }
            sentinel = 0;
            auto jr = executeSystem(system_entity);
            sequence.push_back(system_entity);
            if (jr != std::nullopt) {
                inFlights.emplace_back(jr.value(), system, system_entity);
                jobInterface->schedule(jr.value());
                //jobInterface->awaitCompletion(jr.value());
            } else {
                for (auto & af: system->labels) {
                    labelCounts[af]--;
                }
                for (auto & bf: system->befores) {
                    labelPreCounts[bf]--;
                }
                for (auto & bf: system->writes) {
                    writeCounts[bf]--;
                }
                for (auto & bf: system->streamWrites) {
                    streamWriteCounts[bf]--;
                }
            }
            systemsToRun.pop_front();
        }

        while (!inFlights.empty()) {
            auto & [j, sys, syse] = inFlights.front();
            jobInterface->awaitCompletion(j);
            for (auto & af: sys->labels) {
                labelCounts[af]--;
            }
            for (auto & bf: sys->befores) {
                labelPreCounts[bf]--;
            }
            for (auto & bf: sys->writes) {
                writeCounts[bf]--;
            }
            for (auto & bf: sys->streamWrites) {
                streamWriteCounts[bf]--;
            }
            inFlights.pop_front();
        }
        grp->executionSequence = std::move(sequence);
        //const auto end = std::chrono::high_resolution_clock::now();
    }

    void World::executeSystemGroup(entity_t systemGroup)
    {
        updateSequence++;

        auto group_details = getUpdate<SystemGroup>(systemGroup);
        if (group_details->onBegin) {
            group_details->onBegin();
        }

        //OPTICK_EVENT()
        //group_details->executionSequence.clear();

        float savedDelta = deltaTime_;

        if (group_details->fixed) {
            group_details->delta += deltaTime_;
            deltaTime_ = group_details->rate;
        }
        do {
            if (group_details->fixed) {
                if (group_details->delta >= group_details->rate) {
                    group_details->delta -= group_details->rate;
                } else {
                    break;
                }
            }
            executeGroupsSystems(systemGroup);
        } while (group_details->fixed && group_details->delta >= group_details->rate);

        for (auto & sid: group_details->executionSequence) {
            auto sp = getUpdate<System>(sid);
            sp->lastRunSequence = updateSequence;
        }
        deltaTime_ = savedDelta;
        if (group_details->onEnd) {
            group_details->onEnd();
        }
    }

    void World::step(float delta)
    {
        deltaTime_ = delta;
        recalculateSystemOrder();

        for (auto pg: pipelineGroupSequence) {
            float runTime;
            auto gd = getUpdate<SystemGroup>(pg);
            const auto start = std::chrono::high_resolution_clock::now();
            executeSystemGroup(pg);
            const auto systems = std::chrono::high_resolution_clock::now();

            executeDeferred();
            const auto end = std::chrono::high_resolution_clock::now();

            runTime = std::chrono::duration<float>(end - systems).count();
            gd->deferredTime = gd->deferredTime * 0.9f + 0.1f * runTime;
            gd->deferredCount = deferredCommands.size();

            runTime = std::chrono::duration<float>(end - start).count();
            gd->lastTime = gd->lastTime * 0.9f + 0.1f * runTime;
        }

        getResults(streamQuery).each<StreamComponent>(
            [](EntityHandle, StreamComponent * s)
            {
                s->ptr->clear();
            }
        );
    }

    void World::executeDeferred()
    {
        for (auto & command: deferredCommands) {
            switch (command.type) {
            case DeferredCommandType::Add:
                add(command.entity, command.component);
                break;
            case DeferredCommandType::Destroy:
                destroy(command.entity);
                break;
            case DeferredCommandType::Remove:
                remove(command.entity, command.component);
                break;
            case DeferredCommandType::Set:
                {
                    set(command.entity, command.component, command.ptr);
                    auto cd = getComponentDetails(command.component);
                    cd->componentDestructor(command.ptr, cd->size, 1);

                    delete[] static_cast<char *>(command.ptr);
                }
                break;
            }
        }
        deferredCommands.clear();
    }

    std::string World::description(entity_t id)
    {
        const Name * n = get<Name>(id);
        if (n) {
            return n->name;
        }
        std::string nm = "Entity " + std::to_string(index(id)) + ":" + std::to_string(
            version(id));
        return nm;
    }

    Archetype & World::getEntityArchetypeDetails(entity_t id)
    {
        return am.getArchetypeDetails(getEntityArchetype(id));
    }

    void World::setModuleObject(entity_t module, void * ptr)
    {
        if (!has<ModuleComponent>(module)) {
            return;
        }
        auto m = getUpdate<ModuleComponent>(module);
        m->modulePtr = ptr;
    }

    void World::pushModuleScope(entity_t module)
    {
        moduleScope.push(module);
    }

    void World::popModuleScope()
    {
        assert(!moduleScope.empty());
        moduleScope.pop();
    }

    void World::setModuleEnabled(const entity_t module, const bool enabled)
    {
        auto m = getUpdate<ModuleComponent>(module);
        if (m) {
            m->enabled = enabled;
        }
        markSystemsDirty();
    }

    uint16_t World::getEntityArchetype(entity_t id) const
    {
        //assert(isAlive(id));
        auto & ee = entities[index(id)];
        return ee.archetype;
    }

    void World::moveEntity(entity_t id, uint16_t from, const ArchetypeTransition & trans)
    {
        // assert(isAlive(id));
        assert(entities[index(id)].archetype == from);
        entities[index(id)].archetype = trans.to_at;

        ensureTableForArchetype(trans.to_at);

        Table::moveEntity(this, tables[from].get(), tables[trans.to_at].get(), id, trans);
    }

    void World::addTableToActiveQueries(Table * table, uint16_t aid)
    {
        auto & ad = am.getArchetypeDetails(aid);
        /* This will only be 0 during world bootstrap, ie when we are adding queryquery */
        if (queryQuery) {
            getResults(queryQuery)
                .each<Query>(
                    [&table, &ad](EntityHandle, Query * aq)
                    {
                        assert(aq);
                        if (aq->interestedInArchetype(ad)) {
                            aq->tables.push_back(table);
                        }
                    }
                );
        }
    }

    void World::removeTableFromActiveQueries(Table * table)
    {
        if (queryQuery) {
            getResults(queryQuery)
                .each<Query>(
                    [&table](EntityHandle, Query * aq)
                    {
                        assert(aq);
                        auto it = std::find(aq->tables.begin(), aq->tables.end(), table);
                        if (it != aq->tables.end()) {
                            aq->tables.erase(it);
                        }
                    }
                );
        }
    }

    void World::ensureTableForArchetype(uint16_t aid)
    {
        if (tables.find(aid) != tables.end()) {
            return;
        }

        auto t = std::make_unique<Table>(this, aid);
        Table * ptr = t.get();
        tables[aid] = std::move(t);

        addTableToActiveQueries(ptr, aid);
    }

    WorldIterator World::begin()
    {
        return WorldIterator{this, am.archetypes.begin()};
    }

    WorldIterator World::end()
    {
        return WorldIterator{this, am.archetypes.end()};
    }

    void World::recalculateGroupSystemOrder(entity_t group, std::vector<systemid_t> systems)
    {
        auto grp = getUpdate<SystemGroup>(group);
        grp->systems.clear();
        grp->labelPreCounts.clear();
        grp->labelCounts.clear();
        grp->writeCounts.clear();
        grp->streamWriteCounts.clear();

        std::deque<std::pair<entity_t, const System *>> toProcess;
        std::unordered_map<entity_t, std::set<entity_t>> ready;

        for (auto & s: systems) {
            auto sys = get<System>(s);

            if (sys->enabled) {
                toProcess.push_back({s, sys});
                grp->systems.push_back(s);
            }
        }

        for (auto & [e, s]: toProcess) {
            for (auto & l: s->labels) {
                grp->labelCounts[l]++;
            }

            for (auto & l: s->befores) {
                grp->labelPreCounts[l]++;
            }

            for (auto & l: s->writes) {
                grp->writeCounts[l]++;
            }

            for (auto & l: s->streamWrites) {
                grp->streamWriteCounts[l]++;
            }
        }
    }

    std::string World::trimName(const char * n)
    {
        std::string newName = n;
        if (newName.starts_with("struct ")) {
            newName = newName.substr(7);
        }
        if (newName.starts_with("class ")) {
            newName = newName.substr(6);
        }

        return newName;
    }

    void World::recalculateSystemOrder()
    {
        if (!systemOrderDirty) {
            return;
        }

        std::unordered_map<entity_t, std::vector<entity_t>> systems;

        getResults(systemQuery).each<System, SystemSet, ModuleComponent>(
            [&](EntityHandle e, System * s, const SystemSet * set, const ModuleComponent * mod)
            {
                if ((set && !set->enabled) || !s->enabled) {
                    return;
                }
                if (mod && !mod->enabled) {
                    return;
                }

                systems[s->groupId].push_back(e.id);
            }
        );

        std::vector<std::pair<SystemGroup *, entity_t>> grps;

        getResults(systemGroupQuery).each<SystemGroup>(
            [&grps](EntityHandle e, SystemGroup * g)
            {
                grps.push_back({g, e.id});
            }
        );

        std::ranges::sort(
            grps, [](std::pair<SystemGroup *, entity_t> a,
                     std::pair<SystemGroup *, entity_t> b)
            {
                return a.first->sequence < b.first->sequence;
            }
        );

        pipelineGroupSequence.clear();
        for (auto gg: grps) {
            pipelineGroupSequence.push_back(gg.second);
        }

        for (auto g: pipelineGroupSequence) {
            recalculateGroupSystemOrder(g, systems[g]);
        }

        systemOrderDirty = false;
    }

    void World::setEntityUpdateSequence(entity_t id)
    {
        assert(isAlive(id));
        //const auto v = version(id);
        const auto i = index(id);
        entities[i].updateSequence = updateSequence;
    }

    Filter World::createFilter(std::vector<component_id_t> with,
                               std::vector<component_id_t> without)
    {
        std::vector<TableView> tvs;

        for (auto & at: *this) {
            std::vector<component_id_t> with_overlap;
            std::vector<component_id_t> without_overlap;

            std::ranges::set_intersection(at.components, with, std::back_inserter(with_overlap));

            if (with_overlap.size() < with.size()) {
                continue;
            }

            std::ranges::set_intersection(at.components, without,
                                          std::back_inserter(without_overlap));
            if (!without_overlap.empty()) {
                continue;
            }

            Table * tab = tables[at.id].get();

            tvs.push_back({this, tab, tab->lastUpdateTimestamp, 0, tab->entities.size()});
        }

        return Filter(this, tvs);
    }

    void World::setAsParent(entity_t id)
    {
        createDynamicComponent(id);
    }

    void World::removeAsParent(entity_t id)
    {
        removeDynamicComponent(id);
    }

    void World::postEntity(entity_t id, std::vector<entity_t> & posts)
    {
        for (auto ona: posts) {
            if (queues.contains(ona)) {
                queues[ona]->add(id);
            }
        }
    }

    void World::addRemoveTrigger(component_id_t componentId, entity_t entity)
    {
        auto cd = getUpdate<Component>(componentId);
        cd->onRemove.push_back(entity);
    }

    void World::removeRemoveTrigger(component_id_t componentId, entity_t entity)
    {
        auto cd = getUpdate<Component>(componentId);
        auto it = std::find(cd->onRemove.begin(), cd->onRemove.end(), entity);
        if (it != cd->onRemove.end()) {
            cd->onRemove.erase(it);
        }
    }

    void World::addAddTrigger(component_id_t componentId, entity_t entity)
    {
        auto cd = getUpdate<Component>(componentId);
        cd->onAdds.push_back(entity);
    }

    void World::addUpdateTrigger(component_id_t componentId, entity_t entity)
    {
        auto cd = getUpdate<Component>(componentId);
        cd->onUpdates.push_back(entity);
    }

    void World::removeAddTrigger(component_id_t componentId, entity_t entity)
    {
        auto cd = getUpdate<Component>(componentId);
        auto it = std::find(cd->onAdds.begin(), cd->onAdds.end(), entity);
        if (it != cd->onAdds.end()) {
            cd->onAdds.erase(it);
        }
    }

    EntityQueue * World::getEntityQueue(entity_t id) const
    {
        auto it = queues.find(id);
        if (it == queues.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    EntityQueueHandle World::createEntityQueue(const char * name)
    {
        auto qid = newEntity(name).add<HasEntityQueue>();
        auto q = std::make_unique<EntityQueue>(this);

        queues[qid] = std::move(q);

        return EntityQueueHandle{qid, this};
    }

    void World::destroyEntityQueue(entity_t eq)
    {
        if (!has<HasEntityQueue>(eq)) {
            destroy(eq);
        }

        auto it = queues.find(eq);
        assert(it != queues.end());
        queues.erase(it);
        remove<HasEntityQueue>(eq);
        destroy(eq);
    }
}
