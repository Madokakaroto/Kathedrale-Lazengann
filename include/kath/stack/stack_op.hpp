#pragma once

// stack op
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
	    struct is_ref_value_ptr : std::false_type {};

	    template <typename T>
	    struct is_ref_value_ptr<T, std::enable_if_t<is_smart_pointer_v<T>>>
	    	: is_reference_type<typename T::element_type>
	    {};

        template <typename T>
        inline constexpr bool is_ref_value_ptr_v = is_ref_value_ptr<T>::value;

		template <typename T>
		struct traits_element_type
		{
			using tmp_type = std::remove_reference_t<T>;
			using type = typename tmp_type::element_type;
		};

		template <typename T>
		using traits_element_type_t = typename traits_element_type<T>::type;

		template <typename T>
		using is_pointer_exclude_type = meta_and<
			is_primitive_type<T>,
			negative<is_c_string<T>>
		>;

		template <typename T>
		inline constexpr bool is_pointer_exclude_type_v = is_pointer_exclude_type<T>::value;
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
		-> std::enable_if_t<detail::is_ref_value_ptr_v<std::remove_reference_t<T>>>
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

	template <typename T>
	inline static auto stack_push(lua_State* L, T&& t) -> std::enable_if_t<is_manipulated_type_v<remove_rcv_t<T>>>
	{
		using manipulate_type_t = ext::manipulate_type<remove_rcv_t<T>>;
		manipulate_type_t::stack_push(std::forward<T>(t));
	}

	template <typename T>
	inline static auto stack_push(lua_State* L, T* ptr) -> disable_if_t<detail::is_pointer_exclude_type_v<T>>
	{
		// pointer has no move semantic
		stack_push(L, *ptr);
	}
}

// stack get
namespace kath 
{
	namespace detail
	{
		template <typename T>
		inline static auto stack_get_emplaced_userdata(lua_State* L, int index = -1)
		{
			auto ptr = ::lua_touserdata(L, index);
			assert(ptr);
			return reinterpret_cast<T*>(ptr);
		}

		template <typename T>
		inline static auto stack_get_referenced_userdata(lua_State* L, int index = -1)
		{
			auto ptr = reinterpret_cast<T**>(::lua_touserdata(L, index));
			assert(ptr && *ptr);
			return *ptr;
		}

		template <typename T>
		using is_non_luac_callable = meta_and<is_callable<T>, negative<is_lua_cfunction<T>>>;

