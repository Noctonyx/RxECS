#include "World.h"

#include <cassert>
#include <deque>

#include "Query.h"
#include "QueryResult.h"
#include "System.h"
#include "QueryImpl.h"
#include "Stream.h"
#include "EntityHandle.h"
#include "EntityImpl.h"

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
        auto v = std::type_index(typeid(Component));
        componentMap.emplace(v, componentBootstrapId);
        componentBootstrap = {
            typeid(Component).name(),
            sizeof(Component), alignof(Component),
            componentConstructor<Component>,
            componentDestructor<Component>,
            componentCopy<Component>,
            componentMove<Component>
        };

        set(componentBootstrapId, componentBootstrapId, &componentBootstrap);
        set<Name>(getComponentId<Component>(), {.name="Component"});

        // Query to find queries
        queryQuery = createQuery<Query>().id;

        // query to find systems
        systemQuery = createQuery<System>().withRelation<SetForSystem, SystemSet>().id;
        systemGroupQuery = createQuery<SystemGroup>().id;
        streamQuery = createQuery<StreamComponent>().id;

        singletonId = newEntity().id;
    }

    World::~World()
    {
        getResults(streamQuery).each<StreamComponent>([this](EntityHandle, StreamComponent * s)
        {
            //                removeDeferred<StreamComponent>(e.id);
            delete s->ptr;
        });
        //      executeDeferred();

        for (auto & [k, v]: tables) {
            delete v;
        }
        tables.clear();
    }

    EntityHandle World::newEntity(const char * name)
    {
        while (recycleStart < entities.size()) {
            if (entities[recycleStart].alive == false) {
                entities[recycleStart].alive = true;
                entities[recycleStart].archetype = am.emptyArchetype;
                auto id = makeId(static_cast<uint32_t>(recycleStart),
                                 entities[recycleStart].version);
                tables[am.emptyArchetype]->addEntity(id);
                return EntityHandle{id, this};
            }
            recycleStart++;
        }

        auto i = static_cast<uint32_t>(entities.size());
        const uint32_t v = 0;

        auto & x = entities.emplace_back();
        x.alive = true;
        x.version = 0;
        x.archetype = am.emptyArchetype;
        auto id = makeId(i, v);
        tables[am.emptyArchetype]->addEntity(id);

        if (name) {
            nameIndex[name] = id;
            set<Name>(id, {name});
        }
        return EntityHandle{id, this};
    }

    EntityHandle World::instantiate(entity_t prefab)
    {
        const auto prefabAt = getEntityArchetype(prefab);
        auto trans = am.removeComponentFromArchetype(prefabAt, getComponentId<Prefab>());

        auto e = newEntity();

        ensureTableForArchetype(trans.to_at);
        Table::copyEntity(this, tables[prefabAt], tables[trans.to_at], prefab, e.id, trans);
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
        const auto v = version(id);
        const auto i = index(id);
        (void) v;
        assert(isAlive(id));
        assert(i >= 0);
        assert(i < entities.size());
        assert(entities[i].version == v);

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
        deferredCommands.push_back({DeferredCommandType::Destroy, id, 0, nullptr});
    }

    void World::add(const entity_t id, const component_id_t componentId)
    {
        const auto at = getEntityArchetype(id);
        const auto & newAt = am.addComponentToArchetype(at, componentId);

        moveEntity(id, at, newAt);
    }

    void World::addDeferred(const entity_t id, const component_id_t componentId)
    {
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
        if (componentId == getComponentId<Name>()) {
            auto np = get<Name>(id);
            nameIndex.erase(np->name);
        }

        const auto at = getEntityArchetype(id);
        const auto & newAt = am.removeComponentFromArchetype(at, componentId);
        moveEntity(id, at, newAt);
    }

    void World::removeDeferred(entity_t id, component_id_t componentId)
    {
        deferredCommands.push_back({DeferredCommandType::Remove, id, componentId, nullptr});
    }

    const void * World::get(entity_t id, component_id_t componentId, bool inherited)
    {
        if (!isAlive(id)) {
            return nullptr;
        }
        auto at = getEntityArchetype(id);
        auto table = tables[at];

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

        auto at = getEntityArchetype(id);
        auto table = tables[at];

        void * ptr = table->getUpdateComponent(id, componentId);
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
        auto table = tables[at];

        table->setComponent(id, componentId, ptr);
    }

    void World::setDeferred(entity_t id, component_id_t componentId, void * ptr)
    {
        deferredCommands.push_back({DeferredCommandType::Set, id, componentId, ptr});
    }

    void World::addSingleton(component_id_t componentId)
    {
        add(singletonId, componentId);
    }

    bool World::hasSingleton(component_id_t componentId)
    {
        auto at = getEntityArchetype(singletonId);
        return am.archetypes[at].components.find(componentId) != am.archetypes[at].components.end();
    }

    void World::removeSingleton(component_id_t componentId)
    {
        remove(singletonId, componentId);
    }

    void World::setSingleton(component_id_t componentId, const void * ptr)
    {
        set(singletonId, componentId, ptr);
    }

    const void * World::getSingleton(component_id_t componentId)
    {
        return get(singletonId, componentId);
    }

    void * World::getSingletonUpdate(component_id_t componentId)
    {
        return getUpdate(singletonId, componentId);
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

    QueryBuilder World::createQuery(const std::set<component_id_t> & with)
    {
        auto q = newEntity();
        q.set<Query>(Query{.with = with, .without = {getComponentId<Prefab>()}});

        auto aq = q.getUpdate<Query>();
        aq->recalculateQuery(this);
        return QueryBuilder{q.id, this};
    }

    SystemBuilder World::createSystem(const char * name)
    {
        auto s = newEntity();
        s.set<System>(System{.query = 0, .world = this});
        s.set<Name>({.name = name});
        nameIndex[name] = s.id;

        markSystemsDirty();

        return SystemBuilder{s.id, 0, 0, this, {}};
    }

    void World::deleteQuery(queryid_t q)
    {
        destroy(q);
    }

    void World::deleteSystem(systemid_t s)
    {
        markSystemsDirty();

        auto sys = get<System>(s);
        if (isAlive(sys->query)) {
            deleteQuery(sys->query);
        }
        destroy(s);
    }

    QueryResult World::getResults(queryid_t q)
    {
        assert(isAlive(q));
        assert(has<Query>(q));

        auto aq = get<Query>(q);

        return QueryResult(this, aq->tables, aq->with, aq->relations, aq->singleton,
                           aq->inheritamce);
    }

    void World::executeSystemGroup(entity_t pg)
    {
        auto gd = getUpdate<SystemGroup>(pg);

        float savedDelta = deltaTime_;

        if (gd->fixed) {
            gd->delta += deltaTime_;
            deltaTime_ = gd->rate;
        }
        do {
            if (gd->fixed) {
                if (gd->delta >= gd->rate) {
                    gd->delta -= gd->rate;
                } else {
                    break;
                }
            }
            for (auto s: systemOrder[pg]) {
                if (isAlive(s)) {
                    if (auto system = get<System>(s); system) {
                        if (system->query) {
                            auto res = getResults(system->query);
                            system->queryProcessor(res);
                        } else if (system->stream) {
                            system->streamProcessor(getStream(system->stream));
                        } else {
                            system->executeProcessor();
                        }
                        //executeDeferred();
                    }
                }
            }
        } while (gd->fixed && gd->delta >= gd->rate);

        deltaTime_ = savedDelta;
    }

    void World::step(float delta)
    {
        deltaTime_ = delta;
#if 0
        bool anyDirty = true;

        getResults(systemQuery).each<System>([&anyDirty](World *, entity_t, System * s)
        {
            anyDirty |= s->dirtyOrder;
        });

        if (anyDirty) {
#endif
        recalculateSystemOrder();
#if 0
        }
#endif

        for (auto pg: pipelineGroupSequence) {
            executeSystemGroup(pg);
            executeDeferred();
        }

        getResults(streamQuery).each<StreamComponent>([](EntityHandle, StreamComponent * s)
        {
            s->ptr->clear();
        });
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
        std::string nm = "Entity#" + std::to_string(index(id)) + ":" + std::to_string(
            version(id));
        return nm;
    }

    Archetype & World::getEntityArchetypeDetails(entity_t id)
    {
        return am.getArchetypeDetails(getEntityArchetype(id));
    }

    uint32_t World::getEntityArchetype(entity_t id) const
    {
        //assert(isAlive(id));
        auto & ee = entities[index(id)];
        return ee.archetype;
    }

    void World::moveEntity(entity_t id, uint32_t from, const ArchetypeTransition & trans)
    {
        // assert(isAlive(id));
        assert(entities[index(id)].archetype == from);
        entities[index(id)].archetype = trans.to_at;

        ensureTableForArchetype(trans.to_at);

        Table::moveEntity(this, tables[from], tables[trans.to_at], id, trans);
    }

    void World::addTableToActiveQueries(Table * table, uint32_t aid)
    {
        auto & ad = am.getArchetypeDetails(aid);
        /* This will only be 0 during world bootstrap, ie when we are adding queryquery */
        if (queryQuery) {
            getResults(queryQuery)
                .each<Query>([&table, &ad](EntityHandle, Query * aq)
                    {
                        assert(aq);
                        if (aq->interestedInArchetype(ad)) {
                            aq->tables.push_back(table);
                        }
                    }
                );
        }
    }

    void World::ensureTableForArchetype(uint32_t aid)
    {
        if (tables.find(aid) != tables.end()) {
            return;
        }

        auto t = new Table(this, aid);
        tables[aid] = t;

        addTableToActiveQueries(t, aid);
    }

    WorldIterator World::begin()
    {
        return WorldIterator{this, am.archetypes.begin()};
    }

    WorldIterator World::end()
    {
        return WorldIterator{this, am.archetypes.end()};
    }

    void World::recalculateGroupSystemOrder(entity_t group, std::vector<entity_t> systems)
    {
        systemOrder[group].clear();

        std::unordered_map<entity_t, uint32_t> labelCounts;
        std::unordered_map<entity_t, uint32_t> labelPreCounts;
        std::deque<std::pair<entity_t, const System *>> toProcess;
        std::unordered_map<entity_t, std::set<entity_t>> ready;

        for (auto & s: systems) {
            auto sys = get<System>(s);

            if (sys->enabled) {
                toProcess.push_back({s, sys});
            }
        }

        for (auto & [e, s]: toProcess) {
            for (auto & l: s->labels) {
                labelCounts[l]++;
            }

            for (auto & l: s->befores) {
                labelPreCounts[l]++;
            }
        }

        while (!toProcess.empty()) {
            auto [entity, system] = toProcess.front();

            bool canProcess = true;

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

            if (!canProcess) {
                auto ns = toProcess.front();
                toProcess.pop_front();
                toProcess.push_back(ns);
                continue;
            }

            for (auto & af: system->labels) {
                labelCounts[af]--;
            }
            for (auto & bf: system->befores) {
                labelPreCounts[bf]--;
            }
            systemOrder[group].push_back(entity);
            //getUpdate<System>(entity)->dirtyOrder = false;
            toProcess.pop_front();
        }
    }

    void World::recalculateSystemOrder()
    {
        if (!systemOrderDirty) {
            return;
        }

        systemOrder.clear();
        std::unordered_map<entity_t, std::vector<entity_t>> systems;

        getResults(systemQuery).each<System, SystemSet>(
            [&](EntityHandle e, System * s, const SystemSet * set)
            {
                if ((set && !set->enabled) || !s->enabled) {
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

        std::ranges::sort(grps, [](std::pair<SystemGroup *, entity_t> a,
                                   std::pair<SystemGroup *, entity_t> b)
        {
            return a.first->sequence < b.first->sequence;
        });

        pipelineGroupSequence.clear();
        for (auto gg: grps) {
            pipelineGroupSequence.push_back(gg.second);
        }

        for (auto g: pipelineGroupSequence) {
            recalculateGroupSystemOrder(g, systems[g]);
        }
    }
}
