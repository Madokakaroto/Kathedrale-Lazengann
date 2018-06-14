#pragma once

// カテドラル ラゼンガン

namespace kath
{
	class state
	{
	public:
		explicit state()
			: L(::luaL_newstate())
		{
			//TODO ... for simple
			::luaL_openlibs(L);
		}

		~state()
		{
			if (L)
			{
				::lua_close(L);
				L = nullptr;
			}	
		}

		auto get_state() const noexcept
		{
			return L;
		}

	private:
		lua_State* L;
	};

	class global_table : base_expression<global_table>
	{
		friend class lua;
		using base_type = base_expression<global_table>;

	protected:
		global_table() = default;
		~global_table() = default;
		global_table(global_table const&) = delete;
		global_table& operator=(global_table const&) = delete;

	public:
		template <typename Key, typename = std::enable_if_t<is_string_v<std::remove_reference_t<Key>>>>
		auto access_field(lua_State* L, Key&& key) const noexcept
		{
			return base_type::access_field(L, std::forward<Key>(key));
		}

		template <typename Key>
		auto fetch_field(lua_State* L, Key const& key) const
			-> std::enable_if_t<is_string_v<Key>>
		{
			fetch_global(L, key);
		}
		
		template <typename Key, typename Value>
		void set_field(lua_State* L, Key&& key, Value&& value) const
		{
			set_global(L, key, std::forward<Value>(value));
		}
	};

	class reference_t
	{
	public:
		// make sure the ref object is at the top
		explicit reference_t(lua_State* L)
			: L(L)
			, ref_(::luaL_ref(L, LUA_REGISTRYINDEX))
		{}

		template <typename Proxy>
		explicit reference_t(Proxy const& proxy)
			: L(proxy.get_state())
			, ref_(LUA_REFNIL)
		{
			stack_guard guard{ L };

			proxy.fetch();
			ref_ = ::luaL_ref(L, LUA_REGISTRYINDEX);
		}

		auto get_state() const noexcept
		{
			return L;
		}

	private:
		lua_State*	L;
		int			ref_;
	};

	class lua_value : base_expression<lua_value>
	{
	public:
		template <typename Proxy>
		explicit lua_value(Proxy const& proxy)
			: ref_(proxy)
		{}



	private:
		reference_t		ref_;
	};

	class lua
	{
	public:
		lua() = default;
		~lua() = default;
		lua(lua const&) = delete;
		lua& operator= (lua const&) = delete;
		lua(lua&&) = default;
		lua& operator= (lua&&) = default;

		template <typename Key>
		auto operator[] (Key&& key) noexcept
		{
			return global_.access_field(s_.get_state(), std::forward<Key>(key));
		}

	private:
		state			s_;
		global_table	global_;
	};
}