﻿/*
AssetDatabase.cpp
Copyright (C) 2014-2015 Marc GILLERON
This file is part of the SnowfeetEngine project.
*/

#include "AssetDatabase.h"
#include <core/system/filesystem.h>
#include <core/reflect/ObjectTypeDatabase.h>
#include <core/util/Log.h>
#include <core/util/stringutils.h>

#ifdef SN_BUILD_DEBUG
#include <core/system/Clock.h>
#endif

namespace sn
{

//------------------------------------------------------------------------------
AssetDatabase & AssetDatabase::get()
{
	static AssetDatabase m_database;
	return m_database;
}

//------------------------------------------------------------------------------
AssetDatabase::AssetDatabase()
{
    m_rootWatcher.setFilterDuplicateEvents(true);
}

//------------------------------------------------------------------------------
AssetDatabase::~AssetDatabase()
{
    releaseAssets();
    releaseLoaders();
}

//------------------------------------------------------------------------------
void AssetDatabase::setRoot(const String & root)
{
    SN_ASSERT(m_assets.empty(), "Cannot set root after assets have been loaded!");
    m_root = root;
    m_rootWatcher.setPath(sn::toString(m_root));
}

//------------------------------------------------------------------------------
void AssetDatabase::addLoadersFromModule(const std::string & moduleName)
{
    const ObjectTypeDatabase & otb = ObjectTypeDatabase::get();
    const ObjectTypeList & types = otb.getTypes();
    const ObjectType & loaderType = getObjectType<AssetLoader>();

    for (auto it = types.begin(); it != types.end(); ++it)
    {
        const ObjectType & ot = **it;
        if (ot.getModuleName() == moduleName && ot.derivesFrom(loaderType) && !ot.isAbstract())
        {
            AssetLoader * loader = checked_cast<AssetLoader*>(ot.instantiate());
            addLoader(loader);
        }
    }
}

//------------------------------------------------------------------------------
void AssetDatabase::addLoader(AssetLoader * loader)
{
    SN_ASSERT(loader != nullptr, "addLoader() received null pointer");
    const ObjectType & aot = loader->getAssetInstanceType();
    SN_ASSERT(aot.is(loader->getBaseAssetType(), true), "AssetLoader asset types are not compatible!");
    SN_ASSERT(!aot.isAbstract(), loader->getObjectType().getName() << " cannot instantiate abstract type " << aot.getName());

    const ObjectType & ot = loader->getObjectType();
    auto & loaders = m_loaders[ot.getModuleName()];

    const ObjectType & assetType = loader->getAssetInstanceType();
    SN_ASSERT(loaders.find(assetType.getName()) == loaders.end(), "Loader registered twice for the same base type!");
    loaders.insert(std::make_pair(assetType.getName(), loader));
}

//------------------------------------------------------------------------------
void AssetDatabase::releaseLoadersFromModule(const std::string & moduleName)
{
    auto modIt = m_loaders.find(moduleName);
    if (modIt != m_loaders.end())
    {
        auto & loaders = modIt->second;
        for (auto it = loaders.begin(); it != loaders.end(); ++it)
        {
            AssetLoader * loader = it->second;
            SN_ASSERT(loader->getObjectType().getModuleName() == moduleName, "Invalid state");
            delete loader;
        }
    }
}

//------------------------------------------------------------------------------
void AssetDatabase::releaseLoaders()
{
    for (auto modIt = m_loaders.begin(); modIt != m_loaders.end(); ++modIt)
    {
        auto & loaders = modIt->second;
        for (auto it = loaders.begin(); it != loaders.end(); ++it)
        {
            AssetLoader * loader = it->second;
            delete loader;
        }
    }
    m_loaders.clear();
}

//------------------------------------------------------------------------------
AssetLoaderList AssetDatabase::findLoaders(const AssetMetadata & meta) const
{
    AssetLoaderList chain;
    for (auto modIt = m_loaders.begin(); modIt != m_loaders.end(); ++modIt)
    {
        auto & loaders = modIt->second;
        for (auto it = loaders.begin(); it != loaders.end(); ++it)
        {
            AssetLoader & loader = *it->second;
            if (loader.canLoad(meta) && loader.isDirect(meta))
            {
                chain.push_back(&loader);
            }
        }
    }
    if (!chain.empty())
        orderAssetLoaders(chain);
    return chain;
}

//------------------------------------------------------------------------------
AssetLoader * AssetDatabase::findLoader(const ObjectType & assetType) const
{
    for (auto modIt = m_loaders.begin(); modIt != m_loaders.end(); ++modIt)
    {
        auto & loaders = modIt->second;
        auto it = loaders.find(assetType.getName());
        if (it != loaders.end())
        {
            return it->second;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
void AssetDatabase::orderAssetLoaders(AssetLoaderList & chain) const
{
    std::sort(chain.begin(), chain.end(), 
        [](const AssetLoader * lhs, const AssetLoader * rhs) {
            return lhs->getPriority(*rhs) == -1;
        }
    );
}

//------------------------------------------------------------------------------
void AssetDatabase::loadAssets(const ProjectInfo & projectInfo)
{
#ifdef SN_BUILD_DEBUG
    Clock clock;
#endif
    std::vector<FileNode> files;
    getFilesRecursively(FilePath::join(m_root, projectInfo.directory), files);

    u32 indexedCount = 0;
    u32 loadedCount = 0;

	std::vector<Asset*> projectAssets;
    
    // First, index all the files we can load
    for (auto it = files.begin(); it != files.end(); ++it)
    {
        // TODO Ignore special folders such as cpp/
        const FileNode & file = *it;
		std::vector<Asset*> assets = preloadAssetFile(file.path, projectInfo.name);
        for (u32 i = 0; i < assets.size(); ++i)
			projectAssets.push_back(assets[i]);
        indexedCount += assets.size();;
    }
#ifdef SN_BUILD_DEBUG
    SN_LOG("Indexed " << indexedCount << " assets from " << projectInfo.name << " in " << clock.restart().asSeconds() << " seconds");
#endif

    // Then, load them
    for (auto it = projectAssets.begin(); it != projectAssets.end(); ++it)
    {
        Asset * asset = *it;
        if (loadAsset(asset) == SN_ALS_LOADED)
            ++loadedCount;
    }
#ifdef SN_BUILD_DEBUG
    SN_LOG("Loaded " << loadedCount << " assets from " << projectInfo.name << " in " << clock.getElapsedTime().asSeconds() << " seconds");
#endif
}

//------------------------------------------------------------------------------
std::vector<Asset*> AssetDatabase::preloadAssetFile(const String & path, const std::string & projectName)
{
    // Check if not already indexed
    auto assetIt = m_fileCache.find(path);
    if (assetIt != m_fileCache.end())
    {
        // Already indexed, return the asset
        return assetIt->second;
    }

    // Assets that will be produced by the file (usually one)
    std::vector<Asset*> assets;

    // Retrieve metadata
    AssetMetadata metadata;
    metadata.path = path;
    metadata.project = projectName;
    metadata.loadFromFile(path); // Not fatal if the .meta file isn't found

    // Find loaders
    AssetLoaderList loaderChain = findLoaders(metadata);

    if (!loaderChain.empty())
    {
        // Found loaders, the asset is preloaded
        for (auto it = loaderChain.begin(); it != loaderChain.end(); ++it)
        {
            const AssetLoader & loader = **it;
            const ObjectType & ot = loader.getAssetInstanceType();
            Asset * a = checked_cast<Asset*>(ot.instantiate());
            SN_ASSERT(a != nullptr, "asset instance type is not derived from Asset");
            assets.push_back(a);
        }
    }

    if (!assets.empty())
    {
		metadata.name = getFileNameWithoutExtension(toString(path));

		auto assetsCopy = assets;
		for (u32 i = 0; i < assetsCopy.size(); ++i)
        {
            Asset * asset = assetsCopy[i];

            const AssetLoader * loader = loaderChain.empty() ? nullptr : loaderChain[i];

            // Store in asset mapping using the ObjectType specified by the loader
            const ObjectType & baseObjectType = loader ? loader->getBaseAssetType() : asset->getObjectType();

			// Check ID collision
			// TODO Assets should have a GUID
			Asset *& previousAsset = m_assets[metadata.project][baseObjectType.getName()][metadata.name];
			if (previousAsset)
			{
				SN_ERROR("Asset already registered under the same identifiers (Name collision between objects of the same type).");
				SN_MORE("module: " << metadata.project << ", type : " << baseObjectType.getName() << ", name : " << metadata.name);
				SN_MORE("[0]: " << sn::toString(previousAsset->getAssetMetadata().path));
				SN_MORE("[1]: " << sn::toString(metadata.path) << " (will be ignored)");
				assets.erase(assets.begin() + i);
				asset->release();
			}
			else
			{
				// Assign metadata
				asset->m_metadata = metadata;

				// Store in file mapping
				addToFileCache(*asset);

				// Store in ID map
				previousAsset = asset;
				//m_assets[metadata.module][baseObjectType.getName()][metadata.name] = &asset;
			}
        }

        SN_DLOG("Indexed asset " << toString(path) << " as " << assets.size() << " objects");
    }

    // If nullptr, the asset cannot be loaded
    return assets;
}

//------------------------------------------------------------------------------
AssetLoadStatus AssetDatabase::loadAsset(Asset * asset, const AssetMetadata * a_newMetadata)
{
    SN_ASSERT(asset != nullptr, "Cannot receive nullptr");

    if (a_newMetadata)
    {
        asset->m_metadata = *a_newMetadata;
    }

    const AssetMetadata & metadata = asset->getAssetMetadata();
    const std::string & typeName = asset->getObjectType().getName();

    // TODO Don't load if the file's timestamp has not changed?

    SN_DLOG("Loading asset " << toString(metadata.path) << "...");

    // Open file stream
    std::ifstream ifs(metadata.path, std::ios::binary | std::ios::in);
    if (!ifs.good())
    {
        SN_ERROR("Cannot open file " << toString(metadata.path) << " for asset of type " << typeName);
        //asset->release();
        return SN_ALS_OPEN_ERROR;
    }

    const AssetLoader * loader = findLoader(asset->getObjectType());

    if (loader)
    {
        loader->load(ifs, *asset);
        ifs.close();
        return SN_ALS_LOADED;
    }
    else
    {
        // Loading failure
        //asset->release();
        SN_ERROR("An error occurred when loading asset " << toString(metadata.path) << " of type " << typeName);
        return SN_ALS_READ_ERROR;
    }
}

//------------------------------------------------------------------------------
void AssetDatabase::loadIndexedAssetsByPath(const std::string & path)
{
    auto it = m_fileCache.find(toWideString(path));
    if (it != m_fileCache.end())
    {
        std::vector<Asset*> & assets = it->second;
        for (u32 i = 0; i < assets.size(); ++i)
        {
            loadAsset(assets[i]);
        }
    }
}

//------------------------------------------------------------------------------
void AssetDatabase::addToFileCache(Asset & asset)
{
    std::vector<Asset*> & assets = m_fileCache[asset.getAssetMetadata().path];
    assets.push_back(&asset);
}

//------------------------------------------------------------------------------
void AssetDatabase::removeFromFileCache(Asset & asset)
{
    auto it = m_fileCache.find(asset.getAssetMetadata().path);
    if (it != m_fileCache.end())
    {
        std::vector<Asset*> & assets = it->second;
        if (assets.size() == 1)
        {
            assets.clear();
        }
        else
        {
            for (u32 i = 0; i < assets.size(); ++it)
            {
                if (assets[i] == &asset)
                {
                    assets[i] = assets.back();
                    assets.pop_back();
                    break;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void AssetDatabase::releaseAssets()
{
    u32 leakCount = 0;
    for (auto it1 = m_assets.begin(); it1 != m_assets.end(); ++it1)
    {
        for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
        {
            for (auto it3 = it2->second.begin(); it3 != it2->second.end(); ++it3)
            {
                Asset * asset = it3->second;
                u32 refcount = asset->release();
                if (refcount > 0)
                {
#ifdef SN_BUILD_DEBUG
                    // Note: sometimes, false-positive leaks are detected, because sometimes,
                    // assets reference others through strong refs. It should be okay, but
                    // the warning message is not valid.
                    AssetMetadata meta = asset->getAssetMetadata();
                    SN_WWARNING(L"Asset " << meta.path << L" is still referenced " << refcount << L" times");
#endif
                    ++leakCount;
                }
            }
        }
    }
    if (leakCount > 0)
    {
        SN_WARNING(leakCount << " assets are still referenced after deletion from the database");
    }
    m_assets.clear();
    m_fileCache.clear();
}

//------------------------------------------------------------------------------
// Gets an asset. If it returns null, the asset may not have been loaded or is in progress.
Asset * AssetDatabase::getAsset(const std::string & moduleName, const ObjectType & type, const std::string & name)
{
    auto it1 = m_assets.find(moduleName);
    if (it1 != m_assets.end())
    {
        // Search the specified type
        auto it2 = it1->second.find(type.getName());

        // If not found, search for base types
        const ObjectType * searchType = &type;
        while (it2 == it1->second.end())
        {
            searchType = searchType->getBase();
            if (searchType)
            {
                it2 = it1->second.find(searchType->getName());
            }
            else
            {
                // No more base types and nothing found
                return nullptr;
                // TODO Stop at Asset?
            }
        }

        // Find the asset by name in the container
        auto it3 = it2->second.find(name);
        if (it3 != it2->second.end())
        {
            // Found
            return it3->second;
        }
    }

    // Not found
    return nullptr;
}

//------------------------------------------------------------------------------
Asset * AssetDatabase::getAsset(const AssetLocation & loc, const std::string & typeName)
{
    const ObjectType * ot = ObjectTypeDatabase::get().getType(typeName);
    if (ot)
        return getAsset(loc.project, *ot, loc.name);
    else
        return nullptr;
}

//------------------------------------------------------------------------------
Asset * getAssetBySerializedLocation(
    const std::string & type, 
    const std::string & locationString, 
    const std::string & contextProject, 
    bool raiseError)
{
    AssetLocation location(locationString);
    if (!location.isEmpty())
    {
        AssetDatabase & adb = AssetDatabase::get();

        if (!location.project.empty())
        {
            // The project is specified, no need for contextual lookup
            return adb.getAsset(location, type);
        }

        // Try to get the asset from the project the serialization is occurring into
        location.project = contextProject;
        Asset * asset = adb.getAsset(location, type);
        if (asset)
            return asset;
    }

    if (raiseError)
    {
        SN_ERROR("Asset not found: " << location.getFullName() << " (" << type << ")");
    }

    // Asset not found
    return nullptr;
}

//------------------------------------------------------------------------------
void AssetDatabase::setTrackFileChanges(bool enabled)
{
    //m_rootWatcher.setPath(sn::toString(m_root));
    if (enabled)
        m_rootWatcher.setRecursive(true);
    m_rootWatcher.setEnabled(enabled);
}

//------------------------------------------------------------------------------
bool AssetDatabase::isTrackingFileChanges() const
{
    return m_rootWatcher.isEnabled();
}

//------------------------------------------------------------------------------
void AssetDatabase::updateFileChanges()
{
    if (m_rootWatcher.isEnabled())
    {
        // TODO What if an asset saves itself upon reloading? Infinite loop?
        FileWatcher::Event event;
        while (m_rootWatcher.popEvent(event))
        {
            switch (event.type)
            {
            case FileWatcher::FILE_MODIFIED:
                // TODO win32: two events are received when the asset is saved (CTRL+S). Need to filter the second one!
                SN_DLOG("Received file change: " << event.path);
                loadIndexedAssetsByPath(FilePath::join(toString(m_root), event.path));
                break;

            default:
                break;
            }
        }
    }
}

//------------------------------------------------------------------------------
#ifdef SN_BUILD_DEBUG
void AssetDatabase::printAssetList() const
{
    std::stringstream ss;

    for (auto itMod = m_assets.begin(); itMod != m_assets.end(); ++itMod)
    {
        const auto & modAssets = itMod->second;
        ss << itMod->first << std::endl;

        for (auto itType = modAssets.begin(); itType != modAssets.end(); ++itType)
        {
            const auto & assets = itType->second;
            ss << "    " << itType->first << std::endl;

            for (auto it = assets.begin(); it != assets.end(); ++it)
            {
                ss << "        " << it->first << std::endl;
            }
        }
    }

    SN_DLOG("AssetDatabase: \n\n" << ss.str() << '\n');
}
#endif

} // namespace sn



