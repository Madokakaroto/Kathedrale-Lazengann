#pragma once

namespace kath
{
    enum class basic_type : int
    {
        nil = LUA_TNIL,
        boolean = LUA_TBOOLEAN,
        lightuserdata = LUA_TLIGHTUSERDATA,
        number = LUA_TNUMBER,
        string = LUA_TSTRING,
        table = LUA_TTABLE,
        function = LUA_TFUNCTION,
        userdata = LUA_TUSERDATA,
        thread = LUA_TTHREAD,
    };

    inline constexpr struct nil_t {} nil{};

    inline constexpr struct global_table_tag_t {}  global_table_tag{};
    inline constexpr struct normal_table_tag_t {} normal_table_tag{};

    inline static char const* basic_type_name(basic_type type) noexcept
    {
        switch(type)
        {
        case basic_type::boolean:
            return "b";
        case basic_type::number:
            return "n";
        case basic_type::string:
            return "s";
        default:
            return "X";
        }
    }
}