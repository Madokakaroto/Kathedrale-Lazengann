#pragma once

#include "functional/bind_detail.hpp"

// placeholders
namespace kath
{
    namespace upvalue_placeholders
    {
        template <int I>
        struct placeholder : std::integral_constant<int ,I>
        {
            operator int() const noexcept
            {
                return lua_upvalueindex(I);
            }
        };

        inline constexpr placeholder<1> _1{};
        inline constexpr placeholder<2> _2{};
        inline constexpr placeholder<3> _3{};
        inline constexpr placeholder<4> _4{};
        inline constexpr placeholder<5> _5{};
        inline constexpr placeholder<6> _6{};
        inline constexpr placeholder<7> _7{};
        inline constexpr placeholder<8> _8{};
        inline constexpr placeholder<9> _9{};
    }

    namespace register_placeholders
    {
        constexpr struct placehodler
        {
            operator int() const noexcept
            {
                return LUA_REGISTRYINDEX;
            }
        }
        _reg {};
    }
}

// bind
namespace kath
{
    namespace detail
    {
        template <typename T, typename = void>
        struct bind_property_type
        {
            using type = T;
        };

        template <typename T>
        struct bind_property_type<T, std::enable_if_t<is_userdata_type_v<T>>>
        {
            using type = std::add_lvalue_reference_t<T>;
        };

        template <typename T>
        struct bind_property_type<T, std::enable_if_t<is_manipulated_type_v<T>>>
        {
            using type = typename ext::manipulate_type<T>::type;
        };
    }

    template <typename T>
    using bind_property_type_t = typename detail::bind_property_type<T>::type;

    template <typename Func, typename ... Args>
    inline static auto bind(Func&& f, Args&& ... args)
    {
        return func_detail::bind_impl(std::forward<Func>(f), std::forward<Args>(args)...);
    }

    template <typename T, typename C>
    inline static auto bind_get(T C::* pmd)
    {
        return [pmd](C* ptr) -> bind_property_type_t<T>
        {
            return ptr->*pmd;
        };
    }

    template <typename T>
    inline static auto bind_get(T* pointer)
    {
        return [pointer]() -> bind_property_type_t<T>
        {
            return *pointer;
        };
    }

    template <typename T, typename C>
    inline static auto bind_set(T C::* pmd)
    {
        return [pmd](C* ptr, T const& v) -> void
        {
            ptr->*pmd = v;
        };
    }

    template <typename T>
    inline static auto bind_set(T* pointer)
    {
        return [pointer](T const& v) -> void
        {
            *pointer = v;
        };
    }
}

// detail
namespace kath { namespace detail 
{
    template <typename Tuple>
    struct check_rvalue_reference;
    template <typename ... Args>
    struct check_rvalue_reference<std::tuple<Args...>>
        : meta_or<std::is_rvalue_reference<Args>...>
    {};

    template <typename T, typename = void>
    struct has_method_signature_name : std::false_type {};
    template <typename T>
    struct has_method_signature_name<T, std::void_t<
        decltype(std::declval<T>().signature_name())
        >> : std::true_type{};

    template <typename ... Rets>
    struct traits_result_type;
    template <typename T>
    struct traits_result_type<T>
    {
        using type = T;
    };
    template <typename T1, typename T2, typename ... Rests>
    struct traits_result_type<T1, T2, Rests...>
    {
        static_assert(meta_and_v<negation<std::is_void<T1>>,
            negation<std::is_void<T2>>, negation<std::is_void<Rests>>...>);
        using type = std::tuple<T1, T2, Rests...>;
    };
    template <>
    struct traits_result_type<>
    {
        using type = void;
    };

    /// invoke on stack
    template <typename F, size_t ... Is>
    inline static auto invoke_on_stack_impl(lua_State* L, F const& f, std::index_sequence<Is...>)
    {
        using callable_traits_t = callable_traits<F>;
        using args_pack = typename callable_traits_t::args_pack;
        return f(stack_check<std::tuple_element_t<Is, args_pack>>(L, Is + 1)...);
    }

    template <typename F>
    inline static auto invoke_on_stack_impl(lua_State* L, F const& f)
    {
        using callable_traits_t = callable_traits<F>;
        return invoke_on_stack_impl(L, f, std::make_index_sequence<callable_traits_t::arity>{});
    }

    template <typename F>
    inline static int invoke_on_stack(lua_State* L, F const& f)
    {
        using callable_traits_t = callable_traits<F>;
        using result_type = typename callable_traits_t::result_type;
        if constexpr(std::is_void_v<result_type>)
        {
            invoke_on_stack_impl(L, f);
            return 0;
        }
        else
        {
            auto result = invoke_on_stack_impl(L, f);
            return stack_push_result(L, std::move(result));
        }
    }

