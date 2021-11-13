#include "../elona/filesystem.hpp"
#include "../elona/lua_env/api_manager.hpp"
#include "../elona/lua_env/data_manager.hpp"
#include "../elona/lua_env/export_manager.hpp"
#include "../elona/lua_env/lua_env.hpp"
#include "../elona/lua_env/mod_manager.hpp"
#include "../elona/testing.hpp"
#include "../elona/variables.hpp"
#include "../thirdparty/catch2/catch.hpp"
#include "tests.hpp"



TEST_CASE("test reading invalid HCL file", "[Lua: Data]")
{
    const auto base_path = testing::get_test_data_path() / "registry";

    elona::lua::LuaEnv lua;
    lua.load_mods();
    lua.get_mod_manager().load_testing_mod_from_file(base_path / "invalid");

    REQUIRE_THROWS(lua.get_data_manager().init_from_mods());
}



TEST_CASE("test declaring and loading datatype", "[Lua: Data]")
{
    const auto base_path = testing::get_test_data_path() / "registry";

    elona::lua::LuaEnv lua;
    lua.load_mods();
    lua.get_mod_manager().load_testing_mod_from_file(base_path / "putit");

    REQUIRE_NOTHROW(lua.get_data_manager().init_from_mods());

    auto& table = lua.get_data_manager().get();

    auto normal = table.raw("putit.putit", "putit.normal");
    REQUIRE_SOME(normal);
    REQUIRE((*normal)["display_name"].get<std::string>() == "putit");
    REQUIRE((*normal)["integer_id"].get<int>() == 3);

    auto red = table.raw("putit.putit", "putit.red");
    REQUIRE_SOME(red);
    REQUIRE((*red)["display_name"].get<std::string>() == "red putit");
    REQUIRE((*red)["integer_id"].get<int>() == 4);
}



TEST_CASE("test loading datatype originating from other mod", "[Lua: Data]")
{
    const auto base_path = testing::get_test_data_path() / "registry";

    elona::lua::LuaEnv lua;
    lua.load_mods();
    lua.get_mod_manager().load_testing_mod_from_file(base_path / "putit");
    lua.get_mod_manager().load_testing_mod_from_file(base_path / "putit_b");

    REQUIRE_NOTHROW(lua.get_data_manager().init_from_mods());

    auto& table = lua.get_data_manager().get();

    auto green = table.raw("putit.putit", "putit_b.green");
    REQUIRE_SOME(green);
    REQUIRE((*green)["display_name"].get<std::string>() == "green putit");
    REQUIRE((*green)["integer_id"].get<int>() == 5);
}



TEST_CASE(
    "test verification that Exports table only have string keys",
    "[Lua: Data]")
{
    elona::lua::LuaEnv lua;
    lua.load_mods();
    REQUIRE_NOTHROW(lua.get_mod_manager().load_testing_mod_from_file(
        testing::get_test_data_path() / "mods" / "test_export_keys"));
    REQUIRE_THROWS(lua.get_api_manager().init_from_mods());
}



TEST_CASE("test order of script execution", "[Lua: Data]")
{
    const auto base_path = testing::get_test_data_path() / "registry";

    elona::lua::LuaEnv lua;
    lua.load_mods();
    lua.get_mod_manager().load_testing_mod_from_file(base_path / "load_order");

    REQUIRE_NOTHROW(lua.get_data_manager().init_from_mods());

    auto& table = lua.get_data_manager().get();

    auto spell = table.raw("core.ability", "load_order.expecto_patronum");
    REQUIRE_SOME(spell);
    REQUIRE((*spell)["related_basic_attribute"].get<int>() == 17);
    REQUIRE((*spell)["cost"].get<int>() == 100);
}
