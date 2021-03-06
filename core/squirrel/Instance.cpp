#include "Instance.h"
#include "../app/Application.h"
#include "../squirrel/bind_tools.h"

namespace squirrel
{

//------------------------------------------------------------------------------
bool Instance::createRef(HSQUIRRELVM vm, const std::string & fullClassName, HSQOBJECT * out_obj, bool callConstructor)
{
    // Parse the full name to extract namespaces
    std::vector<std::string> parts = sn::split(fullClassName, '.');

    sq_pushroottable(vm);

    // Push tables the class might be into
    for (size_t i = 0; i < parts.size() - 1; ++i)
    {
        // Push table name
        const std::string &tableName = parts[i];
        sq_pushstring(vm, _SC(tableName.c_str()), tableName.size());

        // Get table
        if (SQ_FAILED(sq_get(vm, -1)))
        {
            sq_pop(vm, 1); // Pop root table
            SN_ERROR("Squirrel class not found: '" << fullClassName << "'");
            return false;
        }
    }

    // Push class name
    const std::string & relativeClassName = parts.back();
    sq_pushstring(vm, _SC(relativeClassName.c_str()), relativeClassName.size());

    // Get the class
    if (SQ_FAILED(sq_get(vm, -2))) // -2 because it's like we do roottable."className"
    {
        sq_pop(vm, 1);
        SN_ERROR("Squirrel class not found: '" << fullClassName << "'");
        return false;
    }

    if (callConstructor)
    {
        sq_pushroottable(vm); // this
        // Create an instance of it (It's like calling a function)
        if (SQ_FAILED(sq_call(vm, 1, SQTrue, SQTrue)))
        {
            sq_pop(vm, 2);
            SN_ERROR("Squirrel class instantiation raised an error: " << getLastError(vm));
            return false;
        }
    }
    else
    {
        // Create the instance without invoking the constructor
        if (SQ_FAILED(sq_createinstance(vm, -1)))
        {
            sq_pop(vm, 2);
            SN_ERROR("Squirrel class instantiation raised an error (without constructor): " << getLastError(vm));
            return false;
        }
    }

    // Pick a ref so we can pop the stack
    sq_resetobject(out_obj);
    sq_getstackobj(vm, -1, out_obj);
    sq_addref(vm, out_obj);

    sq_pop(vm, 3); // instance, class and roottable

    return true;
}

//------------------------------------------------------------------------------
bool Instance::createNoRef(HSQUIRRELVM vm, const std::string & fullClassName, HSQOBJECT * out_obj, bool callConstructor)
{
    if (createRef(vm, fullClassName, out_obj, callConstructor))
    {
        sq_pushobject(vm, *out_obj); // Push the object again
        sq_release(vm, out_obj); // Release the temporary reference so the only one is now on the stack
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
bool Instance::create(HSQUIRRELVM vm, const std::string & fullClassName, bool callConstructor)
{
    // Destroy previous instance if any
    releaseObject();
    // Set VM
    m_vm = vm;
    // Create instance
    return Instance::createRef(vm, fullClassName, &m_object, callConstructor);
}

//------------------------------------------------------------------------------
void Instance::setObject(HSQUIRRELVM vm, HSQOBJECT obj)
{
    SN_ASSERT(sq_isinstance(obj), "Object is not a Squirrel class instance");

    if (!isNull())
    {
        releaseObject();
    }

    m_vm = vm;
    m_object = obj;
    sq_addref(vm, &m_object);
}

//------------------------------------------------------------------------------
bool Instance::hasMethod(const std::string & methodName)
{
    if (isNull())
        return false;

    auto vm = m_vm;

    sq_pushobject(vm, m_object);
    sq_pushstring(vm, methodName.c_str(), methodName.size());

    if (SQ_FAILED(sq_get(vm, -2)))
    {
        sq_pop(vm, -1);
        return false;
    }

    SQObjectType t = sq_gettype(vm, -1);
    sq_pop(vm, -1);
    return t == OT_CLOSURE || t == OT_NATIVECLOSURE;
}

//------------------------------------------------------------------------------
bool Instance::callMethod(const std::string & methodName)
{
    if (!getMemberOnStack(methodName))
        return false;

    auto vm = m_vm;

    // Push this
    sq_pushobject(vm, m_object);

    // Call the method
    if (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue)))
    {
        sq_pop(vm, 2);
        SN_ERROR("Squirrel error on method call");
        return false;
    }

    // Pop the function and the object
    sq_pop(vm, 2);

    return true;
}

//------------------------------------------------------------------------------
bool Instance::getMemberOnStack(const std::string & key)
{
    if (isNull())
        return false;

    auto vm = m_vm;

    sq_pushobject(vm, m_object);
    sq_pushstring(vm, key.c_str(), key.size());

    if (SQ_FAILED(sq_get(vm, -2)))
    {
        sq_pop(vm, 1);
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool Instance::setMember(const char * name, HSQOBJECT obj)
{
    SN_ASSERT(!isNull(), "Instance is null");

    sq_pushobject(m_vm, m_object);
    sq_pushstring(m_vm, name, -1);
    sq_pushobject(m_vm, obj);
    SQRESULT res = sq_set(m_vm, -3);
    sq_pop(m_vm, 1); // pop m_object

    return SQ_SUCCEEDED(res);
}

//------------------------------------------------------------------------------
bool Instance::setMemberNull(const char * name)
{
    SN_ASSERT(!isNull(), "Instance is null");

    sq_pushobject(m_vm, m_object);
    sq_pushstring(m_vm, name, -1);
    sq_pushnull(m_vm);
    SQRESULT res = sq_set(m_vm, -3);
    sq_pop(m_vm, 1); // pop m_object

    return SQ_SUCCEEDED(res);
}

} // namespace squirrel

