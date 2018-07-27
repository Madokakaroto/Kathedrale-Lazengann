#pragma once

namespace kath
{
    struct userdata_helper
    {
    public:
        template <typename T>
        static void init_meta_method(lua_State * L);
    private:
        inline static int __index(lua_State* L);
        inline static int __newindex(lua_State* L);
    };

    template <typename T>
    inline static void new_class(lua_State* L)
    {
        static_assert(negation_v<std::is_const<T>>);

        using type = reflexpr(T);

        stack_guard guard{ L };
        if (::luaL_newmetatable(L, type::name() == 0))       // stack top is the metatable
            return;

        userdata_helper::init_meta_method<T>();
    }

    template <typename Base, typename Derived>
    inline static auto inherit_from(lua_State* L) -> std::enable_if_t<std::is_base_of_v<Base, Derived>>
    {
        stack_guard guard{ L };

        luaL_getmetatable(L, get_class_name<Derived>());
        ::lua_createtable(L, 0, 2);
        luaL_getmetatable(L, get_class_name<Base>());
        set_field(L, "__index");
        ::lua_setmetatable(L, -2);
    }

    int userdata_helper::__index(lua_State* L)
    {
        constexpr int obj_idx = 1;
        constexpr int key_idx = 2;

        // push metatable to stack
        lua_getmetatable(L, obj_idx);

        // get metatable.key
        stack_duplicate(L, key_idx);
        lua_gettable(L, -2);
        if (lua_type(L, -1) != LUA_TNIL)
            return 1;

        // get by calling metatable.__set.key(obj)
        fetch_field(L, "__get");
        if (lua_type(L, -1) != LUA_TFUNCTION)
        {
            stack_push(L);
            return 1;
        }
        stack_duplicate(L, obj_idx);
        return lua_pcall(L, 1, 1, 0);
    }

    int userdata_helper::__newindex(lua_State* L)
    {
        constexpr int obj_idx = 1;
        constexpr int key_idx = 2;
        constexpr int val_idx = 3;

        // push metatable to stack
        lua_getmetatable(L, obj_idx);

        // perform metatable.__set.key(obj, value)
        fetch_field(L, "__set");
        if (lua_type(L, -1) != LUA_TTABLE)
        {
            return luaL_error(L, "This type of userdata is not capable of properties SET OP.");
        }

        stack_duplicate(L, key_idx);
        lua_gettable(L, -2);
        if (lua_type(L, -1) != LUA_TFUNCTION)
        {
            char const* key = stack_check<char const*>(L, key_idx);
            return luaL_error(L, "This type of userdata dosen`t have the %s field.", key);
        }

        stack_duplicate(L, obj_idx);
        stack_duplicate(L, val_idx);
        return lua_pcall(L, 2, 0, 0);
    }

    template <typename T>
    void userdata_helper::init_meta_method(lua_State* L)
    {
        // __index
        set_table(L, "__index", userdata_helper::__index);

        // __newindex
        set_table(L, "__newindex", userdata_helper::__newindex);

        // __gc
        if constexpr(is_userdata_reference_type_v<T>)
        {
            set_table(L, "__gc", [](lua_State* L) -> int
            {
                using shared_ptr_t = decltype(std::declval<T>().shared_from_this());
                auto ptr = stack_get<shared_ptr_t*>(L, 1);
                assert(ptr);
                ptr->~shared_ptr_t();
                return 0;
            });
        }
        else if constexpr(std::conjunction_v<is_userdata_value_type<T>, 
            negation<std::is_trivially_default_constructible<T>>>)
        {
            set_table(L, "__gc", [](lua_State* L) -> int {
                auto ptr = stack_get<T*>(L, 1);
                ptr->~T();
                assert(ptr);
                return 0;
            });
        }
    }
}




