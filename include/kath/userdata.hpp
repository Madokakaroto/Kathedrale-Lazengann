#pragma once

namespace kath
{
    template <typename T>
    inline static void new_class(lua_State* L)
    {
		using type = reflexpr(T);

        // TODO ... higher-level table op
        stack_guard guard{ L };

        // TODO ... check metatable
        ::luaL_newmetatable(L, type::name());       // stack top is the metatable

        // __index
        stack_duplicate(L);                         // duplicate
        set_field(L, "__index");                    // metatable["__index"] = metatable

        // TODO ... __newindex

        // __gc
        if constexpr(is_userdata_reference_type_v<T>)
        {
            stack_push(L, [](lua_State* L) -> int
            {
                using shared_ptr_t = decltype(std::declval<T>().shared_from_this());
                auto ptr = stack_get<shared_ptr_t*>(L, 1);
                assert(ptr);
                ptr->~shared_ptr_t();
                return 0;
            });
            set_field(L, "__gc");
        }

        // TODO ... __call
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
}