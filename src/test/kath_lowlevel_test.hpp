#pragma once

BOOST_AUTO_TEST_CASE(lowlevel_stack_basic)
{
    KATH_LUA_LOWLEVEL_BEGIN;
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
    // push
    stack_push(L, std::numeric_limits<int8_t>::max());                            // 1
    stack_push(L, std::numeric_limits<int16_t>::max());                           // 2
    stack_push(L, std::numeric_limits<int32_t>::max());                           // 3
    stack_push(L, std::numeric_limits<int64_t>::max());                           // 4
    stack_push(L, std::numeric_limits<uint8_t>::max());                           // 5
    stack_push(L, std::numeric_limits<uint16_t>::max());                          // 6
    stack_push(L, std::numeric_limits<uint32_t>::max());                          // 7
    stack_push(L, std::numeric_limits<uint64_t>::max());                          // 8
    stack_push(L, std::numeric_limits<float>::max());                             // 9
    stack_push(L, std::numeric_limits<double>::max());                            // 10

    BOOST_CHECK_EQUAL(std::numeric_limits<int8_t>::max(),   stack_check<int8_t  >(L, 1));
    BOOST_CHECK_EQUAL(std::numeric_limits<int16_t>::max(),  stack_check<int16_t >(L, 2));
    BOOST_CHECK_EQUAL(std::numeric_limits<int32_t>::max(),  stack_check<int32_t >(L, 3));
    BOOST_CHECK_EQUAL(std::numeric_limits<int64_t>::max(),  stack_check<int64_t >(L, 4));
    BOOST_CHECK_EQUAL(std::numeric_limits<uint8_t>::max(),  stack_check<uint8_t >(L, 5));
    BOOST_CHECK_EQUAL(std::numeric_limits<uint16_t>::max(), stack_check<uint16_t>(L, 6));
    BOOST_CHECK_EQUAL(std::numeric_limits<uint32_t>::max(), stack_check<uint32_t>(L, 7));
    BOOST_CHECK_EQUAL(std::numeric_limits<uint64_t>::max(), stack_check<uint64_t>(L, 8));
    BOOST_CHECK_EQUAL(std::numeric_limits<float>::max(),    stack_check<float   >(L, 9));
    BOOST_CHECK_EQUAL(std::numeric_limits<double>::max(),   stack_check<double  >(L, 10));

    KATH_LUA_LOWLEVEL_END;
}

BOOST_AUTO_TEST_CASE(lowlevel_table_basic)
{
    using kath::basic_type;
    using kath::fetch_global;
    using kath::fetch_field;
    using kath::stack_check;

    KATH_LUA_LOWLEVEL_BEGIN;

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

BOOST_AUTO_TEST_CASE(lowlevel_functional)
{
    KATH_LUA_LOWLEVEL_BEGIN;

    auto functor = kath::bind([](int a, int b) { return a + b; });
    kath::set_global(L, "add", functor);
    BOOST_CHECK(::lua_gettop(L) == 0);
    BOOST_CHECK(do_script_low_level(L, R"(
        c = add(1, 2)
    )"));

    kath::fetch_global(L, "c");
    BOOST_CHECK(::lua_gettop(L) == 1);
    BOOST_CHECK_EQUAL(3, kath::stack_check<int>(L));

    KATH_LUA_LOWLEVEL_END;
}

BOOST_AUTO_TEST_CASE(lowlevel_functional_multi_return)
{
    KATH_LUA_LOWLEVEL_BEGIN;

    auto swap = [](int a, int b) { return std::make_tuple(b, a); };
    kath::set_global(L, "swap", swap);
    BOOST_CHECK(::lua_gettop(L) == 0);
    BOOST_CHECK(do_script_low_level(L, R"(
        a, b = swap(-1, 2008)
    )"));

    kath::fetch_global(L, "a");
    kath::fetch_global(L, "b");
    BOOST_CHECK(::lua_gettop(L) == 2);
    BOOST_CHECK_EQUAL(2008, kath::stack_check<int>(L, -2));
    BOOST_CHECK_EQUAL(-1, kath::stack_check<int>(L, -1));

    KATH_LUA_LOWLEVEL_END;
}

BOOST_AUTO_TEST_CASE(lowlevel_pcall)
{
    KATH_LUA_LOWLEVEL_BEGIN;

    BOOST_CHECK(do_script_low_level(L, R"(
        function add(a, b)
            return a + b
        end

        function swap(a, b)
            return b, a
        end
    )"));

    kath::fetch_global(L, "add");
    BOOST_CHECK(::lua_gettop(L) == 1);
    auto result = kath::pcall<int>(L, 1, 2);
    BOOST_CHECK_EQUAL(3, result);

    kath::fetch_global(L, "swap");
    BOOST_CHECK(::lua_gettop(L) == 2);
    auto [a, b] = kath::pcall<int, int>(L, -1, 2008);
    BOOST_CHECK_EQUAL(a, 2008);
    BOOST_CHECK_EQUAL(b, -1);

    KATH_LUA_LOWLEVEL_END;
}

BOOST_AUTO_TEST_CASE(lowlevel_std_container)
{
    using namespace std::string_view_literals;
    KATH_LUA_LOWLEVEL_BEGIN;

    std::vector<int> v = { 1, 2, 3, 4, 5 };
    kath::set_global(L, "int_array", v);
    BOOST_CHECK(::lua_gettop(L) == 0);
    BOOST_CHECK(do_script_low_level(L, R"(
        for k,v in pairs(int_array) do
            print(k, v)
        end
    )"));

    std::map<std::string_view, int> table = {
        { "key1"sv, 1 },
        { "key2"sv, 2 },
        { "keyn"sv, 1024 }
    };
    kath::set_global(L, "table", table);
    BOOST_CHECK(::lua_gettop(L) == 0);
    BOOST_CHECK(do_script_low_level(L, R"(
        for k,v in pairs(table) do
            print(k, v)
        end
    )"));

    KATH_LUA_LOWLEVEL_END;
}