#pragma once

// detail
namespace kath { namespace detail 
{
	// push object on stack
	template <typename T>
	inline static void stack_push_userdata(lua_State* L, T&& t)
	{
		using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
		auto ptr = lua_newuserdata(L, sizeof(value_type));
		new (ptr) value_type{ std::forward<T>(t) };
	}

	// emplace object on stack
	template <typename T, typename ... Args>
	inline static void stack_emplace_userdata(lua_State* L, Args&& ... args)
	{
		using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
		auto ptr = lua_newuserdata(L, sizeof(value_type));
		new (ptr) value_type{ std::forward<Args>(args)... };
	}

	template <typename T>
	inline static auto stack_get_value_userdata(lua_State* L, int index)
	{
		auto ptr = ::lua_touserdata(L, index);
		assert(ptr);
		return reinterpret_cast<T*>(ptr);
	}

	template <typename T>
	inline static auto stack_get_reference_userdata(lua_State* L, int index)
	{
		auto ptr = reinterpret_cast<T**>(::lua_touserdata(L, index));
		assert(ptr && *ptr);
		return *ptr;
	}

	template <size_t Index, typename Tuple, typename
		Type = decltype(std::get<Index>(std::declval<Tuple>()))>
	inline static bool stack_forward_push(lua_State* L, Tuple&& t)
	{
		stack_push(L, std::forward<Type>(std::get<Index>(t)));
		return true;
	}

	template <typename T, size_t ... Is>
	inline static int stack_push_result_impl(lua_State* L, T&& t, std::index_sequence<Is...>)
	{
		swallow_t{ stack_forward_push<Is>(L, std::forward<T>(t))... };
		return sizeof...(Is);
	}
} }

// stack push
namespace kath
{
	// nil
	inline static void stack_push(lua_State* L, nil_t = nil)
	{
		::lua_pushnil(L);
	}

	// boolean
	inline static void stack_push(lua_State* L, bool value)
	{
		::lua_pushboolean(L, static_cast<int>(value));
	}

	// integer
	template <typename T>
	inline static auto stack_push(lua_State* L, T value) 
		-> std::enable_if_t<is_integral_v<T>>
	{
		::lua_pushinteger(L, static_cast<lua_Integer>(value));
	}

	// floating point
	template <typename T>
	inline static auto stack_push(lua_State* L, T value) 
		-> std::enable_if_t<is_floating_point_v<T>>
	{
		::lua_pushnumber(L, static_cast<lua_Number>(value));
	}

	// c-style string
	template <typename T>
	inline static auto stack_push(lua_State* L, T const& value)
		-> std::enable_if_t<meta_or_v<is_c_string<T>, is_char_array<T>>, char const*>
	{
		return ::lua_pushstring(L, value);
	}

	// std-style string
	template <typename T>
	inline static auto stack_push(lua_State* L, T const& value) 
		-> std::enable_if_t<is_string_buffer_v<T>, char const*>
	{
		return ::lua_pushlstring(L, value.data(), value.size());
	}

	// callable
	template <typename T>
	inline static auto stack_push(lua_State* L, T&& t) 
		-> std::enable_if_t<is_callable_v<std::remove_reference_t<T>>>
	{
		//using lua_cfunctor_t = lua_cfunctor<T>;
		//return lua_cfunctor_t::stack_push_callable(L, std::forward<T>(t));
	}

	// extension type - kath implement std::vector, std::map and std::shared_ptr by default
	template <typename T, typename RawT = std::remove_reference_t<T>>
	inline static auto stack_push(lua_State* L, T&& t) 
		-> std::enable_if_t<is_manipulated_type_v<RawT>>
	{
		using manipulate_type_t = ext::manipulate_type<RawT>;
		manipulate_type_t::stack_push(std::forward<T>(t));
	}

	template <typename T, typename RawT = std::remove_reference_t<T>>
	inline static auto stack_push(lua_State* L, T&& t) 
		-> std::enable_if_t<is_userdata_type_v<RawT>>
	{
		if constexpr(is_userdata_value_type_v<RawT>) 
		{
			detail::stack_push_userdata(L, std::forward<T>(t));
			::luaL_setmetatable(L, get_class_name<T>());
		}
		else
		{
			static_assert(is_userdata_reference_type_v<RawT>);
			stack_push(L, std::move(t.shared_from_this()));
		}
	}

	// for pointer
	// TODO ... align with stack_get and stack_check
	template <typename T>
	inline static auto stack_push(lua_State* L, T* ptr) 
		-> disable_if<meta_or_v<is_c_string<T*>, std::is_function<T>>>
	{
		stack_push(L, *ptr);
	}
}

