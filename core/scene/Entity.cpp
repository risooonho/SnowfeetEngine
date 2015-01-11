#include <core/util/Log.hpp>
#include <core/util/assert.hpp>
#include <sstream>

#include "Entity.hpp"
#include "Scene.hpp"

#define SN_JSON_ENTITY_CHILDREN_TAG "_children"

namespace sn
{

//------------------------------------------------------------------------------
void Entity::setName(const std::string & name)
{
    m_name = name;
}

//------------------------------------------------------------------------------
void Entity::setFlag(u32 position, bool v)
{
    m_flags.set(position, v);
}

//------------------------------------------------------------------------------
bool Entity::isEnabled() const
{
    if (m_flags.at(SN_EF_ENABLED))
        return r_parent ? r_parent->isEnabled() : true;
    else
        return false;
}

//------------------------------------------------------------------------------
void Entity::setEnabled(bool e)
{
    m_flags.set(SN_EF_ENABLED, e);
}

//------------------------------------------------------------------------------
void Entity::setUpdatable(bool updatable, s16 order, s16 layer)
{
    if (getFlag(SN_EF_UPDATABLE) ^ updatable)
    {
        Scene * scene = getScene();
        if (scene)
        {
            if (updatable)
                scene->registerUpdatableEntity(this, order, layer);
            else
                scene->unregisterUpdatableEntity(this);
        }
        else
        {
            SN_ERROR("Entity::setUpdatable: scene not found from entity " << toString());
        }
    }
    else
    {
        SN_WARNING("Entity::setUpdatable: no change made (entity " << toString() << ")");
    }
}

//------------------------------------------------------------------------------
void Entity::addTag(const std::string & tag)
{
    if (m_tags.insert(tag).second)
    {
        Scene * scene = getScene();
        if (scene)
            scene->registerTaggedEntity(this, tag);
        else
            SN_ERROR("Entity::addTag: scene not found from entity " << toString());
    }
    else
    {
        SN_WARNING("Entity::addTag: tag " << tag << " already set on entity " << toString());
    }
}

//------------------------------------------------------------------------------
void Entity::removeTag(const std::string & tag)
{
    if (m_tags.erase(tag))
    {
        Scene * scene = getScene();
        if (scene)
            scene->unregisterTaggedEntity(this, tag);
        else
            SN_ERROR("Entity::removeTag: scene not found from entity " << toString());
    }
    else
    {
        SN_WARNING("Entity::addTag: tag " << tag << " already set on entity " << toString());
    }
}

//------------------------------------------------------------------------------
std::string Entity::toString() const
{
    std::stringstream ss;
    if (m_name.empty())
        ss << "[unnamed_entity]";
    else
        ss << m_name;
    return ss.str();
}

//------------------------------------------------------------------------------
void Entity::setParent(Entity * newParent)
{
    if (r_parent)
        r_parent->removeChild(this);
    r_parent = newParent;
    if (newParent)
        newParent->addChild(this);
    r_scene = nullptr;
}

//------------------------------------------------------------------------------
Entity * Entity::getRoot() const
{
    if (r_parent)
    {
        Entity * parent = r_parent;
        while (parent->getParent())
            parent = parent->getParent();
        return parent;
    }
    else
    {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
Scene * Entity::getScene() const
{
    if (r_scene == nullptr)
    {
        Entity * root = getRoot();
        if (root && root->isInstanceOf<Scene>())
        {
            r_scene = (Scene*)root;
        }
    }
    return r_scene;
}

//------------------------------------------------------------------------------
Entity * Entity::getChildByName(const std::string & name) const
{
    for (auto it = m_children.begin(); it != m_children.end(); ++it)
    {
        Entity * e = *it;
        if (e->getName() == name)
            return e;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
Entity * Entity::getChildByType(const std::string & name) const
{
    for (auto it = m_children.begin(); it != m_children.end(); ++it)
    {
        Entity * e = *it;
        if (e->getObjectType().is(name))
            return e;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
Entity * Entity::addChild(Entity * child)
{
#ifdef SN_BUILD_DEBUG
    if (child->getParent() != this)
    {
        SN_WARNING("Entity::addChild: parent is not matching with " << toString());
    }
    if (containsChild(child))
    {
        SN_ERROR("Entity::addChild: already contained in entity " << toString());
        return nullptr;
    }
#endif
    m_children.push_back(child);
    return child;
}

//------------------------------------------------------------------------------
u32 Entity::indexOfChild(const Entity * child) const
{
    u32 i = 0;
    for (auto it = m_children.begin(); it != m_children.end(); ++it)
    {
        if ((*it) == child)
            break;
        ++i;
    }
    return false;
}

//------------------------------------------------------------------------------
Entity * Entity::createChild(const std::string & typeName)
{
    Entity * e = nullptr;

    if (typeName.empty())
    {
        Entity * e = new Entity();
    }
    else
    {
        Object * obj = instantiateDerivedObject(typeName, Entity::__sGetBaseClassName());
        if (obj)
            e = (Entity*)obj;
    }

    if (e)
    {
        e->setParent(this);
    }

    return e;
}

//------------------------------------------------------------------------------
Entity * Entity::requireChild(const std::string & typeName)
{
    Entity * e = getChildByType(typeName);
    if (e == nullptr)
        e = createChild(typeName);
    return e;
}

//------------------------------------------------------------------------------
u32 Entity::removeChild(const Entity * child)
{
    u32 i = indexOfChild(child);
    if (i != m_children.size())
    {
        m_children[i] = nullptr;
        m_children[i] = m_children.back();
        m_children.pop_back();
    }
    return i;
}

//------------------------------------------------------------------------------
void Entity::destroyChildren()
{
    auto children = m_children;
    for (auto it = children.begin(); it != children.end(); ++it)
    {
        delete *it;
    }
    m_children.clear();
}

//------------------------------------------------------------------------------
void Entity::destroy()
{
    onDestroy();
    setFlag(SN_EF_DESTROYED, true);
    release();
}

//------------------------------------------------------------------------------
void Entity::destroyLater()
{
    setFlag(SN_EF_DESTROYED, true);
}

//------------------------------------------------------------------------------
// Static
void Entity::serialize(JsonBox::Value & o, Entity & e)
{
    o[SN_JSON_TYPE_TAG] = Entity::__sGetClassName();
    e.serializeState(o);
    if (e.getChildCount() != 0)
    {
        JsonBox::Value & a = o[SN_JSON_ENTITY_CHILDREN_TAG];
        for (u32 i = 0; i < e.getChildCount(); ++i)
        {
            Entity & child = *(e.getChildByIndex(i));
            serialize(a[i], child);
        }
    }
}

//------------------------------------------------------------------------------
// Static
Entity * Entity::unserialize(JsonBox::Value & o, Entity & parent)
{
    std::string typeName = o[SN_JSON_TYPE_TAG].getString();
    ObjectType * ot = ObjectTypeDatabase::get().getType(typeName);
    if (ot)
    {
        Object * obj = instantiateDerivedObject(typeName, Entity::__sGetClassName());
        if (obj)
        {
            Entity * e = (Entity*)obj;
            e->setParent(&parent);
            e->unserializeState(o);
            if (o[SN_JSON_ENTITY_CHILDREN_TAG].isArray())
            {
                JsonBox::Value & a = o[SN_JSON_ENTITY_CHILDREN_TAG];
                u32 len = a.getArray().size();
                for (u32 i = 0; i < len; ++i)
                {
                    Entity::unserialize(a[i], *e);
                }
            }
            return e;
        }
        // Error message already handled by the instantiate helper
    }
    else
    {
        SN_ERROR("Unknown object object type from JSON (name=" << typeName << ")");
    }
    return nullptr;
}

//------------------------------------------------------------------------------
void Entity::serializeState(JsonBox::Value & o)
{
    o["name"] = m_name;
    o["enabled"] = isEnabledSelf();
    sn::serialize(o, m_tags);
}

//------------------------------------------------------------------------------
void Entity::unserializeState(JsonBox::Value & o)
{
    m_name = o["name"].getString();
    setEnabled(o["enabled"].getBoolean());
    sn::serialize(o, m_tags);

    Scene * scene = getScene();
    for (auto it = m_tags.begin(); it != m_tags.end(); ++it)
    {
        scene->registerTaggedEntity(this, *it);
    }
}

} // namespace sn
