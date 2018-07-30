#pragma once

#include <boost/core/demangle.hpp>

namespace kath
{
    template <typename T>
    inline static auto neat_name() noexcept
    {
        return boost::core::demangle(typeid(T).name());
    }

    namespace detail
    {
        template <typename T, typename ... Args>
        inline static auto create_constructors() noexcept
        {
            return constructor_t<T>{};
        }

        // property
        template <typename Get, typename Set>
        struct property_helper
        {
            // Get -> T(C*)
            // Set -> void(C*, T)
            using get_traits_t = callable_traits<Get>;
            using set_traits_t = callable_traits<Set>;

            static void apply(lua_State* L, char const* name, Get get, Set set)
            {
                // TODO ... check signature of Get and Set

                // get field
                if (fetch_field_as_table(L, "__get"))
                {
                    set_table(L, name, std::forward<Get>(get));
                }

                // set field
                if (fetch_field_as_table(L, "__set"))
                {
                    set_table(L, name, std::forward<Set>(set));
                }

                stack_pop(L, 2);
            }
        };
    }

    template <typename T>
    class userdata_helper
    {
    public:
        userdata_helper(lua_State* L, char const* name = nullptr)
            : L(L)
            , native_name_(std::move(neat_name<T>()))
            , name_(name == nullptr ? native_name_ : name)
        {
            assert(this->L);
            init_userdata();
        }

        template <typename T, typename RawT = std::remove_reference_t<T>>
        userdata_helper& member(char const* name, T&& t)
        {
            if constexpr(std::is_member_function_pointer_v<RawT>)
            {
                set_table(L, name, std::move(bind(t)));
            }
            else if constexpr(std::is_member_object_pointer_v<RawT>)
            {
                this->property(name, 
                    std::move(bind_get(t)), 
                    std::move(bind_set(t)));
            }
            return *this;
        }
        
        template <typename Get, typename Set>
        userdata_helper& property(char const* name, Get&& g, Set&& s)
        {
            detail::property_helper<Get, Set>::apply(L, name,
                std::forward<Get>(g), std::forward<Set>(s));
            return *this;
        }

        // __call metamethod
        template <typename ... Args>
        userdata_helper& constructors()
        {
            static_assert(sizeof...(Args) >= 1);
            auto ctors = detail::create_constructors<T, Args...>();
            set_table(L, "__call", std::move(ctors));
            return *this;
        }

    private:
        void init_userdata()
        {
            // create metatable
            // TODO ... exception handle
            auto r = ::luaL_newmetatable(L, native_name_.c_str());

            // set name as global
            // TODO ... namespace 
            stack_duplicate(L);
            set_global(L, name_);

            // __index meta method
            set_table(L, "__index", &userdata_helper::metatable_index);
            // __newindex meta method
            set_table(L, "__newindex", &userdata_helper::metatable_newindex);
            // __gc meta method
            metatable_gc(L);
        }

        inline static int metatable_index(lua_State* L);
        inline static int metatable_newindex(lua_State* L);
        inline static void metatable_gc(lua_State* L);

    private:
        lua_State*          L;
        std::string         native_name_;
        std::string         name_;
    };

    template <typename T>
    inline static auto new_class(lua_State* L, char const* name = nullptr)
    {
        static_assert(negation_v<std::is_const<T>>);
        return userdata_helper<T>{ L, name };
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

    template <typename T>
    int userdata_helper<T>::metatable_index(lua_State* L)
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

    template <typename T>
    int userdata_helper<T>::metatable_newindex(lua_State* L)
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
    void userdata_helper<T>::metatable_gc(lua_State* L)
    {
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
