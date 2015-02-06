﻿/*
AssetDatabase.hpp
Copyright (C) 2014-2015 Marc GILLERON
This file is part of the SnowfeetEngine project.
*/

#ifndef __HEADER_SN_ASSETDATABASE__
#define __HEADER_SN_ASSETDATABASE__

#include <core/util/typecheck.hpp>
#include <core/asset/AssetType.hpp>
#include <map>


namespace sn
{

/// \brief Contains all resources currently loaded by the application.
class SN_API AssetDatabase
{
public:

    // The name of an asset is the name of its file without the extension.
    // In a module, two assets of the same type cannot have the same name.

    // Note: this class could indeed use templates for each asset type.
    // however, due to the dynamic modular nature of the engine, we can't use compile-time
    // type checking in every part.

    AssetDatabase(String root);
    ~AssetDatabase();

    IAssetType * getType(std::string type);

    bool loadAssetFromFile(String path, bool reload=false);
    //bool releaseAsset(IAsset * asset);

    // Gets an asset. If it returns null, the asset may not have been loaded or is in progress.
    Asset * getAsset(const std::string & callingModule, const std::string & type, const String & name);

    AssetMetadata * getAssetMetadata(Asset * asset);

    // Template version of getAsset, compiled in your native code.
    // It works only if you used the SN_ASSET macro of your asset class.
    template <class Asset_T>
    Asset_T * getAsset(const std::string & callingModule, const String & name)
    {
        // Note: use SN_ASSET in your asset class
        IAsset * a = getAsset(Asset_T::__sGetDatabaseTypeName(), callingModule, name);
        if (a)
            return checked_cast<Asset_T*>(a);
        else
            return nullptr;
    }

    template <class Asset_T>
    bool addType(IAssetType * type)
    {
        // Note: use SN_ASSET in your asset class
        return addLoader(Asset_T::sGetDatabaseTypeName(), type);
    }

private:

	bool addType(std::string name, IAssetType * type);

private:

    // Top directory where assets are located (usually the "projects" directory)
    String m_root;

    // [typeName] => AssetType
    std::map<std::string, IAssetType*> m_types;

    // [typeName][name][moduleName] => Asset
    std::map< std::string, std::map<String, std::map<std::string,Asset*> > > m_assets;

    // [Asset] => AssetMetadata
    std::map<Asset*, AssetMetadata> m_metadatas;

};

} // namespace sn

#endif // __HEADER_SN_ASSETDATABASE__