// stack get
namespace kath
{
	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept 
		-> std::enable_if_t<is_bool_v<T>, T>
	{
		return static_cast<T>(::lua_toboolean(L, index));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) 
		-> std::enable_if_t<is_integral_v<T>, T>
	{
		return static_cast<T>(::lua_tointegerx(L, index, nullptr));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) 
		-> std::enable_if_t<is_floating_point_v<T>, T>
	{
		return static_cast<T>(::lua_tonumberx(L, index, nullptr));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) 
		-> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::lua_tolstring(L, index, nullptr);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) 
		-> std::enable_if_t<is_string_buffer_v<T>, T>
	{
		size_t len{ 0 };
		auto ptr = ::lua_tolstring(L, index, &len);
		return { ptr, len };
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) 
		-> std::enable_if_t<is_manipulated_type_v<T>, T>
	{
		return ext::manipulate_type<T>::stack_get(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) 
		-> std::enable_if_t<is_lua_cfunction_v<T>, ::lua_CFunction>
	{
		return ::lua_tocfunction(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1)
		-> std::enable_if_t<meta_and_v<is_callable<T>, negation<is_lua_cfunction<T>>>, T&>
	{
		return *detail::stack_get_value_userdata<T>(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1)
		-> std::enable_if_t<is_userdata_type_v<T>, T&>
	{
		if constexpr(is_userdata_value_type_v<T>)
		{
			return *detail::stack_get_value_userdata<T>(L, index);
		}
		else
		{
			static_assert(is_userdata_reference_type_v<T>);
			return *detail::stack_get_reference_userdata<T>(L, index);
		}
	}

	// for extension
	//template <typename T>
	//inline static auto stack_get(lua_State* L, int index = -1)
	//	-> std::enable_if_t<is_manipulated_type_v<T>, 
	//		decltype(ext::manipulate_type<T>::stack_get(
	//			std::declval<lua_State*>(), std::declval<int>()))>
	template <typename T>
	inline static decltype(auto) stack_get(lua_State* L,
		std::enable_if_t<is_manipulated_type_v<T>, int> index = -1)
	{
		return ext::manipulate_type<T>::stack_get(L, index);
	}

	// get reference
	template <typename T>
	inline static decltype(auto) stack_get(lua_State* L,
		std::enable_if_t<std::is_reference_v<T>, int> index = -1)
	{
		return stack_get<std::remove_reference_t<T>>(L, index);
	}

	// get pointer
	template <typename T, typename RawT = std::remove_pointer_t<T>>
	inline static auto stack_get(lua_State* L, int index = -1)
		-> std::enable_if_t<meta_and_v<std::is_pointer<T>, 
			meta_or<is_userdata_type<RawT>, is_manipulated_type<RawT>>>, T>
	{
		if constexpr(is_userdata_value_type_v<RawT>)
		{
			return detail::stack_get_value_userdata<RawT>(L, index);
		}
		else if constexpr(is_userdata_reference_type_v<RawT>)
		{
			return detail::stack_get_reference_userdata<RawT>(L, index);
		}
		else
		{
			// TODO ... stack_get_ptr check at SFINAE
			static_assert(is_manipulated_type_v<RawT>);
			return ext::manipulate_type<RawT>::stack_get_ptr(L, index);
		}
	}
}

// stack check
namespace kath
{
	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) noexcept 
		-> std::enable_if_t<is_bool_v<T>, T>
	{
		return stack_get<T>(L, arg);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) 
		-> std::enable_if_t<is_integral_v<T>, T>
	{
		return static_cast<T>(::luaL_checkinteger(L, arg));
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) 
		-> std::enable_if_t<is_floating_point_v<T>, T>
	{
		return static_cast<T>(::luaL_checknumber(L, arg));
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) 
		-> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::luaL_checklstring(L, arg, nullptr);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) 
		-> std::enable_if_t<is_string_buffer_v<T>, T>
	{
		size_t len{ 0 };
		auto ptr = ::luaL_checklstring(L, arg, &len);
		return { ptr, len };
	}

	template <typename T>
	inline static decltype(auto) stack_check(lua_State* L,
		std::enable_if_t<is_manipulated_type_v<T>, int> arg = 1)
	{
		return ext::manipulate_type<T>::stack_check(L, arg);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1)
		-> std::enable_if_t<is_userdata_type_v<T>, T&>
	{
		::luaL_checkudata(L, arg, get_class_name<T>());
		return stack_get<T>(L, arg);
	}

	template <typename T, typename RawT = std::remove_pointer_t<T>>
	inline static auto stack_check(lua_State* L, int arg = 1)
		-> std::enable_if_t<meta_and_v<std::is_pointer<T>,
			meta_or<is_userdata_type<RawT>, is_manipulated_type<RawT>>>, T>
	{
		if constexpr(is_userdata_type_v<RawT>)
		{
			::luaL_checkudata(L, arg, get_class_name<RawT>());
			return stack_get<T>(L, arg);
		}
		else
		{
			static_assert(is_manipulated_type_v<RawT>);
			return ext::manipulate_type<RawT>::stack_check_ptr(L, arg);
		}
	}
}

// the rest of stack op
namespace kath
{
	inline static void stack_pop(lua_State* L, int count = 1)
	{
		lua_pop(L, count);
	}

	inline static void check_type(lua_State* L, basic_type type, int index = -1)
	{
		::luaL_checktype(L, index, static_cast<int32_t>(index));
	}

	inline static void stack_duplicate(lua_State* L, int index = -1)
	{
		::lua_pushvalue(L, index);
	}

	template <typename T, typename Type = std::remove_reference_t<T>>
	inline static auto stack_push_result(lua_State* L, T&& t) -> std::enable_if_t<is_valid_tuple_v<Type>, int>
	{
		return detail::stack_push_result_impl(L, std::forward<T>(t), std::make_index_sequence<std::tuple_size_v<Type>>{});
	}

	template <typename T, typename Type = std::remove_reference_t<T>>
	inline static auto stack_push_result(lua_State* L, T&& t) -> disable_if_t<is_valid_tuple_v<Type>, int>
	{
		stack_push(L, std::forward<T>(t));
		return 1;
	}

	template <typename T, typename Type = std::remove_reference_t<T>>
	inline static auto stack_push_args_pack(lua_State* L, T&& t)
	{
		return detail::stack_push_result_impl(L, std::forward<T>(t), std::make_index_sequence<std::tuple_size_v<Type>>{});
	}
}