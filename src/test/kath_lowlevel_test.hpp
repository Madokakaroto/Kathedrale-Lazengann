#pragma once

BOOST_AUTO_TEST_CASE(stack_basic)
{
	KATH_LUA_LOWLEVEL_BEGIN;
	kath::stack_guard guard{ L };
	std::string hello_world = "hello world";
	kath::stack_push(L, hello_world);
	kath::stack_push(L, 0);
	kath::stack_push(L, 1.0);
	kath::stack_push(L, false);
	BOOST_CHECK_EQUAL(hello_world, luaL_checkstring(L, 1));
	BOOST_CHECK_EQUAL(0, luaL_checkinteger(L, 2));
	BOOST_CHECK_EQUAL(1.0, luaL_checknumber(L, 3));
	BOOST_CHECK(lua_type(L, 4) == LUA_TBOOLEAN);
	BOOST_CHECK_EQUAL(0, ::lua_toboolean(L, 4));
	KATH_LUA_LOWLEVEL_END;
}

BOOST_AUTO_TEST_CASE(functional)
{
	KATH_LUA_LOWLEVEL_BEGIN;
	kath::stack_push(L, [](int a, int b) { return a + b; });
	kath::stack_push(L, [](lua_State* L) { return 0; });
	kath::stack_push(L, [a = 1](lua_State* L){ return a; });

	auto overload = kath::overload([](std::string const& a, int b) { return a.size() + b; }, [](int a) { return a; });
	kath::stack_push(L, std::move(overload));

	KATH_LUA_LOWLEVEL_END
}