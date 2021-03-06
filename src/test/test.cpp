﻿#include <kath.hpp>
#include <iostream>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "kath.test"
#include <boost/test/unit_test.hpp>

namespace
{
    lua_State* init_lua_low_level()
    {
        lua_State* L = ::luaL_newstate();
        assert(L);
        ::luaL_openlibs(L);
        return L;
    }

    bool do_script_low_level(lua_State* L, std::string script)
    {
        assert(L);
        auto error = ::luaL_loadstring(L, script.c_str()) || ::lua_pcall(L, 0, 0, 0);
        if (error)
        {
            std::cout << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
            return false;
        }

        return true;
    }
}

#define KATH_LUA_LOWLEVEL_BEGIN \
auto L = init_lua_low_level(); { kath::stack_guard guard{ L };
#define KATH_LUA_LOWLEVEL_END \
} BOOST_CHECK(::lua_gettop(L) == 0); ::lua_close(L);

#include "kath_mpl_test.hpp"
#include "kath_lowlevel_test.hpp"
#include "kath_userdata_test.hpp"