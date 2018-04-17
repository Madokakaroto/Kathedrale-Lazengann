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

// basic type traits
namespace kath
{
	///////
	template <typename T>
	using is_bool = std::is_same<T, bool>;

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
	using is_c_array_string = meta_and<
		std::is_array<T>, 
		meta_equal<std::rank<T>, std::integral_constant<size_t, 1>>,
		std::is_same<std::add_const_t<std::remove_all_extents_t<T>>, char const>
	>;

	template <typename T>
	inline constexpr bool is_c_array_string_v = is_c_array_string<T>::value;

	// if is a string buffer
	template <typename T, typename = void>
	struct is_string_buffer : std::false_type {};

	template <typename T>
	struct is_string_buffer<T, std::void_t<
		decltype(std::declval<T>().data()),
		decltype(std::declval<T>().size()),
		std::enable_if_t<std::is_same_v<typename T::value_type, char>>
		>> : std::true_type {};

	template <typename T>
	inline constexpr bool is_string_buffer_v = is_string_buffer<T>::value;

	// is std::string and compatible data structure
	template <typename T, typename = void>
	struct is_std_string_compatible : std::false_type {};

	template<typename T>
	struct is_std_string_compatible<T, std::void_t<
		std::enable_if_t<is_string_buffer_v<T>>,
		decltype(std::declval<T>().c_str())
		>> : std::true_type {};

	template <typename T>
	inline constexpr bool is_std_string_compatible_v = is_std_string_compatible<T>::value;

	// is std::string_view and compatible data structure
	template <typename T, typename = void>
	struct is_string_view_compatible : std::false_type {};

	template <typename T>
	struct is_string_view_compatible<T, std::void_t<
		std::enable_if_t<is_string_buffer_v<T>>,
		std::enable_if_t<std::is_trivially_destructible_v<T>>
		>> : std::true_type {};

	template <typename T>
	inline constexpr bool is_string_view_compatible_v = is_string_view_compatible<T>::value;

	// potential string value type in C
	template <typename T>
	using is_string = meta_or<
		is_c_string<T>,
		is_c_array_string<T>,
		is_string_buffer<T>
	>;

	template <typename T>
	inline constexpr bool is_string_v = is_string<T>::value;
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
		struct extract_key_type_impl<Key, std::enable_if_t<is_c_array_string_v<Key>>>
		{
			using type = std::add_lvalue_reference_t<std::add_const_t<remove_rcv_t<Key>>>;
		};

		//template <typename Key>
		//struct extract_key_type_impl<Key, std::enable_if_t<is_c_string_v<Key>>>
		//{
		//	using type = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<Key>>>;
		//};
	}

	template <typename Key>
	struct extract_key_type : detail::extract_key_type_impl<remove_rcv_t<Key>>{};

	template <typename Key>
	using exract_key_type_t = typename extract_key_type<Key>::type;
}

namespace kath
{
	template <bool Test, typename T = void>
	using disable_if = std::enable_if<negative_v<std::bool_constant<Test>>, T>;

	template <bool Test, typename T = void>
	using disable_if_t = typename disable_if<Test, T>::type;
}