		template <typename T>
		inline constexpr bool is_non_luac_callable_v = is_non_luac_callable<T>::value;
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) noexcept -> std::enable_if_t<is_bool_v<T>, T>
	{
		return static_cast<T>(::lua_toboolean(L, index));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) -> std::enable_if_t<is_integral_v<T>, T>
	{
		return static_cast<T>(::lua_tointegerx(L, index, nullptr));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) -> std::enable_if_t<is_floating_point_v<T>, T>
	{
		return static_cast<T>(::lua_tonumberx(L, index, nullptr));
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) -> std::enable_if_t<is_c_string_v<T>, char const*>
	{
		return ::lua_tolstring(L, index, nullptr);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) -> std::enable_if_t<is_string_buffer_v<T>, T>
	{
		size_t len { 0 };
		auto ptr = ::lua_tolstring(L, index, &len);
		return { ptr, len };
	}
	
	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) -> std::enable_if_t<is_lua_cfunction_v<T>, lua_CFunction>
	{
		return ::lua_tocfunction(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) -> std::enable_if_t<detail::is_non_luac_callable_v<T>, T&>
	{
		return *detail::stack_get_emplaced_userdata<T>(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) 
		-> std::enable_if_t<meta_or_v<is_value_type<T>, detail::is_ref_value_ptr<T>>, T&>
	{
		return *detail::stack_get_emplaced_userdata<T>(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) -> std::enable_if_t<is_reference_type_v<T>, T&>
	{
		return *detail::stack_get_referenced_userdata<T>(L, index);
	}

	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) -> std::enable_if_t<is_manipulated_type_v<T>, T>
	{
		return ext::manipulate_type<T>::stack_get(L, index); 
	}

	// get pointer for value type userdata
	template <typename T>
	inline static auto stack_get(lua_State* L, int index = -1) 
		-> std::enable_if_t<meta_and_v<std::is_pointer<T>, 
			meta_or<is_value_type<std::remove_pointer_t<T>>, detail::is_ref_value_ptr<std::remove_pointer_t<T>>>>, T>
	{
		return detail::stack_get_emplaced_userdata<std::remove_pointer_t<T>>(L, index);
	}

	// get pointer for reference type userdata
	template <typename T>
	inline static auto stack_get(lua_State* L, int index = - 1)
		-> std::enable_if_t<meta_and_v<std::is_pointer<T>, is_reference_type<std::remove_pointer_t<T>>>, T>
	{
		return detail::stack_get_referenced_userdata<std::remove_pointer_t<T>>(L, index);
	}

	template <typename T>
	inline static decltype(auto) stack_get(lua_State* L, std::enable_if_t<std::is_reference_v<T>, int> index = -1)
	{
		return stack_get<std::remove_reference_t<T>>(L, index);
	} 
}

// stack check
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
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<is_userdata_type_v<T>, T&>
	{
		::luaL_checkudata(L, arg, get_class_name<T>());
		return stack_get<T>(L, arg);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<detail::is_ref_value_ptr_v<T>, T&>
	{
		using type = typename T::element_type;
		::luaL_checkudata(L, arg, get_class_name<type>());
		return stack_get<T>(L, arg);
	}

	// stack check for reference
	template <typename T>
	inline static decltype(auto) stack_check(lua_State* L, std::enable_if_t<std::is_reference_v<T>, int> arg = 1)
	{
		return stack_check<std::remove_reference_t<T>>(L, arg);
	}

	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<is_manipulated_type_v<T>, T>
	{
		return ext::manipulate_type<T>::stack_check(L, arg); 
	}

	// stack check for pointer of userdata
	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<
		meta_and_v<std::is_pointer<T>, is_userdata_type<std::remove_pointer_t<T>>>, T>
	{
		using type = std::remove_pointer_t<T>;
		::luaL_checkudata(L, arg, get_class_name<type>());
		return stack_get<T>(L, arg);
	}

	// stack check for pointer of share_ptr
	template <typename T>
	inline static auto stack_check(lua_State* L, int arg = 1) -> std::enable_if_t<
		meta_and_v<std::is_pointer<T>, detail::is_ref_value_ptr<std::remove_pointer_t<T>>>, T>
	{
		using type = typename std::remove_pointer_t<T>::element_type;
		::luaL_checkudata(L, arg, get_class_name<type>());
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

    namespace detail
    {
        template <size_t Index, typename Tuple, typename 
            Type =  decltype(std::get<Index>(std::declval<Tuple>()))>
        inline static bool stack_forward_push(lua_State* L, Tuple&& t)
        {
            stack_push(L, std::forward<Type>(std::get<Index>(std::declval<Tuple>())));
            return true;
        }

        template <typename T, size_t ... Is>
        inline static int stack_push_result_impl(lua_State* L, T&& t, std::index_sequence<Is...>)
        {
            swallow_t{ stack_forward_push<Is>(L, std::forward<T>(t))... };
            return sizeof...(Is);
        }
    }

    template <typename T, typename Type = std::remove_reference_t<T>>
    inline static auto stack_push_result(lua_State* L, T&& t) -> std::enable_if_t<is_valid_tuple_v<Type>, int>
    {
        return detail::stack_push_result_impl(L, std::forward<T>(t), std::make_index_sequence<std::tuple_size<Type>>{});
    }
    
    template <typename T, typename Type = std::remove_reference_t<T>>
    inline static auto stack_push_result(lua_State* L, T&& t) -> disable_if_t<is_valid_tuple_v<Type>, int>
    {
        stack_push(L, std::forward<T>(t));
        return 1;
    }
}
