#pragma once

// basic mpl
namespace kath
{
    // negative
    template <typename T>
    struct negation : std::bool_constant<!T::value> {};
    template <typename T>
    inline constexpr bool negation_v = negation<T>::value;

    // logical conjunction
    template <typename ... MetaFunc>
    using meta_and = std::conjunction<MetaFunc...>;
    template <typename ... MetaFunc>
    inline constexpr bool meta_and_v = meta_and<MetaFunc...>::value;

    // logical disjunction
    template <typename ... MetaFunc>
    using meta_or = std::disjunction<MetaFunc...>;
    template <typename ... MetaFunc>
    inline constexpr bool meta_or_v = meta_or<MetaFunc...>::value;

    // check type T is an instance of template Tmpl
    template <typename T, template <typename ...> class Tmpl>
    struct is_instance_of_impl : std::false_type {};
    template <template <typename ...> class Tmpl, typename ... Args>
    struct is_instance_of_impl<Tmpl<Args...>, Tmpl> : std::true_type {};
    template <typename T, template <typename ...> class Tmpl>
    using is_instance_of = is_instance_of_impl<std::remove_cv_t<T>, Tmpl>;
    template <typename T, template <typename ...> class Tmpl>
    inline constexpr bool is_instance_of_v = is_instance_of<T, Tmpl>::value;

    // equal
    template <typename LHS, typename RHS>
    struct meta_equal : std::bool_constant<LHS::value == RHS::value> {};

    template <typename LHS, typename RHS>
    inline constexpr bool meta_equal_v = meta_equal<LHS, RHS>::value;

    // comparable
    template <typename T, typename = void>
    struct is_comparable : std::false_type {};
    template <typename T>
    struct is_comparable<T, std::void_t<
        decltype(std::declval<T>() == std::declval<T>())
    >> : std::true_type {};
    template <typename T>
    inline constexpr bool is_comparable_v = is_comparable<T>::value;
}

// SFINAE
namespace kath
{
    template <bool Test, typename T = void>
    using disable_if = std::enable_if<negation_v<std::bool_constant<Test>>, T>;

    template <bool Test, typename T = void>
    using disable_if_t = typename disable_if<Test, T>::type;
}

// swallo
namespace kath
{
    struct swallow_t
    {
        template <typename ... T>
        constexpr swallow_t(T&& ... t) noexcept {}
    };
}

#define KATH_TMP_TRAITS_TYPE(TYPE)                          \
template <typename T, typename = void>                      \
struct BOOST_PP_CAT(traits_type_, TYPE) {                   \
    using type = void;                                      \
};                                                          \
template <typename T>                                       \
struct BOOST_PP_CAT(traits_type_, TYPE)<T,                  \
    std::void_t<typename T::TYPE>> {                        \
    using type = typename T::TYPE;                          \
};                                                          \
template <typename T>                                       \
using BOOST_PP_CAT(TYPE, _t) = typename T::value_type;      \
template <typename T>                                       \
using BOOST_PP_CAT(safe_, BOOST_PP_CAT(TYPE, _t)) =         \
    typename BOOST_PP_CAT(traits_type_, TYPE)<T>::type;     \
template <typename T, typename = void>                      \
struct BOOST_PP_CAT(has_, TYPE) : std::false_type{};        \
template <typename T>                                       \
struct BOOST_PP_CAT(has_, TYPE)<T,                          \
    std::void_t<typename T::TYPE>> : std::true_type{};

// type meta-function
namespace kath
{
    // remove reference then remove const and volitale
    template <typename T>
    using remove_rcv = std::remove_cv<std::remove_reference_t<T>>;

    template <typename T>
    using remove_rcv_t = typename remove_rcv<T>::type;

    KATH_TMP_TRAITS_TYPE(value_type)
    KATH_TMP_TRAITS_TYPE(element_type)
}

// basic type traits
namespace kath
{
    ///////
    template <typename T>
    using is_bool = std::is_same<std::add_const_t<T>, bool const>;
    template <typename T>
    inline constexpr bool is_bool_v = is_bool<T>::value;

    // is integral except bool
    template <typename T>
    using is_integral = meta_and<std::is_integral<T>, negation<is_bool<T>>>;
    template <typename T>
    inline constexpr bool is_integral_v = is_integral<T>::value;

