#pragma once

namespace kath
{
    template <typename T>
    struct reflect_info {};
}

/* common macros */
#define KATH_APPLY_MACRO(macro) macro
#define KATH_ACCESS_MEMBER(c, m) &c::m
#define KATH_MEMBER_NAME_I(member) std::string_view{ #member }
#define KATH_MEMBER_NAME(member) KATH_MEMBER_NAME_I(member)

namespace kath
{
    template <typename T, typename F>
    struct to_member_function_pointer;

    template <typename T, typename Ret, typename ... Args>
    struct to_member_function_pointer<T, Ret(Args...)>
    {
        using type = Ret(T::*)(Args...);
    };

    template <typename T, typename Ret, typename ... Args>
    struct to_member_function_pointer<T, Ret(Args...) const>
    {
        using type = Ret(T::*)(Args...) const;
    };

    template <typename T, typename = void>
    struct is_reflected : std::false_type {};

    template <typename T>
    struct is_reflected<T, std::void_t<
        decltype(reflect_info<T>::name())
    >> : std::true_type {};

    template <typename T>
    inline constexpr bool is_reflected_v = is_reflected<T>::value;
}

/* non-static member function */
// function member without overloading
#define KATH_MFUNC_MEMBER_II_1 KATH_ACCESS_MEMBER
// function member with overloading
#define KATH_MFUNC_MEMBER_II_2(c, m, t) \
    static_cast<kath::to_member_function_pointer<c, t>::type>(KATH_ACCESS_MEMBER(c, m))
// reflect all members
#define KATH_MFUNC_MEMBER_II(c, ...) \
    KATH_APPLY_MACRO(BOOST_PP_OVERLOAD(KATH_MFUNC_MEMBER_II_, __VA_ARGS__)(c, __VA_ARGS__))
// for each macro to create names
#define KATH_MEMBER_NAME_0 KATH_MEMBER_NAME
#define KATH_MEMBER_NAME_1(tuple) KATH_MEMBER_NAME(BOOST_PP_TUPLE_ELEM(0, tuple))
#define KATH_MFUNC_MEMBER_NAME(r, data, i, t) \
    BOOST_PP_COMMA_IF(i) \
    KATH_APPLY_MACRO(BOOST_PP_CAT(KATH_MEMBER_NAME_, BOOST_PP_IS_BEGIN_PARENS(t))(t))
// for each macro to create pointer to member function
#define KATH_MFUNC_MEMBER_I_0 KATH_MFUNC_MEMBER_II
#define KATH_MFUNC_MEMBER_I_1(c, t) KATH_MFUNC_MEMBER_II(c, BOOST_PP_TUPLE_ENUM(t))
#define KATH_MFUNC_MEMBER_I(c, t) \
    BOOST_PP_CAT(KATH_MFUNC_MEMBER_I_, BOOST_PP_IS_BEGIN_PARENS(t))(c, t)
#define KATH_MFUNC_MEMBER(r, data, i, t) BOOST_PP_COMMA_IF(i) KATH_MFUNC_MEMBER_I(data, t)
// entry for all member function pointers
#define KATH_MFUNC_PROCESS(CLASS, N, ...) \
static constexpr auto mfunc_names() noexcept -> std::array<std::string_view, N> {\
    return { BOOST_PP_SEQ_FOR_EACH_I(KATH_MFUNC_MEMBER_NAME, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) };} \
static constexpr auto mfunc() noexcept { \
    return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(KATH_MFUNC_MEMBER, CLASS, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))); } \
static constexpr size_t mfunc_count = N;

/* static member function */
// for each macro to create names
#define KATH_SFUNC_MEMBER_NAME KATH_MFUNC_MEMBER_NAME
// for each macro to create function pointer
#define KATH_SFUNC_MEMBER_II_1 KATH_ACCESS_MEMBER
#define KATH_SFUNC_MEMBER_II_2(c, m, t) static_cast<std::add_pointer_t<t>>(KATH_ACCESS_MEMBER(c, m))
#define KATH_SFUNC_MEMBER_II(c, ...) \
    KATH_APPLY_MACRO(BOOST_PP_OVERLOAD(KATH_SFUNC_MEMBER_II_, __VA_ARGS__)(c, __VA_ARGS__))
#define KATH_SFUNC_MEMBER_I_0 KATH_SFUNC_MEMBER_II
#define KATH_SFUNC_MEMBER_I_1(c, t) KATH_SFUNC_MEMBER_II(c, BOOST_PP_TUPLE_ENUM(t))
#define KATH_SFUNC_MEMBER_I(c, t) \
    BOOST_PP_CAT(KATH_SFUNC_MEMBER_I_, BOOST_PP_IS_BEGIN_PARENS(t))(c, t)