    /// for overload functions
    template <typename ... Args>
    inline static std::string types2string()
    {
        std::string result{};
        swallow_t{ (result += get_type_name<Args>())... };
        return result;
    }

    template <>
    inline static std::string types2string<>()
    {
        return "";
    }

    template <typename Tuple, size_t ... Is>
    inline static std::string callable_signature_name(std::index_sequence<Is...>)
    {
        return types2string<std::tuple_element_t<Is, Tuple>...>();
    }

    template <typename T, typename Tuple, size_t ... Is>
    inline static void construct_new_object(lua_State* L, std::index_sequence<Is...>)
    {
        if constexpr(is_userdata_value_type_v<T>)
        {
            auto emplace_address = ::lua_newuserdata(L, sizeof(T));
            new (emplace_address) T{ stack_check<std::tuple_element_t<Is, Tuple>>(L, Is + 2)... };
            luaL_setmetatable(L, get_class_name<T>().c_str());
        }
        else
        {
            static_assert(is_userdata_reference_type_v<T>);
            auto ptr = std::make_shared<T>(stack_check<std::tuple_element_t<Is, Tuple>>(L, Is + 2)...);
            stack_push(std::move(ptr));
        }
    }

    // for pcall
    template <typename Ret, size_t ... Is>
    static auto stack_multi_result(lua_State* L, int begin, std::index_sequence<Is...>)
    {
        return std::make_tuple(std::move(stack_check<std::tuple_element_t<Is, Ret>>(L, Is + begin))...);
    }
}}

// lua_cfunctor
namespace kath
{
    template <typename Func>
    struct lua_cfunctor
    {
        using function_type = std::remove_reference_t<Func>;
        using callable_traits_t = callable_traits<Func>;
        using result_type = typename callable_traits_t::result_type;
        using args_pack = typename callable_traits_t::args_pack;
        static constexpr size_t arity = callable_traits_t::arity;

        static_assert(negation_v<detail::check_rvalue_reference<args_pack>>,
            "lua does not support move semantic with cpp");

    public:
        static void stack_push_callable(lua_State* L, Func func)
        {
            if constexpr(is_lua_cfunction_v<function_type>)
            {
                ::lua_pushcclosure(L, func, 0);
            }
            else
            {
                detail::stack_push_userdata(L, std::forward<Func>(func));
                if constexpr(negation_v<std::is_trivially_destructible<function_type>>)
                    gc_for_function_object(L);
                ::lua_pushcclosure(L, &lua_cfunctor::invoke, 1);
            }
        }

    private:
        static int invoke(lua_State* L)
        {
            using upvalue_placeholders::_1;
            auto callable = detail::stack_get_value_userdata<function_type>(L, _1);
            assert(callable);

            if constexpr(is_lua_cfunctor_v<function_type>)
            {
                return (*callable)(L);
            }
            else
            {
                return detail::invoke_on_stack(L, *callable);
            }
        }

        static void gc_for_function_object(lua_State* L)
        {
            using raw_func_t = std::remove_cv_t<function_type>;
            auto r = ::lua_gettop(L);
            ::lua_createtable(L, 0, 1);
            stack_push(L, [](lua_State* L) -> int
            {
                auto ptr = detail::stack_get_value_userdata<raw_func_t>(L, 1);
                ptr->~raw_func_t();
                return 0;
            });
            set_field(L, "__gc");
            ::lua_setmetatable(L, -2);
        }
    };
}

// constructor
namespace kath
{
    template <typename T, typename ... Args>
    class constructor_t
    {
    public:
        int operator() (::lua_State* L) const
        {
            detail::construct_new_object<T, std::tuple<Args...>>(L,
                std::make_index_sequence<sizeof...(Args)>{});
            return 1;
        }

        static std::string signature_name()
        {
            return detail::types2string<Args...>();
        }
    };

    template <typename T, typename ... Args>
    inline static constexpr auto constructor() noexcept ->
        std::enable_if_t<
            meta_and_v<is_userdata_type<T>, std::is_constructible<T, Args...>>,
            constructor_t<T, Args...>
        >
    {
        return {};
    }

    template <typename T, typename ... Args>
    inline static constexpr auto constructor(type_list<T, Args...>) noexcept ->
        std::enable_if_t<
            meta_and_v<is_userdata_type<T>, std::is_constructible<T, Args...>>,
            constructor_t<T, Args...>
        >
    {
        return {};
    }
}

// overload function
namespace kath
{
    template <typename F, typename RawF = std::remove_reference_t<F>>
    inline static auto callable_type_erase(F&& f)
    {
        if constexpr(meta_or_v<is_lua_cfunction<RawF>, is_lua_cfunctor<RawF>>)
            return std::forward<F>(f);
        else 
        {
            return [func = std::forward<F>(f)](lua_State* L) -> int
            {
                return detail::invoke_on_stack(L, func);
            };
        }	
    }