    ///////
    using std::is_floating_point;
    using std::is_floating_point_v;

    ///////
    template <typename T, typename P>
    struct is_pointer_of : std::false_type {};
    template <typename T>
    struct is_pointer_of<T*, T> : std::true_type {};
    template <typename T, typename P>
    inline constexpr bool is_pointer_of_v = is_pointer_of<T, P>::value;

    // c-style string is the zero-terminal string
    template <typename T>
    using is_c_string = meta_or<is_pointer_of<T, char>, is_pointer_of<T, char const>>;
    template <typename T>
    inline constexpr bool is_c_string_v = is_c_string<T>::value;

    // char[M] or char const[N]
    template <typename T>
    struct is_char_array_impl
    {
    private:
        using array_type = std::remove_reference_t<T>;
        using element_type = std::remove_all_extents_t<array_type>;
    public:
        using type = meta_and<
            std::is_array<array_type>,
            meta_equal<std::rank<array_type>, std::integral_constant<size_t, 1>>,
            std::is_same<std::add_const_t<element_type>, char const>
        >;
    };
    template <typename T>
    using is_char_array = typename is_char_array_impl<T>::type;
    template <typename T>
    inline constexpr bool is_char_array_v = is_char_array<T>::value;

    namespace detail
    {
        template <typename T>
        using is_instance_of_std_string = is_instance_of<T, std::basic_string>;
        template <typename T>
        using is_instance_of_std_string_view = is_instance_of<T, std::basic_string_view>;
    }

    // std::string with char as element_type
    template <typename T>
    using is_std_string = meta_and<
        detail::is_instance_of_std_string<T>, 
        std::is_same<safe_value_type_t<T>, char> 
    >;
    template <typename T>
    inline constexpr bool is_std_string_v = is_std_string<T>::value;

    // std::string_view with char as element_type
    template <typename T>
    using is_string_view = meta_and<
        detail::is_instance_of_std_string_view<T>,
        std::is_same<safe_value_type_t<T>, char>
    >;
    template <typename T>
    inline constexpr bool is_string_view_v = is_string_view<T>::value;

    // callable
    template <typename T, typename = void>
    struct is_functor : std::false_type {};
    template <typename T>
    struct is_functor<T, std::void_t<decltype(&T::operator())>> : std::true_type {};
    template <typename T>
    inline constexpr bool is_functor_v = is_functor<T>::value;

    template <typename T>
    using is_callable = meta_or<std::is_function<T>, is_functor<T>>;
    template <typename T>
    inline constexpr bool is_callable_v = is_callable<T>::value;

    // is smart pointer
    template <typename T, typename = void>
    struct is_smart_pointer : std::false_type {};
    template <typename T>
    struct is_smart_pointer<T, std::void_t<
        typename T::element_type,
        decltype(&T::operator->),
        decltype(&T::operator*),
        decltype(std::declval<T>().reset()),
        decltype(std::declval<T>().get())
    >> : std::true_type {};
    template <typename T>
    static constexpr bool is_smart_pointer_v = is_smart_pointer<T>::value;
}

// kaht type traits
namespace kath
{
    namespace ext
    {
        // for extention
        // and manipulate type can only have value semantic
        template <typename T, typename>
        struct manipulate_type
        {
            static constexpr bool value = false;
        };
    }

    /* Manipulated type will be mapped into lua table */
    template <typename T>
    struct is_manipulated_type : std::bool_constant<ext::manipulate_type<T>::value> {};
    template <typename T>
    inline constexpr bool is_manipulated_type_v = is_manipulated_type<T>::value;

    // is string buffer
    template <typename T, typename = void>
    struct is_string_buffer : std::false_type {};
    template <typename T>
    struct is_string_buffer<T, std::void_t<
        decltype(std::declval<T>().data()),
        decltype(std::declval<T>().size()),
        decltype(T{ std::declval<char const*>(), std::declval<size_t>() })
        >> : negation<meta_or<is_manipulated_type<T>, std::is_reference<T>>> {};
    template <typename T>
    inline constexpr bool is_string_buffer_v = is_string_buffer<T>::value;

    // is string 
    template <typename T>
    using is_string = meta_or<
        is_c_string<T>,
        is_char_array<T>,
        is_string_buffer<T>
    >;
    template <typename T>
    inline constexpr bool is_string_v = is_string<T>::value;

