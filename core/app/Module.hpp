﻿/*
Module.hpp
Copyright (C) 2014-2015 Marc GILLERON
This file is part of the SnowfeetEngine project.
*/

#ifndef __HEADER_SN_MODULE__
#define __HEADER_SN_MODULE__

#include <core/app/ModuleArgs.hpp>
#include <core/app/ModuleInfo.hpp>
#include <core/system/time/Time.hpp>
#include <core/system/lib/SharedLib.hpp>

#include <set>
#include <list>
#include <map>

namespace sn
{

namespace CallbackName
{
    extern const char * CREATE;
    extern const char * START;
    extern const char * EVENT;
    extern const char * UPDATE;
    extern const char * DESTROY;
}

class Application;
class ScriptManager;
class Scene;

class SN_API Module
{
public:

    Module(Application & app, const ModuleInfo & info);
    ~Module();

    bool loadNativeBindings(ScriptManager & scriptEngine);
    bool compileScripts();
    void createServices(Scene & scene);

    static void calculateDependencies(
        const String & pathToProjects,
        const String & modPath, 
        std::list<ModuleInfo> & dependencies,
        std::set<String> * openSet = nullptr
    );

    //bool hasUpdateFunction();

    //void callVoidCallback(std::string cbName);
    //void onUpdate(Time delta);

    inline const ModuleInfo & getInfo() const { return m_info; }

private:

    void getScriptFiles(std::vector<String> & out_filePaths, const std::set<String> & extensions);

    bool loadNativeBindingsImpl(ScriptManager & scriptEngine);

    void unloadNativeBindings();
    void unloadNativeBindingsImpl();

    //void referenceCallbacks();
    //void clearCallbacks();

    const ModuleInfo m_info;
    Application & r_app;
    std::vector<SharedLib*> m_sharedLibs;

};

} // namespace sn

#endif // __HEADER_SN_MODULE__

