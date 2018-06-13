#pragma once

namespace kath
{
	template <typename Expr>
	inline static decltype(auto) extrac_expression(base_expression<Expr>& base_expr) noexcept
	{
		return static_cast<Expr&>(base_expr);
	}

	template <typename Expr>
	inline static decltype(auto) extrac_expression(base_expression<Expr> const& base_expr) noexcept
	{
		return static_cast<Expr const&>(base_expr);
	}
	
	template <typename Expr>
	class base_expression
	{
	public:
		static_assert(meta_and_v<negative<std::is_pointer<Expr>>, negative<std::is_reference<Expr>>>);
		using expression_type = Expr;
		using const_expression = std::add_const_t<Expr>;

		template <typename Key>
		auto access_field(lua_State* L, Key&& key) const noexcept
		{
			using forward_type = exract_key_type_t<Key>;
			using table_proxy_t = table_proxy<Expr, forward_type>;
			return table_proxy_t{ L, extrac_expression(*this), std::forward<forward_type>(key) };
		}

	protected:
		base_expression() = default;
	};

	template <typename Expr>
	class terminate_expression
	{
	public:
		static_assert(meta_and_v<negative<std::is_pointer<Expr>>, negative<std::is_reference<Expr>>>);
		using expression_type = Expr;
		using const_expression = std::add_const_t<Expr>;

		terminate_expression(terminate_expression const&) = delete;
		terminate_expression& operator=(terminate_expression const&) = delete;

	protected:
		terminate_expression() = default;
	};

	template <typename Expr, typename Key>
	class index_expression : public base_expression<index_expression<Expr, Key>>
	{
	public:
		using expression_type = Expr;
		using const_expression = std::add_const_t<Expr>;
		using key_type = Key;

		template <typename OtherExpr, typename Otherkey>
		friend class index_expression;

		index_expression(const_expression& expr, key_type key)
			: expr_(expr)
			, key_(std::move(key))
		{}

		~index_expression() = default;

		void fetch(lua_State* L) const
		{
			expr_.fetch_field(L, key_);
		}

		template <typename K>
		void fetch_field(lua_State* L, K const& key) const
		{
			fetch(L);
			kath::fetch_field(L, key);
		}

		template <typename K, typename Value>
		void set_field(lua_State* L, K const& key, Value&& value) const
		{
			fetch(L);
			kath::set_field(L, key, std::forward<Value>(value));
		}

		template <typename Value>
		void set_field(lua_State* L, Value&& value) const
		{
			expr_.set_field(L, key_, std::forward<Value>(value));
		}

        template <typename ... Rets, typename ... Args>
        decltype(auto) pcall(lua_State* L, Args&& ... args) const
        {
            using result_type = std::tuple<Rets...>;

            fetch(L);
            return lua_pcall::do_call<result_type>(L, std::forward_as_tuple(std::forward<Args>(args)...));
        }

	private:
		const_expression&	expr_;
		key_type			key_;
	};

    // lazy invoke
	template <typename Expr, typename Invoker, typename ... Args>
	class invoke_expression : public terminate_expression<invoke_expression<Expr, Invoker>>
	{
	public:
		using expression_type = Expr;
		using const_expression = std::add_const_t<Expr>;
		using tuple_type = std::tuple<Args...>;
	
		invoke_expression(lua_State* L, const_expression& expr, Args ... args)
			: L(L)
			, expr_(expr)
			, tuple_(std::forward_as_tuple(args...))
			, dismiss_(false)
		{
		}

		~invoke_expression()
		{
			if (!dismiss_)
			{
				call<void>();
			}
		}
	
		template <typename Ret, typename = disable_if_t<is_instance_of_v<Ret, tuple>>>
		operator Ret() const
		{
			return call<Ret>();
		}

	private:
		template <typename Ret>
		auto call() const
		{
			assert(!dismiss_);
			dismiss_ = true;

			expr_.fetch(L);
			return Invoker::template do_call<Ret>(L, tuple_);
		}
	
	private:
		lua_State*			L;
		const_expression&	expr_;
		tuple_type			tuple_;
		mutable bool		dismiss_;
	};

	template <typename Expr, typename Key>
	class table_proxy
	{
	public:
		static_assert(negative_v<meta_or<std::is_pointer<Expr>, std::is_reference<Expr>>>);
		static_assert(std::is_base_of_v<base_expression<Expr>, Expr>);

		using index_expression_t = index_expression<Expr, Key>;

		table_proxy(lua_State* L, Expr const& expr, Key key) noexcept
			: L(L)
			, expr_(expr, std::forward<Key>(key))
		{
		}

		template <typename Value>
		operator Value() const
		{
			expr_.fetch(L);
			return kath::stack_get<Value>(L);
		}

		template <typename Value>
		table_proxy& operator= (Value&& v)
		{
			expr_.set_field(L, std::forward<Value>(v));
			return *this;
		}

		template <typename Key>
		auto operator[](Key&& key) const noexcept
		{
			return expr_.access_field(L, std::forward<Key>(key));
		}

        // lazy invoke
		template <typename ... Args>
		auto operator() (Args&& ... args) const
			->  invoke_expression<index_expression_t, lua_pcall, Args...>
		{
			return { L, expr_, std::forward<Args>(args)... };
		}

        // immediate invoke
        template <typename ... Rets, typename ... Args>
        decltype(auto) operator()(type_list<Rets...>, Args&& ... args) const
        {
            return pcall<Rets...>(std::forward<Args>(args)...);
        }

        // immeidate invoke
        template <typename ... Rets, typename ... Args>
        decltype(auto) pcall(Args&& ... args) const
        {
            return expr_.pcall<Rets...>(L, std::forward<Args>(args)...);
        }

	private:
		lua_State *			L;
		index_expression_t	expr_;
	};
}