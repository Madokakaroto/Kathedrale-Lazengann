#pragma once

namespace kath
{
	struct context
	{
		lua_State* L;
	};
}

// stack push
namespace kath 
{
	namespace detail
	{
		template<typename T>
		inline static size_t check_char_array(T const& value)
		{
			constexpr size_t len = std::extent_v<T> - 1;
			assert('\0' == value[len]);
			return len;
		}
	}
	inline static void stack_push(lua_State* L, nil_t = nil)
	{
		::lua_pushnil(L);
	}

	inline static void stack_push(lua_State* L, bool value)
	{
		::lua_pushboolean(L, static_cast<int>(value));
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T value) -> std::enable_if_t<is_integral_v<T>>
	{
		::lua_pushinteger(L, static_cast<lua_Integer>(value));
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T value) -> std::enable_if_t<is_floating_point_v<T>>
	{
		::lua_pushnumber(L, static_cast<lua_Number>(value));
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T const& value) -> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::lua_pushstring(L, value);
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T const& value) -> std::enable_if_t<is_c_array_string_v<T>, char const*>
	{
		auto len = detail::check_char_array(value);
		return ::lua_pushlstring(L, value, len);
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T const& value) -> std::enable_if_t<is_string_buffer_v<T>, char const*>
	{
		return ::lua_pushlstring(L, value.data(), value.size());
	}
}

