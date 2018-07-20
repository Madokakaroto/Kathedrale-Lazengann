#pragma once

// mpl extentions
namespace kath
{
    template <typename T, typename = void>
    struct has_forward_iterator : std::false_type {};
    template <typename T>
    struct has_forward_iterator<T, std::void_t<
        typename T::iterator,
    decltype(std::declval<typename T::iterator>()++)
    >> : std::true_type {};
    template <typename T>
    inline constexpr bool has_forward_iterator_v = has_forward_iterator<T>::value;

    template <typename T, typename = void>
    struct has_key_mapped_type : std::false_type {};
    template <typename T>
    struct has_key_mapped_type<T, std::void_t<
        typename T::key_type,
        typename T::mapped_type
        >> : std::true_type {};
    template <typename T>
    inline constexpr bool has_key_mapped_type_v = has_key_mapped_type<T>::value;

    // container
    template <typename T, typename = void>
    struct is_container : std::false_type {};
    template <typename T>
    struct is_container<T, std::void_t<
        decltype(std::declval<T>().size()),
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end())
        >> : meta_and<
            has_value_type<T>,
            has_forward_iterator<T>,
            negation<is_std_string<T>>,
            negation<is_string_view<T>>>{};

    // sequential container
    template <typename T>
    using is_sequential_container = meta_and<
        negation<has_key_mapped_type<T>>,
        is_container<T>
    >;
    template <typename T>
    inline constexpr bool is_sequential_container_v = is_sequential_container<T>::value;

    // associative container
    template <typename T>
    using is_associative_container = meta_and<
        has_key_mapped_type<T>, 
        is_container<T>
    >;
    template <typename T>
    inline constexpr bool is_associative_container_v = is_associative_container<T>::value;

    namespace detail
    {
        template <typename C, typename E>
        struct forward_type
        {
        private:
            using temp_type = std::remove_reference_t<C>;

            using temp_type1 = std::conditional_t<
                std::is_volatile_v<temp_type>,
                std::add_volatile_t<E>, E
            >;

            using temp_type2 = std::conditional_t<
                std::is_const_v<temp_type>,
                std::add_const_t<temp_type1>, temp_type1
            >;

        public:
            using type = std::conditional_t<
                std::is_lvalue_reference_v<C>,
                std::add_lvalue_reference_t<temp_type2>,
                std::conditional_t<
                    std::is_rvalue_reference_v<C>,
                    std::add_rvalue_reference_t<temp_type2>,
                    temp_type2
                >
            >;
        };

        template <typename T, typename E>
        using forward_type_t = typename forward_type<T, E>::type;
    }
}

namespace kath { namespace ext
{
    template <typename T>
    struct manipulate_type<T, std::enable_if_t<is_sequential_container_v<T>>>
    {
        static constexpr bool value = true;
        using type = T;
        using value_type = typename T::value_type;

        template <typename C>
        static auto stack_push(lua_State* L, C&& c)
            -> std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<C>>, type>>
        {
            using forward_value_type = detail::forward_type_t<C, value_type>;

            auto size = static_cast<int>(c.size());
            ::lua_createtable(L, size, 0);
            for(auto loop = 0; loop < size; ++loop)
            {
                set_table(L, loop, std::forward<forward_value_type>(c[loop]));
            }
        }

        static type stack_get(lua_State* L, int index = -1)
        {
            if(index < 0) index -= 1;

            type result{};
            kath::stack_push(L);
            while(::lua_next(L, index) != 0)
            {
                // TODO ... check index continuity
                //kath::stack_get<int>(L, -2);
             decltype(auto) value = kath::stack_get<value_type>(L);

                result.push_back(value);
                kath::stack_pop(L);
            }
            return result;
        }

        static type stack_check(lua_State* L, int arg = 1)
        {
            ::luaL_checktype(L, arg, LUA_TTABLE);
            if(arg < 0) arg -= 1;

            type result{};
            kath::stack_push(L);
            while(::lua_next(L, arg) != 0)
            {
                // TODO ... check index continuity
                //kath::stack_check<int>(L, -2);
                decltype(auto) value = kath::stack_check<value_type>(L, -1);

                result.push_back(value);
                kath::stack_pop(L);
            }
            return result;
        }
    };

    template <typename T>
    struct manipulate_type<T, std::enable_if_t<is_associative_container_v<T>>>
    {
        static constexpr bool value = true;
        using type = T;
        using value_type = typename T::value_type;
        using key_type = typename T::key_type;
        using mapped_type = typename T::mapped_type;

        template <typename C>
        static auto stack_push(lua_State* L, C&& c)
            -> std::enable_if_t<std::is_same_v<std::remove_cv_t<std::remove_reference_t<C>>, type>>
        {
            using forward_mapped_type = detail::forward_type_t<C, mapped_type>;

            ::lua_createtable(L, 0, static_cast<int>(c.size()));
            for(auto&& elem : c)
            {
                kath::set_table(L, elem.first, std::forward<forward_mapped_type>(elem.second));
            }
        }

        static type stack_get(lua_State* L, int index = -1)
        {
            if(index < 0) index -= 1;

            type result{};
            kath::stack_push(L);
            while(::lua_next(L, index) != 0)
            {
                decltype(auto) key = kath::stack_get<key_type>(L, -2);
                decltype(auto) value = kath::stack_get<mapped_type>(L, -1);

                result.emplace(key, value);
                kath::stack_pop(L);
            }
            return result;
        }

        static type stack_check(lua_State* L, int arg = 1)
        {
            ::luaL_checktype(L, arg, LUA_TTABLE);
            if(arg < 0) arg -= 1;

            type result{};
            kath::stack_push(L);
            while(::lua_next(L, arg) != 0)
            {
                decltype(auto) key = kath::stack_check<key_type>(L, -2);
                decltype(auto) value = kath::stack_check<mapped_type>(L, -1);

                result.emplace(key, value);
                kath::stack_pop(L);
            }
            return result;
        }
    };
} }