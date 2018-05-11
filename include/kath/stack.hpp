#pragma once

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

	// function
	inline static void stack_push(lua_State* L, lua_CFunction f, int n = 0)
	{
		::lua_pushcclosure(L, f, 0);
	}

	//light userdata
	template <typename T>
	inline static auto stack_push(lua_State* L, T* uobj) -> disable_if<is_c_string_v<T>>
	{
		::lua_pushlightuserdata(L, reinterpret_cast<void*>(uobj));
	}
}

// statck get
namespace kath 
{
	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_bool_v<T>, T>
	{
		return static_cast<T>(::lua_toboolean(L, index));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_integral_v<T>, T>
	{
		return static_cast<T>(::lua_tointegerx(L, index, nullptr));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_floating_point_v<T>, T>
	{
		return static_cast<T>(::lua_tonumberx(L, index, nullptr));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::lua_tolstring(L, index, nullptr);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_string_buffer_v<T>, T>
	{
		size_t len { 0 };
		auto ptr = ::lua_tolstring(L, index, &len);
		return { ptr, len };
	}
	
	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<std::is_same_v<T, lua_CFunction>, lua_CFunction>
	{
		return ::lua_tocfunction(L, index);
	}
}

// stack check get
namespace kath 
{
	template <typename T>
	inline static auto stack_check(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_bool_v<T>, T>
	{
		return stack_get<T>(L, index);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int index = -1) -> std::enable_if_t<is_integral_v<T>, T>
	{
		return static_cast<T>(::luaL_checkinteger(L, index));
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int index = -1) -> std::enable_if_t<is_floating_point_v<T>, T>
	{
		return static_cast<T>(::luaL_checknumber(L, index));
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::luaL_checklstring(L, index, nullptr);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_string_buffer_v<T>, T>
	{
		size_t len { 0 };
		auto ptr = ::luaL_checklstring(L, index, &len);
		return { ptr, len };
	}
}

// other stack operation
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

	template <typename Key>
	inline static auto fetch_global(lua_State* L, Key const& key) -> std::enable_if_t<is_c_string_v<Key>, basic_type>
	{
		return basic_type{ ::lua_getglobal(L, value) };
	}

	template <typename Key>
	inline static auto fetch_global(lua_State* L, Key const& key) -> std::enable_if_t<is_c_array_string_v<Key>, basic_type>
	{
		detail::check_char_array(key);
		return basic_type{ ::lua_getglobal(L, key) };
	}

	template <typename Key>
	inline static auto fetch_global(lua_State* L, Key const& key) 
		-> std::enable_if_t<is_std_string_compatible_v<Key>, basic_type>
	{
		return basic_type{ ::lua_getglobal(L, key.c_str()) };
	}

	template <typename Key>
	inline static auto fetch_global(lua_State* L, Key const& key) 
		-> std::enable_if_t<is_string_view_compatible_v<Key>, basic_type>
	{
		detail::check_string_view(key);
		return basic_type{ ::lua_getglobal(L, key.data()) };
	}

	template <typename Key>
	inline static auto fetch_field(lua_State* L, Key const& key, int index = -1) 
		-> std::enable_if_t<is_c_string_v<Key>, basic_type>
	{
		return basic_type{ ::lua_getfield(L, index, key) };
	}

	template <typename Key>
	inline static auto fetch_field(lua_State* L, Key const& key, int index = -1) 
		-> std::enable_if_t<is_c_array_string_v<Key>, basic_type>
	{
		detail::check_char_array(key);
		return basic_type{ ::lua_getfield(L, index, key) };
	}

	template <typename Key>
	inline static auto fetch_field(lua_State* L, Key const& key, int index = -1)
		-> std::enable_if_t<is_std_string_compatible_v<Key>, basic_type>
	{
		return basic_type{ ::lua_getfield(L, index, key.c_str()) };
	}

	template <typename Key>
	inline static auto fetch_field(lua_State* L, Key const& key, int index = -1) 
		-> std::enable_if_t<meta_or_v<is_string_view_compatible<Key>, is_floating_point<Key>, is_bool<Key>>, basic_type>
	{
		stack_push(L, key);
		return basic_type{ ::lua_gettable(L, index - 1) };
	}

	template <typename Key>
	inline static auto fetch_field(lua_State* L, Key const& key, int index = - 1)
		-> std::enable_if_t<is_integral_v<Key>, basic_type>
	{
		return basic_type{ ::lua_geti(L, index, static_cast<lua_Integer>(key)) };
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

// higher level
namespace kath
{
	struct gloabl_table_op : protected stack_guard
	{
		gloabl_table_op(lua_State* L)
			: stack_guard(L)
		{}

		template <typename Key>
		void fetch(Key const& key)
		{
			assert(this->L_);
			fetch_global(this->L_, key);
		}

		template <typename Key, typename Value>
		auto set(Key const& key, Value&& value) -> disable_if_t<is_callable_v<Value>>
		{
			assert(this->L_);
			set_global(this->L_, key);
		}

	};

	//struct 
}