    template <typename F>
    inline static std::string callable_signature_name(F const& f)
    {
        if constexpr(detail::has_method_signature_name<F>::value)
        {
            return f.signature_name();
        }
        else
        {
            using callable_traits_t = callable_traits<F>;
            using args_pack = typename callable_traits_t::args_pack;
            constexpr auto arity = callable_traits_t::arity;
            return detail::callable_signature_name<args_pack>(std::make_index_sequence<arity>{});
        }
    }

    class overload_functor
    {
    public:
        template <typename F, typename ... Fs>
        overload_functor(F&& f, Fs&& ... fs)
        {
            swallow_t {  
                (emplace_function(std::forward<F>(f)), true), 
                (emplace_function(std::forward<Fs>(fs)), true)... 
            };
        }

        ~overload_functor() = default;
        overload_functor(overload_functor const&) = default;
        overload_functor& operator=(overload_functor const&) = default;
        overload_functor(overload_functor&&) = default;
        overload_functor& operator=(overload_functor&&) = default;

        int operator()(lua_State* L) const
        {
            std::string overload_name{};
            auto count = ::lua_gettop(L);
            for(auto loop = 2; loop <= count; ++loop)
            {
                overload_name += stack_type_name(L, loop);
            }
            
            auto itr = functions_.find(overload_name);
            if(itr == functions_.end())
            {
                // TODO ... better runtime error
                using namespace std::string_literals;
                std::string error_str = "Cannot find overload resolution for "s + overload_name;
                throw std::runtime_error{ std::move(error_str) };
            }

            return itr->second(L);
        }

    private:
        template <typename F>
        void emplace_function(F&& f)
        {
            auto overload_name = callable_signature_name(std::forward<F>(f));
            auto overload_function = callable_type_erase(std::forward<F>(f));

            if(!functions_.emplace(std::move(overload_name), std::move(overload_function)).second)
            {
                // TODO ... better exception
                using namespace std::string_literals;
                std::string error_str = "Overload functor signature("s + overload_name + ") conflicted!";
                throw std::runtime_error{ std::move(error_str) };
            }
        }

    private:
        std::map<std::string, std::function<int(lua_State* L)>> functions_;
    };

    template <typename F0, typename F1, typename ... Fs>
    inline static overload_functor overload(F0&& f0, F1&& f1, Fs&& ... fs)
    {
        return overload_functor{ 
            std::forward<F0>(f0), 
            std::forward<F1>(f1), 
            std::forward<Fs>(fs)... 
        };
    }
}

// lua callable
namespace kath
{
    struct lua_pcall
    {
        template <typename ArgsPack>
        static void do_call_impl(lua_State* L, ArgsPack&& args_pack, int ret_count)
        {
            assert(ret_count >= 0);
            auto arity = stack_push_args_pack(L, std::forward<ArgsPack>(args_pack));
            lua_pcall(L, arity, ret_count, 0);
        }

        template <typename Ret, typename ArgsPack>
        static auto do_call_multi_return(lua_State* L, ArgsPack&& args_pack)
        {
            auto begin = ::lua_gettop(L);
            do_call_impl(L, std::forward<ArgsPack>(args_pack), is_valid_tuple<Ret>::size);
            return detail::stack_multi_result<Ret>(L, begin, std::make_index_sequence<is_valid_tuple<Ret>::size>{});
        }

        template <typename Ret, typename ArgsPack>
        static auto do_call_single_return(lua_State* L, ArgsPack&& args_pack)
        {
            auto begin = ::lua_gettop(L);
            do_call_impl(L, std::forward<ArgsPack>(args_pack), 1);
            return stack_check<Ret>(L, begin);
        }

        template <typename ... Rets, typename ArgsPack>
        static decltype(auto) do_call(lua_State* L, ArgsPack&& args_pack)
        {
            using args_pack_t = std::remove_reference_t<ArgsPack>;
            using result_type = typename detail::traits_result_type<Rets...>::type;

            if constexpr(std::is_void_v<result_type>)
            {
                do_call_impl(L, std::forward<ArgsPack>(args_pack), 0);
            }
            else if constexpr(is_valid_tuple_v<result_type>)
            {
                return do_call_multi_return<result_type>(L, std::forward<ArgsPack>(args_pack));
            }
            else
            {
                return do_call_single_return<result_type>(L, std::forward<ArgsPack>(args_pack));
            }
        }
    };

    template <typename ... Rets, typename ... Args>
    inline static decltype(auto) pcall(lua_State* L, Args&& ... args)
    {
        return lua_pcall::do_call<Rets...>(L, std::forward_as_tuple(std::forward<Args>(args)...));
    }
}