    // has c_str member function
    template <typename T, typename = void>
    struct has_c_str : std::false_type {};
    template <typename T>
    struct has_c_str<T, std::void_t<
        decltype(&T::c_str)
    >> : std::true_type {};
    template <typename T>
    inline constexpr bool has_c_str_v = has_c_str<T>::value;

    // is lua_cfunction
    template <typename F>
    using is_lua_cfunction = meta_and<
        negation<std::is_reference<F>>,
        std::is_convertible<F, ::lua_CFunction>
    >;
    template <typename F>
    inline constexpr bool is_lua_cfunction_v = is_lua_cfunction<F>::value;

    // is lua_cfunctor
    template <typename F, typename = void>
    struct is_lua_cfunctor : std::false_type {};
    template <typename F>
    struct is_lua_cfunctor<F, std::enable_if_t<meta_and_v<
        negation<std::is_reference<F>>,
        negation<std::is_convertible<F, ::lua_CFunction>>,
        std::is_invocable<F, ::lua_State*>,
        std::is_same<std::invoke_result_t<F, ::lua_State*>, int>
        >>> : std::true_type {};
    template <typename F>
    inline constexpr bool is_lua_cfunctor_v = is_lua_cfunctor<F>::value;

    // primitive type
    // 1. boolean
    // 2. integer
    // 3. floating point
    // 4. string
    template <typename T>
    using is_primitive_type = meta_or<
        is_bool<T>,
        is_integral<T>,
        is_floating_point<T>,
        is_string<T>
    >;
    template <typename T>
    inline constexpr bool is_primitive_type_v = is_primitive_type<T>::value;

    // type can shared_from_this and weak_from_this
    template <typename T, typename = void>
    struct is_shared_type : std::false_type {};
    template <typename T>
    struct is_shared_type<T, std::void_t<
        decltype(std::declval<T>().shared_from_this()),
        decltype(std::declval<T>().weak_from_this())
    >> : std::true_type {};

    // user data common
    template <typename T>
    using userdata_category = meta_and<
        negation<is_primitive_type<T>>,
        negation<is_callable<T>>,
        negation<is_manipulated_type<T>>,
        negation<std::is_reference<T>>,
        negation<std::is_pointer<T>>
    >;

    // userdata value category
    template <typename T>
    using userdata_value_category = meta_and<
        std::is_copy_assignable<T>,
        is_comparable<T>
    >;

    // type with value semantics
    template <typename T>
    using is_userdata_value_type = meta_and<
        userdata_category<T>,
        //userdata_value_category<T>,
        negation<is_shared_type<T>>
    >;
    template <typename T>
    inline constexpr bool is_userdata_value_type_v = is_userdata_value_type<T>::value;

    // type with reference semantics
    template <typename T>
    using is_userdata_reference_type = meta_and<
        userdata_category<T>,
        is_shared_type<T>
    >;
    template <typename T>
    inline constexpr bool is_userdata_reference_type_v = is_userdata_reference_type<T>::value;

    template <typename T>
    using is_userdata_type = meta_or<
        is_userdata_value_type<T>, is_userdata_reference_type<T>
    >;
    template <typename T>
    inline constexpr bool is_userdata_type_v = is_userdata_type<T>::value;
}

// key value type traits
namespace kath
{
    namespace detail
    {
        template <typename T>
        struct is_key_type_impl : meta_or<is_integral<T>, is_string<T>>{};
    }

    template <typename T>
    struct is_key_type : detail::is_key_type_impl<remove_rcv_t<T>> {};

    template <typename T>
    inline constexpr bool is_key_type_v = is_key_type<T>::value;

    namespace detail
    {
        template <typename Key, typename = void>
        struct extract_key_type_impl
        {
            using type = Key;
        };

        template <typename Key>
        struct extract_key_type_impl<Key, std::enable_if_t<is_char_array_v<std::remove_reference_t<Key>>>>
        {
            using type = std::add_lvalue_reference_t<std::add_const_t<remove_rcv_t<Key>>>;
        };

        template <typename Key>
        struct extract_key_type_impl<Key, std::enable_if_t<is_c_string_v<std::remove_reference_t<Key>>>>
        {
            using type = remove_rcv_t<Key>;
        };
    }

