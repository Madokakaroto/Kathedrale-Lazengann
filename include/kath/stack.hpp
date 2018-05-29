#pragma once

// stack push
namespace kath 
{
	namespace detail
	{
		template <typename T>
		inline static void stack_push_userdata(lua_State* L, T&& t)
		{
			using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
			auto ptr = lua_newuserdata(L, sizeof(value_type));
			new (ptr) value_type{ std::forward<T>(t) };
		}

		template <typename T, typename ... Args>
		inline static void stack_emplace_userdata(lua_State* L, Args&& ... args)
		{
			using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
			auto ptr = lua_newuserdata(L, sizeof(value_type)); 
			new (ptr) value_type{ std::forward<Args>(args)... };
		}

		template <typename T, typename = void>
		struct is_smart_pointer_with_reference_type : std::false_type {};

		template <typename T>
		struct is_smart_pointer_with_reference_type<T, std::enable_if_t<is_smart_pointer_v<T>>>
			: is_reference_type<typename T::element_type>
		{};

		// traits element type
		template <typename T>
		struct traits_element_type
		{
			using tmp_type = std::remove_reference_t<T>;
			using type = typename tmp_type::element_type;
		};

		template <typename T>
		using traits_element_type_t = typename traits_element_type<T>::type;
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
	inline static auto stack_push(lua_State* L, T const& value) 
		-> std::enable_if_t<meta_or_v<is_c_string<T>, is_char_array<T>>, char const*>
	{
		return ::lua_pushstring(L, value);
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T const& value) -> std::enable_if_t<is_string_buffer_v<T>, char const*>
	{
		return ::lua_pushlstring(L, value.data(), value.size());
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T&& t) -> std::enable_if_t<is_callable_v<std::remove_reference_t<T>>>
	{
		using lua_cfunctor_t = lua_cfunctor<T>;
		return lua_cfunctor_t::stack_push_callable(L, std::forward<T>(t));
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T&& t) -> std::enable_if_t<is_value_type_v<std::remove_reference_t<T>>>
	{
		detail::stack_push_userdata(L, std::forward<T>(t));
		::luaL_setmetatable(L, get_class_name<T>());
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T&& t) 
		-> std::enable_if_t<detail::is_smart_pointer_with_reference_type<std::remove_reference_t<T>>::value>
	{
		detail::stack_push_userdata(L, std::forward<T>(t));
		::luaL_setmetatable(L, get_class_name<detail::traits_element_type_t<T>>());
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T&& t) -> std::enable_if_t<is_reference_type_v<std::remove_reference_t<T>>>
	{
		auto ref_ptr = t.ref_from_this();
		stack_push(L, std::move(ref_ptr));
	}

}

// statck get
namespace kath 
{
	namespace detail
	{
		template <typename T>
		inline static auto stack_get_emplaced_userdata(lua_State* L, int index = -1)
		{
			// TODO ... check nullptr
			return reinterpret_cast<T*>(::lua_touserdata(L, index));
		}