// statck cast
namespace kath 
{
	template <typename T>
	inline static auto stack_cast(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_bool_v<T>, T>
	{
		return static_cast<T>(::lua_toboolean(L, index));
	}

	template <typename T>
	inline static auto stack_cast(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_integral_v<T>, T>
	{
		return static_cast<T>(::lua_tointegerx(L, index, nullptr));
	}

	template <typename T>
	inline static auto stack_cast(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_floating_point_v<T>, T>
	{
		return static_cast<T>(::lua_tonumberx(L, index, nullptr));
	}

	template <typename T>
	inline static auto stack_cast(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::lua_tolstring(L, index, nullptr);
	}

	template <typename T>
	inline static auto stack_cast(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_string_buffer_v<T>, T>
	{
		size_t len { 0 };
		auto ptr = ::lua_tolstring(L, index, &len);
		return { ptr, len };
	}
}

// stack check cast
namespace kath 
{
	template <typename T>
	inline static auto stack_check_cast(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_bool_v<T>, T>
	{
		return stack_cast<T>(L, index);
	}

	template <typename T>
	inline static auto stack_check_cast(lua_State* L, int index = -1) -> std::enable_if_t<is_integral_v<T>, T>
	{
		return static_cast<T>(::luaL_checkinteger(L, index));
	}

	template <typename T>
	inline static auto stack_check_cast(lua_State* L, int index = -1) -> std::enable_if_t<is_floating_point_v<T>, T>
	{
		return static_cast<T>(::luaL_checknumber(L, index));
	}

	template <typename T>
	inline static auto stack_check_cast(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::luaL_checklstring(L, index, nullptr);
	}

	template <typename T>
	inline static auto stack_check_cast(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_string_buffer_v<T>, T>
	{
		size_t len { 0 };
		auto ptr = ::luaL_checklstring(L, index, &len);
		return { ptr, len };
	}
}

// other primative stack operation
namespace kath
{
	inline static void stack_pop(lua_State* L, int count = 1)
	{
		lua_pop(L, count);
	}
}

// fetch
namespace kath
{
	namespace detail
	{
		template <typename T>
		inline static void check_string_view(T const& value)
		{
			assert(std::strlen(value.data()) == value.size());
		}
	}

	template <typename T>
	inline static auto fetch_global(lua_State* L, T const& key) -> std::enable_if_t<is_c_string_v<T>, data_type_t>
	{
		return data_type_t{ ::lua_getglobal(L, value) };
	}

	template <typename T>
	inline static auto fetch_global(lua_State* L, T const& key) -> std::enable_if_t<is_c_array_string_v<T>, data_type_t>
	{
		detail::check_char_array(key);
		return data_type_t{ ::lua_getglobal(L, key) };
	}

	template <typename T>
	inline static auto fetch_global(lua_State* L, T const& key) 
		-> std::enable_if_t<is_std_string_compatible_v<T>, data_type_t>
	{
		return data_type_t{ ::lua_getglobal(L, key.c_str()) };
	}

	template <typename T>
	inline static auto fetch_global(lua_State* L, T const& key) 
		-> std::enable_if_t<is_string_view_compatible_v<T>, data_type_t>
	{
		detail::check_string_view(key);
		return data_type_t{ ::lua_getglobal(L, key.data()) };
	}

	template <typename T>
	inline static auto fetch_field(lua_State* L, T const& key, int index = -1) 
		-> std::enable_if_t<is_c_string_v<T>, data_type_t>
	{
		return data_type_t{ ::lua_getfield(L, index, key) };
	}

	template <typename T>
	inline static auto fetch_field(lua_State* L, T const& key, int index = -1) 
		-> std::enable_if_t<is_c_array_string_v<T>, data_type_t>
	{
		detail::check_char_array(key);
		return data_type_t{ ::lua_getfield(L, index, key) };
	}

	template <typename T>
	inline static auto fetch_field(lua_State* L, T const& key, int index = -1)
		-> std::enable_if_t<is_std_string_compatible_v<T>, data_type_t>
	{
		return data_type_t{ ::lua_getfield(L, index, key.c_str()) };
	}

	template <typename T>
	inline static auto fetch_field(lua_State* L, T const& key, int index = -1) 
		-> std::enable_if_t<meta_or_v<is_string_view_compatible<T>, is_floating_point<T>, is_bool<T>>, data_type_t>
	{
		stack_push(L, key);
		return data_type_t{ ::lua_gettable(L, index - 1) };
	}

	template <typename T>
	inline static auto fetch_field(lua_State* L, T const& key, int index = - 1)
		-> std::enable_if_t<is_integral_v<T>, data_type_t>
	{
		return data_type_t{ ::lua_geti(L, index, static_cast<lua_Number>(key)) };
	}
}

// set
namespace kath
{
	template <typename Key>
	auto set_global(lua_State* L, Key const& key) -> std::enable_if_t<is_c_string_v<Key>>
	{
		::lua_setglobal(L, key);
	}

	template <typename Key>
	auto set_global(lua_State* L, Key const& key) -> std::enable_if_t<is_c_array_string_v<Key>>
	{
		detail::check_char_array(key);
		::lua_setglobal(L, key);
	}

	template <typename Key>
	auto set_global(lua_State* L, Key const& key) -> std::enable_if_t<is_std_string_compatible_v<Key>>
	{
		::lua_setglobal(L, key.c_str());
	}

	template <typename Key>
	auto set_global(lua_State* L, Key const& key) -> std::enable_if_t<is_string_view_compatible_v<Key>>
	{
		detail::check_string_view(key);
		::lua_setglobal(L, key.data());
	}

	template <typename Key, typename Value>
	void set_global(lua_State* L, Key const& key, Value&& value)
	{
		stack_push(L, std::forward<Value>(value));
		set_global(L, key);
	}

	template <typename Key>
	auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_c_string_v<Key>>
	{
		::lua_setfield(L, index, key);
	}

	template <typename Key>
	auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_c_array_string_v<Key>>
	{
		detail::check_char_array(key);
		::lua_setfield(L, index, key);
	}

	template <typename Key>
	auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_std_string_compatible_v<Key>>
	{
		::lua_setfield(L, index, key.c_str());
	}

	template <typename Key>
	auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_string_view_compatible_v<Key>>
	{
		detail::check_string_view(key);
		::lua_setfield(L, index, key.data());
	}

	template <typename Key>
	auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_integral_v<Key>>
	{
		::lua_seti(L, index, static_cast<lua_Integer>(key));
	}

	template <typename Key, typename Value>
	auto set_table(lua_State* L, Key const& key, Value&& value, int index = -1)
		-> std::enable_if_t<meta_or_v<is_string<Key>, is_integral<Key>>>
	{
		stack_push(L, std::forward<Value>(value));
		set_field(L, key, index - 1);
	}

	template <typename Key, typename Value>
	auto set_table(lua_State* L, Key const& key, Value&& value, int index = -1)
		-> disable_if_t<meta_or_v<is_string<Key>, is_integral<Key>>>
	{
		stack_push(L, key);
		stack_push(L, std::forward<Value>(value));
		::lua_settable(L, index - 2);
	}
}
