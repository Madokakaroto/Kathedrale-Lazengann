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
	template <typename Func, typename ... Args>
	inline static auto bind(Func&& f, Args&& ... args)
	{
		return func_detail::bind_impl(std::forward<Func>(f), std::forward<Args>(args)...);
	}

	template <typename T, typename C>
	inline static auto bind_field_get(T C::* pmd) 
	{
		return [pmd](C* ptr) -> T const&
		{
			return ptr->*pmd;
		};
	}

	template <typename T, typename C>
	inline static auto bind_field_set(T C::* pmd)
	{
		return [pmd](C* ptr, T const& v) -> void
		{
			ptr->*pmd = v;
		};
	}
}

// lua_cfunctor
namespace kath
{
    namespace detail
    {
        template <typename F, size_t ... Is>
        inline static auto invoke_on_stack_impl(lua_State* L, F const& f, std::index_sequence<Is...>)
        {
            using callable_traits_t = callable_traits<F>;
            using args_pack = typename callable_traits_t::args_pack;
            return f(std::forward<std::tuple_element_t<Is, args_pack>>(stack_check<std::tuple_element_t<Is, args_pack>>(L, Is + 1))...);
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
                stack_push(L, std::move(result));
                return 1;
            }
        }

        template <typename Tuple>
        struct check_rvalue_reference;

        template <typename ... Args>
        struct check_rvalue_reference<std::tuple<Args...>>
            : meta_or<std::is_rvalue_reference<Args>...>
        {};
    }

	template <typename Func>
	struct lua_cfunctor
	{
		using function_type = std::remove_reference_t<Func>;
		using callable_traits_t = callable_traits<Func>;
		using result_type = typename callable_traits_t::result_type;
		using args_pack = typename callable_traits_t::args_pack;
		static constexpr size_t arity = callable_traits_t::arity;

        static_assert(negative_v<detail::check_rvalue_reference<args_pack>>, 
            "lua does not support move semantic with cpp");

	public:
		static void stack_push_callable(lua_State* L, Func func)
		{
			stack_push_impl(L, std::forward<function_type>(func));
		}

	private:
		static int invoke(lua_State* L)
		{
			using upvalue_placeholders::_1;
            auto& callable = stack_get<function_type>(L, _1);

            if constexpr(std::is_same_v<lua_CFunction, typename callable_traits_t::signature_type>)
            {
                return callable(L);
            }
            else
            {
                return detail::invoke_on_stack(L, callable);
            }
		}

		template <typename F>
		static auto stack_push_impl(lua_State* L, F&& f) -> std::enable_if_t<is_lua_cfunction_v<F>>
		{
			::lua_pushcclosure(L, f, 0);
		}

		static void gc_for_function_object(lua_State* L)
		{
			using orginal_function_type = std::remove_cv_t<function_type>;
			if constexpr(!std::is_trivially_destructible_v<orginal_function_type>)
			{
				::lua_createtable(L, 0, 1);
				stack_push(L, [](lua_State* L) -> int
				{
					// TODO ... check?
					auto ptr = detail::stack_get_emplaced_userdata<orginal_function_type>(L, 1);
					assert(ptr);
					ptr->~orginal_function_type();
					return 0;
				});
				set_field(L, "__gc");

				::lua_setmetatable(L, -2);
			}
		}

		template <typename F>
		static auto stack_push_impl(lua_State* L, F&& f) -> disable_if_t<is_lua_cfunction_v<F>>
		{
			detail::stack_push_userdata(L, std::forward<F>(f));
			gc_for_function_object(L);
			::lua_pushcclosure(L, &lua_cfunctor::invoke, 1);
		}
	};
}

// overload function
namespace kath
{
    namespace detail
    {
        template <typename F>
        inline static auto callable_type_erase_impl(F&& f)
            -> std::function<int(lua_State* L)>
        {
            using callable_traits_t = callable_traits<F>;
            using args_pack = typename callable_traits_t::args_pack;

            return [func = std::forward<F>(f)](lua_State* L) -> int
            {
                return detail::invoke_on_stack(L, func);
            };
        }

