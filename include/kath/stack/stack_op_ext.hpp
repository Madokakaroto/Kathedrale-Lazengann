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
    struct is_container : std::false_type {};

    template <typename T>
    struct is_container<T, std::void_t<
        decltype(std::declval<T>().size()),
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end()),
        typename T::value_type,
        std::enable_if_t<has_forward_iterator_v<T>>
    >> : std::true_type {};

    template <typename T>
    using is_sequential_container = meta_and<
        negative<is_string_buffer<T>>,
        is_container<T>
    >;

    template <typename T>
    inline constexpr bool is_sequential_container_v = is_sequential_container<T>::value;

    template <typename T, typename = void>
    struct is_associative_container : std::false_type {};

    template <typename T>
    struct is_associative_container<T, std::void_t<
        typename T::key_type,
        typenmae T::mapped_type,
    >> : is_container<T> {};

    template <typename T>
    inline constexpr bool is_associative_container_v = is_associative_container<T>::value;
}

namespace kath { namespace ext
{
    template <typename T>
    struct manipulate_type<T, std::enable_if_t<is_sequential_container_v<T>>
    {

        void stack_push(T const& c)
        {

        }

        auto stack_get()
        {
            return ...;
        }
    };
} }