#define KATH_SFUNC_MEMBER(r, data, i, t) BOOST_PP_COMMA_IF(i) KATH_SFUNC_MEMBER_I(data, t)
// entry for all static member function
#define KATH_SFUNC_PROCESS(CLASS, N, ...) \
static constexpr auto sfunc_names() noexcept -> std::array<std::string_view, N> {\
    return { BOOST_PP_SEQ_FOR_EACH_I(KATH_SFUNC_MEMBER_NAME, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) };} \
static constexpr auto mfunc() noexcept { \
    return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(KATH_SFUNC_MEMBER, CLASS, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));}\
static constexpr size_t sfunc_count = N;

/* static and no-static data member*/
#define KATH_DATA_MEMBER_NAME(r, data, i, t) BOOST_PP_COMMA_IF(i) KATH_MEMBER_NAME(t)
#define KATH_DATA_MEMBER(r, data, i, t) BOOST_PP_COMMA_IF(i) &data::t
// non-static data member
#define KATH_MDATA_PROCESS(CLASS, N, ...) \
static constexpr auto mdata_names() noexcept -> std::array<std::string_view, N> { \
    return { BOOST_PP_SEQ_FOR_EACH_I(KATH_DATA_MEMBER_NAME, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) };} \
static constexpr auto mdata() noexcept { \
    return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(KATH_DATA_MEMBER, CLASS, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));} \
static constexpr size_t mdata_count = N;
// static data member
#define KATH_SDATA_PROCESS(CLASS, N, ...) \
static constexpr auto sdata_names() noexcept -> std::array<std::string_view, N> {\
    return { BOOST_PP_SEQ_FOR_EACH_I(KATH_DATA_MEMBER_NAME, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) };} \
static constexpr auto sdata() noexcept { \
    return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(KATH_DATA_MEMBER, CLASS, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)));} \
static constexpr size_t sdata_count = N;

/* constructor */
#define KATH_TUPLE_TO_STD_TUPLE(t) std::tuple<BOOST_PP_TUPLE_ENUM(t)>
#define KATH_CTOR_LIST_ITEM(r, data, i, t) BOOST_PP_COMMA_IF(i) KATH_TUPLE_TO_STD_TUPLE(t)
#define KATH_CTOR_PROCESS(CLASS, N, ...) \
using ctor_list = std::tuple<BOOST_PP_SEQ_FOR_EACH_I(KATH_CTOR_LIST_ITEM, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))>; \
static constexpr size_t ctor_count = N;

/* dispatch */
#define KATH_MDATA(...) ((KATH_MDATA_PROCESS, (__VA_ARGS__)))
#define KATH_SDATA(...) ((KATH_SDATA_PROCESS, (__VA_ARGS__)))
#define KATH_MFUNC(...) ((KATH_MFUNC_PROCESS, (__VA_ARGS__)))
#define KATH_SFUNC(...) ((KATH_SFUNC_PROCESS, (__VA_ARGS__)))
#define KATH_CTOR(...)  ((KATH_CTOR_PROCESS,  (__VA_ARGS__)))
#define KATH_APPLY_PROCESS_II(c, p, n, ...) p(c, n, __VA_ARGS__)
#define KATH_APPLY_PROCESS_I(c, p, params) \
    KATH_APPLY_PROCESS_II(c, p, BOOST_PP_TUPLE_SIZE(params), BOOST_PP_TUPLE_ENUM(params))
#define KATH_APPLY_PROCESS(r, data, tuple) \
    KATH_APPLY_PROCESS_I(data, BOOST_PP_TUPLE_ELEM(0, tuple), BOOST_PP_TUPLE_ELEM(1, tuple))

