#pragma once

BOOST_AUTO_TEST_CASE(lowlevel_stack_basic)
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

BOOST_AUTO_TEST_CASE(lowlevel_stack_basic_numberic)
{
	using kath::stack_push;
	using kath::stack_check;

	KATH_LUA_LOWLEVEL_BEGIN;
	kath::stack_guard guard{ L };
	// push
	stack_push(L, std::numeric_limits<int8_t>::max());							  // 1
	stack_push(L, std::numeric_limits<int16_t>::max());							  // 2
	stack_push(L, std::numeric_limits<int32_t>::max());							  // 3
	stack_push(L, std::numeric_limits<int64_t>::max());							  // 4
	stack_push(L, std::numeric_limits<uint8_t>::max());							  // 5
	stack_push(L, std::numeric_limits<uint16_t>::max());						  // 6
	stack_push(L, std::numeric_limits<uint32_t>::max());						  // 7
	stack_push(L, std::numeric_limits<uint64_t>::max());						  // 8
	stack_push(L, std::numeric_limits<float>::max());							  // 9
	stack_push(L, std::numeric_limits<double>::max());							  // 10

	BOOST_CHECK_EQUAL(std::numeric_limits<int8_t>::max(),	stack_check<int8_t  >(L, 1));
	BOOST_CHECK_EQUAL(std::numeric_limits<int16_t>::max(),	stack_check<int16_t >(L, 2));
	BOOST_CHECK_EQUAL(std::numeric_limits<int32_t>::max(),	stack_check<int32_t >(L, 3));
	BOOST_CHECK_EQUAL(std::numeric_limits<int64_t>::max(),	stack_check<int64_t >(L, 4));
	BOOST_CHECK_EQUAL(std::numeric_limits<uint8_t>::max(),	stack_check<uint8_t >(L, 5));
	BOOST_CHECK_EQUAL(std::numeric_limits<uint16_t>::max(), stack_check<uint16_t>(L, 6));
	BOOST_CHECK_EQUAL(std::numeric_limits<uint32_t>::max(), stack_check<uint32_t>(L, 7));
	BOOST_CHECK_EQUAL(std::numeric_limits<uint64_t>::max(), stack_check<uint64_t>(L, 8));
	BOOST_CHECK_EQUAL(std::numeric_limits<float>::max(),	stack_check<float   >(L, 9));
	BOOST_CHECK_EQUAL(std::numeric_limits<double>::max(),	stack_check<double  >(L, 10));

	KATH_LUA_LOWLEVEL_END;
}

BOOST_AUTO_TEST_CASE(lowlevel_table_basic)
{
	using kath::basic_type;
	using kath::fetch_global;
	using kath::fetch_field;
	using kath::stack_check;

	KATH_LUA_LOWLEVEL_BEGIN;
	kath::stack_guard guard{ L };

	BOOST_CHECK(do_script_low_level(L, R"(
		table = { key = "value", key1 = 1 }	
	)"));
	BOOST_CHECK(::lua_gettop(L) == 0);

	std::string value = "value";
	BOOST_CHECK(basic_type::table == fetch_global(L, "table"));
	BOOST_CHECK(basic_type::string == fetch_field(L, "key"));
	BOOST_CHECK_EQUAL(value, stack_check<std::string>(L, -1));
	BOOST_CHECK(::lua_gettop(L) == 2);
	BOOST_CHECK(basic_type::number == fetch_field(L, "key1", -2));
	BOOST_CHECK_EQUAL(1, stack_check<int>(L, -1));

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