#pragma once

#include "stack/stack_op.hpp"
#include "stack/stack_op_ext.hpp"


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