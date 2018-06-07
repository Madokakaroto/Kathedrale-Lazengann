#pragma once

// basic mpl
namespace kath
{
	// remove reference then remove const and volitale
	template <typename T>
	using remove_rcv = std::remove_cv<std::remove_reference_t<T>>;

    template <typename T>
    using remove_rcv_t = typename remove_rcv<T>::type;

	// negative
	template <typename T>
	struct negative : std::bool_constant<!T::value> {};

	template <typename T>
	inline constexpr bool negative_v = negative<T>::value;

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
	struct is_instance_of : std::false_type {};

	template <template <typename ...> class Tmpl, typename ... Args>
	struct is_instance_of<Tmpl<Args...>, Tmpl> : std::true_type {};

	template <typename T, template <typename ...> class Tmpl>
	inline constexpr bool is_instance_of_v = is_instance_of<T, Tmpl>::value;

	// equal
	template <typename LHS, typename RHS>
	struct meta_equal
	{
		static constexpr bool value = LHS::value == RHS::value;
	};

	template <typename LHS, typename RHS>
	inline constexpr bool meta_equal_v = meta_equal<LHS, RHS>::value;
}

// SFINAE
namespace kath
{
	template <bool Test, typename T = void>
	using disable_if = std::enable_if<negative_v<std::bool_constant<Test>>, T>;

	template <bool Test, typename T = void>
	using disable_if_t = typename disable_if<Test, T>::type;
}

// swallo
namespace kath
{
    struct swallow_t
    {
        template <typename ... T>
        swallow_t(T&& ... t)
        {}
    };
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
	using is_integral = meta_and<std::is_integral<T>, negative<is_bool<T>>>;

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
	struct is_char_array
	{
	private:
		using array_type = std::remove_reference_t<T>;
		using element_type = std::remove_all_extents_t<array_type>;
	public:
		static constexpr bool value = meta_and_v<
			std::is_array<array_type>,
			meta_equal<std::rank<array_type>, std::integral_constant<size_t, 1>>,
			std::is_same<std::add_const_t<element_type>, char const>
		>;
	};

	template <typename T>
	inline constexpr bool is_char_array_v = is_char_array<T>::value;

	namespace detail
	{
		template <typename T>
		using is_instance_of_std_string = is_instance_of<T, std::basic_string>;

		template <typename T>
		using is_instance_of_std_string_view = is_instance_of<T, std::basic_string_view>;
	}

	template <typename T, typename = void>
	struct element_type
	{
		using type = void;
		static constexpr bool value = false;
	};

	template <typename T>
	struct element_type<T, std::void_t<typename T::element_type>>
	{
		using type = element_type;
		static constexpr bool value = true;
	};

	template <typename T>
	using element_type_t = typename element_type<T>::type;

	// std::string with char as element_type
	template <typename T>
	using is_std_string = meta_and<
		detail::is_instance_of_std_string<T>, 
		std::is_same<element_type_t<T>, char> 
	>;

	template <typename T>
	inline constexpr bool is_std_string_v = is_std_string<T>::value;

	// std::string_view with char as element_type
	template <typename T>
	using is_string_view = meta_and<
		detail::is_instance_of_std_string_view<T>,
		std::is_same<element_type_t<T>, char>
	>;

	template <typename T>
	inline constexpr bool is_string_view_v = is_string_view<T>::value;

	// if is a string buffer
	template <typename T>
	using is_string_buffer = meta_and<
		meta_or<is_std_string<T>, is_string_view<T>>,
		std::is_same<element_type_t<T>, char>
	>;

	template <typename T>
	inline constexpr bool is_string_buffer_v = is_string_buffer<T>::value;

	// potential string value type in C
	template <typename T>
	using is_string = meta_or<
		is_c_string<T>,
		is_char_array<T>,
		is_string_buffer<T>
	>;

	template <typename T>
	inline constexpr bool is_string_v = is_string<T>::value;

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

	// is lua_CFuntion type
	template <typename T, typename = void>
	struct is_lua_cfunction : std::false_type {};

	template <typename T>
	struct is_lua_cfunction<T, std::void_t<decltype(static_cast<lua_CFunction>(std::declval<T>()))>>
		: std::true_type {};

	template <typename T>
	inline constexpr bool is_lua_cfunction_v = is_lua_cfunction<T>::value;

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

// user data
namespace kath
{
	namespace ext 
	{
		// for extention
		template <typename T, typename>
		struct manipulate_type
		{
			static constexpr bool value = false;
		};
	}

	template <typename T, typename = void>
	struct can_be_referenced_from_this : std::false_type {};

	template <typename T>
	struct can_be_referenced_from_this<T, std::void_t<
		decltype(std::declval<T>().ref_from_this()),
		decltype(std::declval<T>().weak_from_this())
	>> : std::true_type {};

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

	// type that will be specially treated
	template <typename T>
	struct is_manipulated_type 
	{
		static constexpr bool value = ext::manipulate_type<T>::value;
	};

	template <typename T>
	inline constexpr bool is_manipulated_type_v = is_manipulated_type<T>::value;

	// type with value semantics
	template <typename T>
	using is_value_type = meta_and<
		negative<is_primitive_type<T>>,
		negative<is_callable<T>>,
		negative<is_manipulated_type<T>>,
		negative<std::is_reference<T>>,
		negative<std::is_pointer<T>>,
		std::is_trivially_destructible<T>,
		std::is_copy_assignable<T>,
		std::is_copy_constructible<T>
	>;

	template <typename T>
	inline constexpr bool is_value_type_v = is_value_type<T>::value;

	// type with reference semantics
	template <typename T>
	using is_reference_type = meta_and<
		negative<is_primitive_type<T>>,
		negative<is_callable<T>>,
		negative<is_manipulated_type<T>>,
		negative<std::is_trivially_destructible<T>>,
		can_be_referenced_from_this<T>
	>;

	template <typename T>
	inline constexpr bool is_reference_type_v = is_reference_type<T>::value;

	template <typename T>
	using is_userdata_type = meta_or<
		is_value_type<T>, is_reference_type<T>
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
			using type = remove_rcv_t<Key>;
		};

		template <typename Key>
		struct extract_key_type_impl<Key, std::enable_if_t<is_char_array_v<Key>>>
		{
			using type = std::add_lvalue_reference_t<std::add_const_t<remove_rcv_t<Key>>>;
		};
	}

	template <typename Key>
	struct extract_key_type : detail::extract_key_type_impl<remove_rcv_t<Key>>{};

	template <typename Key>
	using exract_key_type_t = typename extract_key_type<Key>::type;
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
