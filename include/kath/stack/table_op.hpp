#pragma once

// global table
namespace kath
{
    template <typename Key>
    inline static auto fetch_global(lua_State* L, Key const& key)
        -> std::enable_if_t<meta_or_v<is_c_string<Key>, is_char_array<Key>>, basic_type>
    {
        return basic_type{ ::lua_getglobal(L, key) };
    }

    template <typename Key>
    inline static auto fetch_global(lua_State* L, Key const& key)
        -> std::enable_if_t<has_c_str_v<Key>, basic_type>
    {
        return basic_type{ ::lua_getglobal(L, key.c_str()) };
    }

    template <typename Key>
    inline static auto set_global(lua_State* L, Key const& key)
        -> std::enable_if_t<meta_or_v<is_c_string<Key>, is_char_array<Key>>>
    {
        ::lua_setglobal(L, key);
    }

    template <typename Key>
    inline static auto set_global(lua_State* L, Key const& key)
        -> std::enable_if_t<has_c_str_v<Key>>
    {
        ::lua_setglobal(L, key.c_str());
    }

    template <typename Key, typename Value>
    inline static void set_global(lua_State* L, Key const& key, Value&& value)
    {
        stack_push(L, std::forward<Value>(value));
        set_global(L, key);
    }
}

// local table fetch
namespace kath
{
    template <typename Key>
    inline static auto fetch_field(lua_State* L, Key const& key, int index = -1)
        -> std::enable_if_t<meta_or_v<is_c_string<Key>, is_char_array<Key>>, basic_type>
    {
        return basic_type{ ::lua_getfield(L, index, key) };
    }

    template <typename Key>
    inline static auto fetch_field(lua_State* L, Key const& key, int index = -1)
        -> std::enable_if_t<has_c_str_v<Key>, basic_type>
    {
        return basic_type{ ::lua_getfield(L, index, key.c_str()) };
    }

    template <typename Key>
    inline static auto fetch_field(lua_State* L, Key const& key, int index = -1)
        -> std::enable_if_t<meta_and_v<is_string_buffer<Key>, negation<has_c_str<Key>>>, basic_type>
    {
        stack_push(L, key);
        return basic_type{ ::lua_gettable(L, index - 1) };
    }

    template <typename Key>
    inline static auto fetch_field(lua_State* L, Key const& key, int index = -1)
        -> std::enable_if_t<meta_or_v<is_floating_point<Key>, is_bool<Key>>, basic_type>
    {
        stack_push(L, key);
        return basic_type{ ::lua_gettable(L, index - 1) };
    }

    template <typename Key>
    inline static auto fetch_field(lua_State* L, Key const& key, int index = -1)
        -> std::enable_if_t<is_integral_v<Key>, basic_type>
    {
        return basic_type{ ::lua_geti(L, index, static_cast<lua_Integer>(key)) };
    }
}

// local table set
namespace kath
{
    template <typename Key>
    inline static auto set_field(lua_State* L, Key const& key, int index = -2)
        -> std::enable_if_t<meta_or_v<is_c_string<Key>, is_char_array<Key>>>
    {
        ::lua_setfield(L, index, key);
    }

    template <typename Key>
    inline static auto set_field(lua_State* L, Key const& key, int index = -2)
        -> std::enable_if_t<has_c_str_v<Key>>
    {
        ::lua_setfield(L, index, key.c_str());
    }

    template <typename Key>
    inline static auto set_field(lua_State* L, Key const& key, int index = -2)
        -> std::enable_if_t<is_integral_v<Key>>
    {
        ::lua_seti(L, index, static_cast<lua_Integer>(key));
    }

    template <typename Key, typename Value>
    inline static auto set_table(lua_State* L, Key const& key, Value&& value, int index = -1)
        -> std::enable_if_t<meta_or_v<has_c_str<Key>, is_integral<Key>>>
    {
        stack_push(L, std::forward<Value>(value));
        set_field(L, key, index - 1);
    }

    template <typename Key, typename Value, typename KeyR = std::remove_reference_t<Key>>
    inline static auto set_table(lua_State* L, Key&& key, Value&& value, int index = -1)
        -> disable_if_t<meta_or_v<has_c_str<KeyR>, is_integral<KeyR>>>
    {
        stack_push(L, std::forward<Key>(key));
        stack_push(L, std::forward<Value>(value));
        ::lua_settable(L, index - 2);
    }
}

// stack-op implement implement
namespace kath
{
    template <bool Safe>
    char const* stack_type_name(lua_State* L, int index)
    {
        auto type = static_cast<basic_type>(::lua_type(L, index));
        if constexpr(Safe)
        {
            switch (type)
            {
            case basic_type::userdata:
            {
                stack_guard guard{ L };
                ::lua_getmetatable(L, index);
                fetch_field(L, "__name");
                return stack_check<char const*>(L, -1);
            }
            default:
                return basic_type_name(type);
            }
        }
        else
            return basic_type_name(type);
    }
}