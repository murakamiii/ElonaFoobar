#pragma once

#include <string>
#include <unordered_map>

#include "../filesystem.hpp"
#include "../semver.hpp"



namespace elona
{
namespace lua
{

struct ModList
{
public:
    using RequirementList =
        std::unordered_map<std::string, semver::VersionRequirement>;


    static ModList from_file(const fs::path& path);

    // for testing
    static ModList from_string(const std::string& str);
    static ModList from_stream(std::istream& in, const std::string& filepath);


    // Save the mod list to `path`.
    void save(const fs::path& path);



    RequirementList& mods() noexcept
    {
        return _mods;
    }


    const RequirementList& mods() const noexcept
    {
        return _mods;
    }



private:
    RequirementList _mods;


    ModList(const RequirementList& mods)
        : _mods(mods)
    {
    }
};



struct ModLock
{
    // TODO
};



struct ModIndex
{
public:
    using ModId = std::string;
    using Dependencies = std::unordered_map<ModId, semver::VersionRequirement>;


    struct IndexEntry
    {
        semver::Version version;
        Dependencies dependencies;
        Dependencies optional_dependencies;
    };


    using QueryResult =
        either::either<std::string, std::reference_wrapper<const IndexEntry>>;


    static ModIndex traverse(const fs::path& mod_root_dir);


    ModIndex(const std::unordered_map<ModId, std::vector<IndexEntry>>& mods)
        : _mods(mods)
    {
    }


    QueryResult query_latest(
        const ModId& id,
        const semver::VersionRequirement& requirement) const;


    const Dependencies& get_dependencies(
        const ModId& id,
        const semver::Version& version) const
    {
        return get_index_entry(id, version).dependencies;
    }


    const Dependencies& get_optional_dependencies(
        const ModId& id,
        const semver::Version& version) const
    {
        return get_index_entry(id, version).optional_dependencies;
    }


    // for debugging
    std::string to_string() const;



private:
    std::unordered_map<ModId, std::vector<IndexEntry>> _mods;



    const IndexEntry& get_index_entry(
        const ModId& id,
        const semver::Version& version) const;
};



struct ResolvedModList
{
public:
    using ModVersionMap = std::unordered_map<std::string, semver::Version>;
    using SortedModList = std::vector<std::string>;


    ResolvedModList(
        const ModVersionMap& mod_versions,
        const SortedModList& sorted_mods)
        : _mod_versions(mod_versions)
        , _sorted_mods(sorted_mods)
    {
    }



    const ModVersionMap& mod_versions() const noexcept
    {
        return _mod_versions;
    }



    const SortedModList& sorted_mods() const noexcept
    {
        return _sorted_mods;
    }



private:
    ModVersionMap _mod_versions;
    SortedModList _sorted_mods;
};



class ModVersionResolver
{
public:
    using ResolveResult = either::either<std::string, ResolvedModList>;

    ResolveResult
    resolve(const ModList& list, const ModLock& lock, const ModIndex& index);
};

} // namespace lua
} // namespace elona
