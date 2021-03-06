/*
Variant.cpp
Copyright (C) 2015-2015 Marc GILLERON
This file is part of the SnowfeetEngine project.
*/

#include "Variant.h"

namespace sn
{

//-----------------------------------------------------------------------------
Variant::Variant(const Variant & other):
    m_type(SN_VT_NIL)
{
    *this = other;
}

//-----------------------------------------------------------------------------
Variant::~Variant()
{
    reset();
}

//-----------------------------------------------------------------------------
void Variant::reset()
{
    switch (m_type.id)
    {
    case SN_VT_STRING:      delete m_data.pString; break;
    case SN_VT_ARRAY:       delete m_data.pArray; break;
    case SN_VT_DICTIONARY:  delete m_data.pDictionary; break;
    default: break;
    }
    m_type = SN_VT_NIL;
    memset(&m_data, 0, sizeof(VariantData));
}

//-----------------------------------------------------------------------------
void Variant::reset(VariantType t)
{
    reset();
    m_type = t;
    switch (m_type.id)
    {
    case SN_VT_BOOL:
    case SN_VT_INT:
    case SN_VT_FLOAT:
        memset(&m_data, 0, sizeof(VariantData));
    case SN_VT_STRING:      m_data.pString = new String(); break;
    case SN_VT_ARRAY:       m_data.pArray = new Array(); break;
    case SN_VT_DICTIONARY:  m_data.pDictionary = new Dictionary(); break;
    default: break;
    }
}

//-----------------------------------------------------------------------------
bool Variant::getBool() const
{
    if (m_type.id == SN_VT_BOOL)
        return m_data.vBool;
    return false;
}

//-----------------------------------------------------------------------------
s32 Variant::getInt() const
{
    if (m_type.id == SN_VT_INT)
        return m_data.vInt;
    return 0;
}

//-----------------------------------------------------------------------------
f32 Variant::getFloat() const
{
    if (m_type.id == SN_VT_FLOAT)
        return m_data.vFloat;
    return 0;
}

//-----------------------------------------------------------------------------
const Variant::String & Variant::getString() const
{
    if (m_type.id == SN_VT_STRING)
        return *m_data.pString;
    else
    {
        static String s_defaultString;
        return s_defaultString;
    }
}

//-----------------------------------------------------------------------------
const Variant::Array & Variant::getArray() const
{
    if (m_type.id == SN_VT_ARRAY)
        return *m_data.pArray;
    else
    {
        static Array s_defaultArray;
        return s_defaultArray;
    }
}

//-----------------------------------------------------------------------------
const Variant::Dictionary & Variant::getDictionary() const
{
    if (m_type.id == SN_VT_DICTIONARY)
        return *m_data.pDictionary;
    else
    {
        static Dictionary s_defaultDictionary;
        return s_defaultDictionary;
    }
}

//-----------------------------------------------------------------------------
void Variant::setBool(bool b)
{
    if (m_type != SN_VT_BOOL)
    {
        reset();
        m_type = SN_VT_BOOL;
    }
    m_data.vBool = b;
}

//-----------------------------------------------------------------------------
void Variant::setInt(s32 n)
{
    if (m_type != SN_VT_INT)
    {
        reset();
        m_type = SN_VT_INT;
    }
    m_data.vInt = n;
}

//-----------------------------------------------------------------------------
void Variant::setFloat(f32 f)
{
    if (m_type != SN_VT_FLOAT)
    {
        reset();
        m_type = SN_VT_FLOAT;
    }
    m_data.vFloat = f;
}

//-----------------------------------------------------------------------------
void Variant::setString(const String & str)
{
    if (m_type != SN_VT_STRING)
    {
        reset();
        m_type = SN_VT_STRING;
        m_data.pString = new String(str);
    }
    else
    {
        *m_data.pString = str;
    }
}

//-----------------------------------------------------------------------------
void Variant::setArray()
{
    if (m_type != SN_VT_ARRAY)
    {
        reset();
        m_type = SN_VT_ARRAY;
        m_data.pArray = new Array();
    }
}

//-----------------------------------------------------------------------------
void Variant::setArray(const Array & va)
{
    if (m_type != SN_VT_ARRAY)
    {
        reset();
        m_type = SN_VT_ARRAY;
        m_data.pArray = new Array(va);
    }
    else
    {
        *m_data.pArray = va;
    }
}

//-----------------------------------------------------------------------------
void Variant::setDictionary()
{
    if (m_type != SN_VT_DICTIONARY)
    {
        reset();
        m_type = SN_VT_DICTIONARY;
        m_data.pDictionary = new Dictionary();
    }
}

//-----------------------------------------------------------------------------
void Variant::setDictionary(const Dictionary & vd)
{
    if (m_type != SN_VT_DICTIONARY)
    {
        reset();
        m_type = SN_VT_DICTIONARY;
        m_data.pDictionary = new Dictionary(vd);
    }
    else
    {
        *m_data.pDictionary = vd;
    }
}

//-----------------------------------------------------------------------------
void Variant::grab(Variant & other)
{
    reset();
    m_type = other.m_type;
    m_data = other.m_data;
    other.m_type = SN_VT_NIL;
    memset(&other.m_data, 0, sizeof(VariantData));
}

//-----------------------------------------------------------------------------
Variant & Variant::operator=(const Variant & other)
{
    reset();
    switch (other.m_type.id)
    {
    // Objects need proper handling
    case SN_VT_STRING:      setString(*other.m_data.pString); break;
    case SN_VT_ARRAY:       setArray(*other.m_data.pArray); break;
    case SN_VT_DICTIONARY:  setDictionary(*other.m_data.pDictionary); break;
    // Null and scalar types just need copy
    default:
        m_type = other.m_type;
        m_data = other.m_data;
        break;
    }
    return *this;
}

//-----------------------------------------------------------------------------
bool Variant::operator==(const Variant & other) const
{
    if (m_type.id != other.m_type.id)
        return false;
    if (m_type.id == SN_VT_NIL)
        return true;

    switch (m_type.id)
    {
    case SN_VT_BOOL:        return m_data.vBool == other.m_data.vBool; break;
    case SN_VT_INT:         return m_data.vInt == other.m_data.vInt; break;
    case SN_VT_FLOAT:       return m_data.vFloat == other.m_data.vFloat; break;
    case SN_VT_STRING:      return *m_data.pString == *other.m_data.pString; break;
    case SN_VT_ARRAY:       return *m_data.pArray == *other.m_data.pArray; break;
    case SN_VT_DICTIONARY:  return *m_data.pDictionary == *other.m_data.pDictionary; break;
    default: return false;
    }
}

//-----------------------------------------------------------------------------
Variant & Variant::operator[](size_t index)
{
    assertType(SN_VT_ARRAY);
    if (m_data.pArray->size() <= index)
        m_data.pArray->resize(index + 1);
    return (*m_data.pArray)[index];
}

//-----------------------------------------------------------------------------
const Variant & Variant::operator[](size_t index) const
{
    if (m_type.id == SN_VT_ARRAY)
        return (*m_data.pArray)[index];
    else
    {
        static Variant s_defaultVariant;
        return s_defaultVariant;
    }
}

//-----------------------------------------------------------------------------
Variant & Variant::operator[](const String & fieldName)
{
    assertType(SN_VT_DICTIONARY);
    return (*m_data.pDictionary)[fieldName];
}

//-----------------------------------------------------------------------------
const Variant & Variant::operator[](const String & fieldName) const
{
    if (m_type.id == SN_VT_DICTIONARY)
    {
        const Dictionary & dict = *m_data.pDictionary;
        auto it = dict.find(fieldName);
        if (it != dict.end())
            return it->second;
    }
    static Variant s_defaultVariant;
    return s_defaultVariant;
}

//-----------------------------------------------------------------------------
union MarshallFloat
{
    f32 f;
    u32 i;
};

size_t Variant::getHash() const
{
    switch (m_type.id)
    {
    case SN_VT_NIL:
        return 0;

    case SN_VT_BOOL:
        return m_data.vBool ? 1 : 0;

    case SN_VT_INT:
        return m_data.vInt;

    case SN_VT_FLOAT:
    {
        MarshallFloat mf;
        mf.f = m_data.vFloat;
        return mf.i;
    }

    // TODO Have hash functions for the following containers.
    // At the moment, addresses are used.

    case SN_VT_STRING:
        return std::hash<String>()(*m_data.pString);

    case SN_VT_ARRAY:
        return (size_t)m_data.pArray;
        //return std::hash<Array>()(*m_data.pArray);

    case SN_VT_DICTIONARY:
        return (size_t)m_data.pDictionary;
        //return std::hash<Dictionary>()(*m_data.pDictionary);

    default: return 0;
    }
}

//-----------------------------------------------------------------------------
std::string toString(VariantType vt)
{
    switch (vt.id)
    {
    case SN_VT_NIL:         return "nil"; break;
    case SN_VT_BOOL:        return "bool"; break;
    case SN_VT_INT:         return "int"; break;
    case SN_VT_FLOAT:       return "float"; break;
    case SN_VT_STRING:      return "string"; break;
    case SN_VT_ARRAY:       return "array"; break;
    case SN_VT_DICTIONARY:  return "dictionary"; break;
    default:                return "undefined"; break;
    }
}

//-----------------------------------------------------------------------------
std::string toString(const Variant & v)
{
    if (v.isNil())
        return "Variant(nil)";
    std::stringstream ss;
    ss << toString(v.getType()) << "(";
    switch (v.getType().id)
    {
    case SN_VT_BOOL:        ss << v.getBool(); break;
    case SN_VT_INT:         ss << v.getInt(); break;
    case SN_VT_FLOAT:       ss << v.getFloat(); break;
    case SN_VT_STRING:      ss << v.getString(); break;
    case SN_VT_ARRAY:       ss << v.getArray().size() << " elements"; break;
    case SN_VT_DICTIONARY:  ss << v.getDictionary().size() << " elements"; break;
    default: break;
    }
    ss << ")";
    return ss.str();
}

} // namespace sn

