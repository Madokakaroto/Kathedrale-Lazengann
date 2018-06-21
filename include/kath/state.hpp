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
        reference_t(lua_State* L, int ref)
            : L_(L)
            , ref_(ref)
        {}

		template <typename TableExpr>
		explicit reference_t(TableExpr const& expr)
			: L_(expr.get_state())
			, ref_(LUA_NOREF)
		{
            assert(L_);
            stack_guard gaurd{ L_ };
            expr.fetch();
            ref_ = ::luaL_ref(L, LUA_REGISTRYINDEX);
		}

        ~reference_t() noexcept
        {
            if(nullptr != L_ && ref_ > 0)
            {
                ::luaL_unref(L_, LUA_REGISTRYINDEX, ref_);
            }

            ref_ = LUA_NOREF;
        }

		auto get_state() const noexcept
		{
			return L_;
		}

        void fetch() const 
        {
            ::lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_);
        }

	private:
		lua_State*	    L_;
		int			    ref_;
	};

	class lua_value : base_expression<lua_value>
	{
	public:

        template <typename TabExpr>
		lua_value(TabExpr const& expr)
			: ref_(proxy)
		{}

        template <typename Value, typename = disable_if_t<std::is_same_v<Value, lua_value>>>
        operator Value() const
        {
            stack_guard gaurd{ ref_.get_state() };
            ref_.fetch();
            return kath::stack_get<Value>(L_);
        }

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