﻿/*
ObjectType.h
Copyright (C) 2014-2015 Marc GILLERON
This file is part of the SnowfeetEngine project.
*/

#include "ObjectType.h"
#include "ObjectTypeDatabase.h"

#include <sstream>

namespace sn
{

    namespace
    {
        std::string getClassNameWithoutNamespace(const std::string & str)
        {
            size_t i = str.find_last_of(':');
            if (i == std::string::npos)
                return str;
            else
                return str.substr(i+1);
        }
    }

//------------------------------------------------------------------------------
ObjectType::ObjectType():
    m_isAbstract(false),
    m_ID(0), // null ID, until the type gets registered
    r_base(nullptr)
{
}

//------------------------------------------------------------------------------
bool ObjectType::is(const std::string & typeName, bool includeInheritance) const
{
    const ObjectType * t = ObjectTypeDatabase::get().getType(typeName);
    if (t)
        return is(*t, includeInheritance);
    else
        return false;
}

//------------------------------------------------------------------------------
bool ObjectType::is(const ObjectType & other, bool includeInheritance) const
{
    // If the object has a Null type
    if(m_ID == 0)
    {
        // Always return false
        return false;
    }

    if(m_ID == other.m_ID)
    {
        // Types exactly match
        return true;
    }
    else if(includeInheritance)
    {
        // Search inheritance tree
		return derivesFrom(other);
    }

	return false;
}

//------------------------------------------------------------------------------
bool ObjectType::derivesFrom(const ObjectType & other) const
{
    ObjectTypeDatabase & odb = ObjectTypeDatabase::get();
    const ObjectType * baseType = this;

    while(baseType->r_base != nullptr)
    {
        baseType = baseType->r_base;
        if(baseType == nullptr)
            return false;
        if(baseType->m_ID == other.m_ID)
            return true;
    }

	return false;
}

//------------------------------------------------------------------------------
Object * ObjectType::instantiate() const
{
    Object * obj = m_factory();
#ifdef SN_BUILD_DEBUG
    if (obj == nullptr)
    {
        SN_ERROR("ObjectType::instantiate: couldn't create instance of " << toString());
    }
#endif
    return obj;
}

//------------------------------------------------------------------------------
std::string ObjectType::toString() const
{
    std::stringstream ss;
    ss << "{[" << m_ID << "]" << m_name;
    if (r_base)
        ss << " : " << r_base->getName();
    ss << "}";
    return ss.str();
}

//------------------------------------------------------------------------------
void ObjectType::getChildrenTypes(std::vector<const ObjectType*> & out_children) const
{
	const ObjectTypeDatabase & odb = ObjectTypeDatabase::get();
	auto allTypes = odb.getTypes();
	for (auto it = allTypes.begin(); it != allTypes.end(); ++it)
	{
		const ObjectType & ot = **it;
		if (ot.derivesFrom(*this))
			out_children.push_back(this);
	}
}

//------------------------------------------------------------------------------
const ObjectType * ObjectType::getBase() const
{
    return r_base;
}

//------------------------------------------------------------------------------
void ObjectType::setName(const char * fullName)
{
    SN_ASSERT(m_name.empty(), "Cannot set type name twice!");
    m_name = fullName;
    if (m_scriptName.empty())
        m_scriptName = getClassNameWithoutNamespace(fullName);
}

//------------------------------------------------------------------------------
void ObjectType::setScriptName(const std::string & fullClassName)
{
    m_scriptName = fullClassName;
}

//------------------------------------------------------------------------------
bool operator==(const ObjectType & a, const ObjectType & b)
{
	// TODO Compare modules too?
    return a.getName() == b.getName();
}

//------------------------------------------------------------------------------
bool operator!=(const ObjectType & a, const ObjectType & b)
{
    return a.getName() != b.getName();
}


} // namespace sn