/* MAIN entry */
#define KATH_REFLECT_1(c)                                               \
template<> struct kath::reflect_info<c> {                               \
    static constexpr std::string_view name() noexcept { return #c; }    \
};
#define KATH_REFLECT_2(c, seq)                                          \
template<> struct kath::reflect_info<c> {                               \
    static constexpr std::string_view name() noexcept { return #c; }    \
    BOOST_PP_SEQ_FOR_EACH(KATH_APPLY_PROCESS, c, seq)                   \
};
#define KATH_REFLECT(...) KATH_APPLY_MACRO(BOOST_PP_OVERLOAD(KATH_REFLECT_, __VA_ARGS__)(__VA_ARGS__))

/* useful mpl */
#define KATH_TMP_HAS(CAT)                                               \
template <typename T, typename = void>                                  \
struct BOOST_PP_CAT(has_reflect_, CAT) : std::false_type{};             \
template <typename T>                                                   \
struct BOOST_PP_CAT(has_reflect_, CAT)<T, std::void_t<                  \
    decltype(T::BOOST_PP_CAT(CAT, _names)()),                           \
    decltype(T::BOOST_PP_CAT(CAT, BOOST_PP_EMPTY())())                  \
>> : std::true_type{};                                                  \
template <typename T, typename = void>                                  \
struct BOOST_PP_CAT(has_visit_, CAT) : std::false_type{};               \
template <typename T>                                                   \
struct BOOST_PP_CAT(has_visit_, CAT)<T, std::void_t<                    \
    decltype(std::declval<T>().BOOST_PP_CAT(visit_, CAT)(               \
        std::declval<std::string_view>(),                               \
        std::declval<int>(),                                            \
        std::declval<size_t>(),                                         \
        std::declval<int>()                                             \
    ))>> : std::true_type{};

#define KATH_TMP_VISIT(CAT)                                             \
template <typename T, typename V>                                       \
inline static auto BOOST_PP_CAT(visit_, CAT)(reflect_info<T>, V const& visitor) noexcept \
-> std::enable_if_t<std::conjunction<                                   \
    BOOST_PP_CAT(has_reflect_, CAT)<reflect_info<T>>,                   \
    BOOST_PP_CAT(has_visit_, CAT)<V>>::value> {                         \
    using reflect_into_t = reflect_info<T>;                             \
    constexpr size_t size = reflect_into_t::BOOST_PP_CAT(CAT, _count);  \
    visit_loop(                                                         \
        reflect_into_t::BOOST_PP_CAT(CAT, _names)(),                    \
        reflect_into_t::BOOST_PP_CAT(CAT, BOOST_PP_EMPTY())(),          \
        [&visitor](auto ... args) {                                     \
            visitor.BOOST_PP_CAT(visit_, CAT)(std::forward<decltype(args)>(args)...); }, \
        std::make_index_sequence<size>{});                              \
}                                                                       \
template <typename T, typename V>                                       \
inline static auto BOOST_PP_CAT(visit_, CAT)(reflect_info<T>, V const& visitor) noexcept \
-> disable_if_t<std::conjunction<                                       \
    BOOST_PP_CAT(has_reflect_, CAT)<reflect_info<T>>,                   \
    BOOST_PP_CAT(has_visit_, CAT)<V>>::value>{}

/* some tricks to remove DRY code */
#define KATH_VISIT_SEQ (mdata)(sdata)(mfunc)(sfunc)
#define KATH_VISIT_PROC(r, data, elem) BOOST_PP_CAT(visit_, elem)(ri, visitor);
#define KATH_TMP_HAS_PROC(r, data, elem) KATH_TMP_HAS(elem)
#define KATH_TMP_VISIT_PROC(r, data, elem) KATH_TMP_VISIT(elem)

/* interface to visit the reflect_info */
namespace kath
{
    BOOST_PP_SEQ_FOR_EACH(KATH_TMP_HAS_PROC, _, KATH_VISIT_SEQ)

    template <typename T, typename = void>
    struct has_ctor_list : std::false_type {};
    template <typename T>
    struct has_ctor_list<T, std::void_t<
        typename T::ctor_list
        >> : std::true_type {};

    template <typename T, typename = void>
    struct has_visit_ctor : std::false_type {};
    template <typename T>
    struct has_visit_ctor<T, std::void_t<
        decltype(&T::visit_ctor)
        >> : std::true_type {};

    namespace reflect_detail
    {
        template <typename Arr, typename Tuple, typename F, size_t ... Is>
        inline static auto visit_loop(Arr const& names, Tuple const& members, std::index_sequence<Is...>)
        {
            swallow_t{
                (func(names[Is], std::get<Is>(members), Is, names), true) ...
            };
        }

        BOOST_PP_SEQ_FOR_EACH(KATH_TMP_VISIT_PROC, _, KATH_VISIT_SEQ)

        template <typename T, typename V>
        inline static void visit(reflect_info<T> ri, V const& visitor) noexcept
        {
            BOOST_PP_SEQ_FOR_EACH(KATH_VISIT_PROC, _, KATH_VISIT_SEQ)
        }
    }

    template <typename T, typename V>
    inline static void reflect_visit(V const& visitor) noexcept
    {
        reflect_detail::visit(reflect_info<std::remove_const_t<T>>{}, visitor);
    }

    template <typename T, typename V>
    inline static void reflect_visit(reflect_info<T> ri, V const& visitor) noexcept
    {
        reflect_detail::visit(ri, visitor);
    }
}

// reflect interface
namespace kath
{
    template <typename T>
    inline static constexpr auto reflect()
    {
        static_assert(is_reflected_v<T>);
        return reflect_info<T>{};
    }
}

// simulate the new reflexpr keyword
#define reflexpr(x) kath::reflect_info<std::remove_const_t<x>>