		template <typename T>
		inline static auto stack_get_referenced_userdata(lua_State* L, int index = -1)
		{
			// TODO ... check nullptr
			auto ptr = reinterpret_cast<T**>(::lua_touserdata(L, index));
			return *ptr;
		} 
	}

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
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_lua_cfunction_v<T>, lua_CFunction>
	{
		return ::lua_tocfunction(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept 
		-> std::enable_if_t<meta_and_v<kath::is_callable<T>, negative<is_lua_cfunction<T>>>, T*>
	{
		return detail::stack_get_emplaced_userdata<T>(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_value_type_v<T>, T*>
	{
		return detail::stack_get_emplaced_userdata<T>(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_reference_type_v<T>, T*>
	{
		return detail::stack_get_referenced_userdata<T>(L, index);
	}

}

// stack check get
namespace kath 
{
	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) noexcept -> std::enable_if_t<is_bool_v<T>, T>
	{
		return stack_get<T>(L, arg);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<is_integral_v<T>, T>
	{
		return static_cast<T>(::luaL_checkinteger(L, arg));
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<is_floating_point_v<T>, T>
	{
		return static_cast<T>(::luaL_checknumber(L, arg));
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::luaL_checklstring(L, arg, nullptr);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<is_string_buffer_v<T>, T>
	{
		size_t len { 0 };
		auto ptr = ::luaL_checklstring(L, arg, &len);
		return { ptr, len };
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<is_userdata_type_v<T>, T*>
	{
		// TODO ... check class type
		//::luaL_checkudata(L, arg, "dummy_class");
		return stack_get<T>(L, arg);
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

	inline static void stack_duplicate(lua_State* L, int index = -1)
	{
		::lua_pushvalue(L, index);
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
		return basic_type{ ::lua_getglobal(L, key) };
	}

	template <typename Key>
	inline static auto fetch_global(lua_State* L, Key const& key) -> std::enable_if_t<is_char_array_v<Key>, basic_type>
	{
		return basic_type{ ::lua_getglobal(L, key) };
	}

	template <typename Key>
	inline static auto fetch_global(lua_State* L, Key const& key) 
		-> std::enable_if_t<is_std_string_v<Key>, basic_type>
	{
		return basic_type{ ::lua_getglobal(L, key.c_str()) };
	}

	template <typename Key>
	inline static auto fetch_global(lua_State* L, Key const& key) 
		-> std::enable_if_t<is_string_view_v<Key>, basic_type>
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
		-> std::enable_if_t<is_char_array_v<Key>, basic_type>
	{
		return basic_type{ ::lua_getfield(L, index, key) };
	}

	template <typename Key>
	inline static auto fetch_field(lua_State* L, Key const& key, int index = -1)
		-> std::enable_if_t<is_std_string_v<Key>, basic_type>
	{
		return basic_type{ ::lua_getfield(L, index, key.c_str()) };
	}

	template <typename Key>
	inline static auto fetch_field(lua_State* L, Key const& key, int index = -1) 
		-> std::enable_if_t<meta_or_v<is_string_view<Key>, is_floating_point<Key>, is_bool<Key>>, basic_type>
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
	inline static auto set_global(lua_State* L, Key const& key) -> std::enable_if_t<is_c_string_v<Key>>
	{
		::lua_setglobal(L, key);
	}

	template <typename Key>
	inline static auto set_global(lua_State* L, Key const& key) -> std::enable_if_t<is_char_array_v<Key>>
	{
		::lua_setglobal(L, key);
	}

	template <typename Key>
	inline static auto set_global(lua_State* L, Key const& key) -> std::enable_if_t<is_std_string_v<Key>>
	{
		::lua_setglobal(L, key.c_str());
	}

	template <typename Key>
	inline static auto set_global(lua_State* L, Key const& key) -> std::enable_if_t<is_string_view_v<Key>>
	{
		detail::check_string_view(key);
		::lua_setglobal(L, key.data());
	}

	template <typename Key, typename Value>
	inline static void set_global(lua_State* L, Key const& key, Value&& value)
	{
		stack_push(L, std::forward<Value>(value));
		set_global(L, key);
	}

	template <typename Key>
	inline static auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_c_string_v<Key>>
	{
		::lua_setfield(L, index, key);
	}

	template <typename Key>
	inline static auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_char_array_v<Key>>
	{
		::lua_setfield(L, index, key);
	}

	template <typename Key>
	inline static auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_std_string_v<Key>>
	{
		::lua_setfield(L, index, key.c_str());
	}

	template <typename Key>
	inline static auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_string_view_v<Key>>
	{
		detail::check_string_view(key);
		::lua_setfield(L, index, key.data());
	}

	template <typename Key>
	inline static auto set_field(lua_State* L, Key const& key, int index = -2) -> std::enable_if_t<is_integral_v<Key>>
	{
		::lua_seti(L, index, static_cast<lua_Integer>(key));
	}

	template <typename Key, typename Value>
	inline static auto set_table(lua_State* L, Key const& key, Value&& value, int index = -1)
		-> std::enable_if_t<meta_or_v<is_string<Key>, is_integral<Key>>>
	{
		stack_push(L, std::forward<Value>(value));
		set_field(L, key, index - 1);
	}

	template <typename Key, typename Value>
	inline static auto set_table(lua_State* L, Key const& key, Value&& value, int index = -1)
		-> disable_if_t<meta_or_v<is_string<Key>, is_integral<Key>>>
	{
		stack_push(L, key);
		stack_push(L, std::forward<Value>(value));
		::lua_settable(L, index - 2);
	}

	inline static void set_table(lua_State* L, int index = -3)
	{
		::lua_settable(L, index);
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