#pragma once

namespace kath
{
    template <typename T, typename = void>
    struct is_sequence_container : std::false_type {};

    template <typename T>
    struct is_sequence_container<T, std::void_t<
        decltype(std::declval<T>().size()),
        
    >> : std::true_type {};
}

namespace kath { namespace ext
{

} }