        template <typename T>
        inline static auto static_type_name() noexcept
        {
            static_assert(meta_or_v<is_primitive_type<T>, is_userdata_type<T>>, "Type not supported!");
            if constexpr(is_bool_v<T>)
            {
                return 'b';
            }
            else if constexpr(meta_or_v<is_floating_point<T>, is_integral<T>>)
            {
                return 'n';
            }
            else if constexpr(is_string_v<T>)
            {
                return 's';
            }
            else 
            {
                return get_class_name<T>();
            }
        }

        inline static char const* stack_type_name(lua_State* L, int arg = 1) noexcept
        {
            auto type = static_cast<basic_type>(::lua_type(L, arg));
            switch(type)
            {
            case basic_type::userdata:
            {
                // TODO ... higher level table op
                stack_guard guard{ L };
                ::lua_getmetatable(L, arg);
                fetch_field(L, "__name");
                return stack_get<char const*>(L, -1);
            }
            default:
                return basic_type_name(type);
            }
        }

        template <typename F, size_t ... Is>
        inline static std::string callable_type_string_impl(F&&, std::index_sequence<Is...>)
        {
            using args_pack = typename callable_traits<F>::args_pack;

            std::string result{};
            swallow_t{ (result += static_type_name<std::tuple_element_t<Is, args_pack>>()) ... };
            return result;
        }

        template <typename F, typename = std::enable_if_t<is_callable_v<std::remove_reference_t<F>>>>
        inline static auto callable_type_erase(F&& f)
        {
            return detail::callable_type_erase_impl(std::forward<F>(f));
        }

        template <typename F, typename = std::enable_if_t<is_callable_v<std::remove_reference_t<F>>>>
        inline static std::string callable_type_string(F&& f)
        {
            using callable_traits = callable_traits<F>;
            return detail::callable_type_string_impl(std::forward<F>(f), std::make_index_sequence<callable_traits::arity>{});
        }
    }

    class overload_functor
    {
    public:
        template <typename F, typename ... Fs>
        overload_functor(F&& f, Fs&& ... fs)
            : functions_()
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
            for(auto loop = 1; loop <= count; ++loop)
            {
                overload_name += basic_type_name(static_cast<basic_type>(::lua_type(L, loop)));
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
            auto overload_name = detail::callable_type_string(std::forward<F>(f));
            auto overload_function = detail::callable_type_erase(std::forward<F>(f));

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

    template <typename F, typename ... Fs>
    inline static overload_functor overload(F&& f, Fs&& ... fs)
    {
        return overload_functor{ std::forward<F>(f), std::forward<Fs>(fs)... };
    }
}

// constructor
namespace kath
{
    namespace detail
    {
        template <typename T, typename Tuple, size_t ... Is>
        inline static void emplace_new_object(lua_State* L, std::index_sequence<Is...>)
        {
            auto emplace_address = ::lua_newuserdata(L, sizeof(T));
            new (emplace_address) T{ std::forward<std::tuple_element_t<Is, Tuple>>(
                stack_get<std::tuple_element_t<Is, Tuple>>(L, Is + 1))... };
        }

        template <typename T, typename RefCounter, typename Tuple, size_t ... Is>
        inline static void make_ref_object(lua_State* L, std::index_sequence<Is...>)
        {
            auto ptr = make_ref<T, RefCounter>(std::forward<std::tuple_element_t<Is, Tuple>>(
                stack_get<std::tuple_element_t<Is, Tuple>>(L, Is + 1))...);
            
            detail::stack_push_userdata(L, std::move(ptr));
        }
    }

    template <typename T, typename ... Args>
    inline static auto constructor()
    {
        return [](lua_State* L)
        {
            if constexpr(is_value_type_v<T>)
            {
                detail::emplace_new_object<T, std::tuple<Args...>>(L, std::make_index_sequence<sizeof...(Args)>{});
            }
            else
            {
                static_assert(is_reference_type_v<T>, "Unsupported type!");
                using ref_count_ptr_t = decltype(std::declval<T>().ref_from_this());
                detail::make_ref_object<T, typename ref_count_ptr_t::ref_counter, std::tuple<Args...>>(
                    L, std::make_index_sequence<sizeof...(Args)>{});
            }
            
            // set class
            ::luaL_setmetatable(L, get_class_name<T>());
            return 1;
        };
    }
}

// lua callable
namespace kath
{
	
}