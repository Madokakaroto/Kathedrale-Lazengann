#pragma once

namespace kath
{
    template <typename T>
    inline static void new_class(lua_State* L)
    {
        using type = meta_class_t<T>;
        static_assert(!std::is_same_v<type, dummy_meta_class>, "not supported class");

        // TODO ... higher-level table op
        stack_guard guard{ L };

        // TODO ... check metatable
        ::luaL_newmetatable(L, type::name());       // stack top is the metatable

        // __index
        stack_duplicate(L);                         // duplicate
        set_field(L, "__index");                    // metatable["__index"] = metatable

        // TODO ... __newindex

        // __gc
        if constexpr(is_reference_type_v<T>)
        {
            stack_push(L, [](lua_State* L) -> int
            {
                using ref_counter_type = decltype(std::declval<T>().ref_from_this());
                // TODO .. check ?
                auto ptr = detail::stack_get_emplaced_userdata<ref_counter_type>(L, 1);
                assert(ptr);
                ptr->~ref_counter_type();
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