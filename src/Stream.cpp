#include "Stream.h"
#include "Column.h"

namespace ecs
{
    Stream::Stream(component_id_t componentId, World * world)
        : componentId(componentId)
        , world(world)
    {
        column = new Column(componentId, world);
    }

    Stream::~Stream()
    {
        active.clear();
        delete column;
    }

    void Stream::clear()
    {
        active.clear();
        column->clear();
    }
}