    template <typename Key>
    struct extract_key_type : detail::extract_key_type_impl<std::remove_reference_t<Key>>{};

    template <typename Key>
    using exract_key_type_t = typename extract_key_type<Key>::type;
}

// traits for tuple
namespace kath
{
    template <typename T, typename = void>
    struct is_valid_tuple : std::false_type
    {
        static constexpr size_t size = 0;
    };

    template <typename T>
    struct is_valid_tuple<T, std::void_t<decltype(std::tuple_size<T>::value)>>
    {
        static constexpr size_t size = std::tuple_size_v<T>;
        static constexpr bool value = size > 0;
    };

    template <typename T>
    inline constexpr bool is_valid_tuple_v = is_valid_tuple<T>::value;
}

// callable traits
namespace kath
{
    namespace detail
    {
        // generics 
        template <typename Callable, typename = void>
        struct callable_traits_impl;

        template <typename Callable>
        struct callable_traits_impl<Callable, std::enable_if_t<is_functor_v<Callable>>>
            : callable_traits_impl<decltype(&Callable::operator())> 
        {
            inline static constexpr bool is_pmf = false;
        };

        // specialization for function
        template <typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(Args...), void>
        {
            using result_type = Ret;
            using caller_type = void;
            using args_pack = std::tuple<Args...>;
            using signature_type = Ret(*)(Args...);

            inline static constexpr size_t arity = sizeof...(Args);
            inline static constexpr bool is_pmf = false;
            inline static constexpr bool has_const_qualifier = false;
            inline static constexpr bool has_volatile_qualifier = false;
            inline static constexpr bool has_lvalue_ref_qualifier = false;
            inline static constexpr bool has_rvalue_ref_qualifier = false;
        };

        template <typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(*)(Args...), void> : callable_traits_impl<Ret(Args...)> {};

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...), void> : callable_traits_impl<Ret(Args...)> 
        {
            using caller_type = T;
            using args_pack = std::tuple<Args...>;

            inline static constexpr bool is_pmf = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) &, void> : callable_traits_impl<Ret(T::*)(Args...)> 
        {
            inline static constexpr bool has_lvalue_ref_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) &&, void> : callable_traits_impl<Ret(T::*)(Args...)> 
        {
            inline static constexpr bool has_rvalue_ref_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) const, void> : callable_traits_impl<Ret(T::*)(Args...)>  
        {
            inline static constexpr bool has_const_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) const &, void> : callable_traits_impl<Ret(T::*)(Args...) &> 
        {
            inline static constexpr bool has_const_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) const &&, void> : callable_traits_impl<Ret(T::*)(Args...) &&> 
        {
            inline static constexpr bool has_const_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) volatile, void> : callable_traits_impl<Ret(T::*)(Args...)> 
        {
            inline static constexpr bool has_volatile_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) volatile &, void> : callable_traits_impl<Ret(T::*)(Args...) &> 
        {
            inline static constexpr bool has_volatile_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) volatile &&, void> : callable_traits_impl<Ret(T::*)(Args...) &&> 
        {
            inline static constexpr bool has_volatile_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) const volatile, void> : callable_traits_impl<Ret(T::*)(Args...) const> 
        {
            inline static constexpr bool has_volatile_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) const volatile &, void> : callable_traits_impl<Ret(T::*)(Args...) const&> 
        {
            inline static constexpr bool has_volatile_qualifier = true;
        };

        template <typename T, typename Ret, typename ... Args>
        struct callable_traits_impl<Ret(T::*)(Args...) const volatile &&, void> : callable_traits_impl<Ret(T::*)(Args...) const &&> 
        {
            inline static constexpr bool has_volatile_qualifier = true;
        };

    }

    template <typename Callable>
    struct callable_traits : detail::callable_traits_impl<std::remove_reference_t<Callable>> {};
}

// result traits
namespace kath
{
    template <typename T, typename = void>
    struct result_size : std::integral_constant<size_t, 1> {};

    template <>
    struct result_size<void, void> : std::integral_constant<size_t, 0> {};

    template <typename T>
    struct result_size<T, std::void_t<decltype(std::tuple_size_v<T>)>> : std::tuple_size<T> {};

    template <typename T>
    inline constexpr bool result_size_v = result_size<T>::value; 
}