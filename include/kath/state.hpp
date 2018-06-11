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
	protected:
		using base_type = base_expression<global_table>;

		global_table() = default;
		~global_table()
		{
			auto a = 1;
		}
		global_table(global_table const&) = delete;
		global_table& operator=(global_table const&) = delete;

	public:
		template <typename Key, typename = std::enable_if_t<is_string_v<std::remove_reference_t<Key>>>>
		static auto access_field(lua_State* L, Key&& key) noexcept
		{
			using index_expression_t = index_expression<global_table, exract_key_type_t<Key>>;
			using table_proxy_t = table_proxy<index_expression_t>;
			return table_proxy_t{ L, index_expression_t{ global_table{}, std::forward<Key>(key) } };
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
			//return this->access_field(s_.get_state(), std::forward<Key>(key));
			return global_table::access_field(s_.get_state(), std::forward<Key>(key));
		}

	private:
		state			s_;
	};
}