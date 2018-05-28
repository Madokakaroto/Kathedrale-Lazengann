#pragma once

#ifdef KATH_REFLECT
#error KATH_REFLECT macro is already defined!
#else 
#define KATH_REFLECT(Type) \
static inline auto kath_reflect_meta_class((Type)&&) \
{ \
    static_assert(true, "Const is not appropriate!");\
    struct meta_class \
    { \
        static char const* name() noexcept \
        { \
            return #Type; \
        } \
    }; \
    return meta_class {}; \
}
#endif

namespace kath
{
    struct dummy_meta_class {};

    inline static dummy_meta_class kath_reflect_meta_class(...) noexcept
    {
        return dummy_meta_class{};
    }

    template <typename T>
    struct meta_class
    {
        using type = decltype(kath_reflect_meta_class(std::declval<T>()));
    };

    template <typename T>
    using meta_class_t = typename meta_class<T>::type;

    template <typename T>
    inline static char const* get_class_name() noexcept
    {
        using type = std::remove_cv_t<std::remove_reference_t<T>>;
        return meta_class_t<type>::name();
    }
}