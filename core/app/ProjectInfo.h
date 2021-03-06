﻿/*
ModuleInfo.h
Copyright (C) 2014-2015 Marc GILLERON
This file is part of the SnowfeetEngine project.
*/

#ifndef __HEADER_PROJECT_INFO__
#define __HEADER_PROJECT_INFO__

#include <core/util/String.h>
#include <core/util/Variant.h>
#include <vector>

namespace sn
{

class ProjectInfo
{
public:
    /// \brief Default name of a project file
    static const char * FILE_NAME;

    bool loadFromFile(const String & pathToProjects, const String & pathToFile);

    struct Service
    {
        std::string type;
        Variant args;
    };

    std::string name;
    String directory;
    String filePath;

    std::string scriptNamespace;

    /// \brief Dependency projects required by this project
    std::vector<String> dependencies;

    /// \brief Native libraries required by the project
    std::vector<std::string> modules;

    /// \brief Name of the startup scene asset.
    std::string startupScene;

    /// \brief Global entities to create in the main scene when the module is loaded
    std::string sceneManagers;

    /// \brief Serialized form of the update manager
    Variant updateLayers;

private:
    bool setDataFromVariant(const Variant & v);

};

} // namespace sn


#endif // __HEADER_MODULE_INFO__





