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
			using index_expression_t = index_expression<Expr, exract_key_type_t<Key>>;
			using table_proxy_t = table_proxy<index_expression_t>;
			return table_proxy_t{ L, index_expression_t{ extrac_expression(*this), std::forward<Key>(key) } };
		}

	protected:
		base_expression() = default;
	};

	template <typename Expr>
	class table_proxy
	{
	public:
		static_assert(negative_v<meta_or<std::is_pointer<Expr>, std::is_reference<Expr>>>);
		static_assert(std::is_base_of_v<base_expression<Expr>, Expr>);

		table_proxy(lua_State* L, Expr const& expr) noexcept
			: L(L)
			, expr_(expr)
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

	private:
		lua_State*		L;
		Expr const&		expr_;
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

		~index_expression()
		{

		}

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

	private:
		const_expression&	expr_;
		key_type			key_;
	};
}