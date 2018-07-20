#pragma once

namespace kath
{
    namespace func_detail
    {
        template <typename T, typename Tuple>
        struct append_tuple_front;

        template <typename T, typename...Args>
        struct append_tuple_front<T, std::tuple<Args...>>
        {
            using type = std::tuple<T, Args...>;
        };

        template <size_t I, typename IndexSequence>
        struct append_index_front;

        template <size_t I, size_t ... Is>
        struct append_index_front<I, std::index_sequence<Is...>>
        {
            using type = std::index_sequence<I, Is...>;
        };

        template <typename ArgsTuple, typename ... BindArgs>
        struct make_index_sequence_and_args_pack;

        template <typename ArgsTuple>
        struct make_index_sequence_and_args_pack<ArgsTuple>
        {
            using index_sequence = std::index_sequence<>;
            using args_pack = std::tuple<>;
        };

        // meta function to generate
        template <typename Arg0, typename ... Args, typename BindArg0, typename ... BindArgs>
        struct make_index_sequence_and_args_pack<std::tuple<Arg0, Args...>, BindArg0, BindArgs...>
        {
        private:
            using bind_arg0_type = std::remove_cv_t<std::remove_reference_t<BindArg0>>;
            constexpr static auto ph_value = std::is_placeholder<bind_arg0_type>::value;
            constexpr static bool is_not_placeholder = 0 == ph_value;
            using rests_make = make_index_sequence_and_args_pack<std::tuple<Args...>, BindArgs...>;
            using rests_index_sequence = typename rests_make::index_sequence;
            using rests_args_pack = typename rests_make::args_pack;
    
        public:
            using index_sequence = std::conditional_t<is_not_placeholder, rests_index_sequence,
                typename append_index_front<ph_value - 1, rests_index_sequence>::type>;
            using args_pack = std::conditional_t<is_not_placeholder, rests_args_pack,
                typename append_tuple_front<Arg0, rests_args_pack>::type>;
        };

        template <typename Indices, typename Ret, typename ArgsPack>
        struct make_std_function;

        template <size_t ... Is, typename Ret, typename ArgsPack>
        struct make_std_function<std::index_sequence<Is...>, Ret, ArgsPack>
        {
            using type = std::function<Ret(std::tuple_element_t<Is, ArgsPack>...)>;
        };

        // helper to make std::function type from bind
        template <typename F, typename ... Args>
        struct bind_to_function
        {
        private:
            using callable_traits_t = callable_traits<F>;
            using caller_type = typename callable_traits_t::caller_type;
            using args_pack_org = typename callable_traits_t::args_pack;
            using args_pack_t = std::conditional_t<callable_traits_t::is_pmf, 
                typename append_tuple_front<caller_type, args_pack_org>::type, args_pack_org>;
            using make_indices_args = make_index_sequence_and_args_pack<args_pack_t, Args...>;
            using index_sequence = typename make_indices_args::index_sequence;
            using args_pack = typename make_indices_args::args_pack;
        public:
            using type = typename make_std_function<index_sequence, typename callable_traits_t::result_type, args_pack>::type;
        };

        // implementation of bind pointer to member function
        template <typename PMF, size_t ... Is>
        inline static auto bind_pmf_impl(PMF pmf, std::index_sequence<Is...>)
        {
            using callable_traits_t = callable_traits<PMF>;
            using args_pack = typename callable_traits_t::args_pack;
            
            return [pmf](typename callable_traits_t::caller_type* c, std::tuple_element_t<Is, args_pack> ... args)
            {
                return std::invoke(pmf, c, std::forward<std::tuple_element_t<Is, args_pack>>(args)...);
            };
        }

        // bind raw pointer to member function
        template <typename PMF>
        inline static auto bind_pmf(PMF pmf)
        {
            using callable_traits_t = callable_traits<PMF>;
            return bind_pmf_impl(pmf, std::make_index_sequence<callable_traits_t::arity>{});
        } 

        template <typename PMF, typename Arg, typename ... Args>
        inline static auto bind_pmf(PMF pmf, Arg&& arg, Args&& ... args) 
            -> typename bind_to_function<PMF, Arg, Args...>::type
        {
            return std::bind(pmf, std::forward<Arg>(arg), std::forward<Args>(args)...);
        }

        template <typename Callable, typename Arg, typename ... Args>
        inline static auto bind_callable(Callable&& callable, Arg&& arg, Args&& ... args)
            -> typename bind_to_function<Callable, Arg, Args...>::type
        {
            return std::bind(callable, std::forward<Arg>(arg), std::forward<Args>(args)...);
        }

        template <typename F, typename ... Args>
        inline static auto bind_impl(F&& f, Args&& ... args)
        {
            using function_type = std::remove_reference_t<F>;
            if constexpr(std::is_member_function_pointer_v<function_type>)
            {
                return bind_pmf(std::forward<F>(f), std::forward<Args>(args)...);
            }
            else
            {
                return bind_callable(std::forward<F>(f), std::forward<Args>(args)...);
            }
        }
    }
}