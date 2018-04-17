#pragma once

namespace kath
{
	template <typename Expr>
	inline decltype(auto) extrac_expression(base_expression<Expr>& base_expr) noexcept
	{
		return static_cast<Expr&>(base_expr);
	}

	template <typename Expr>
	inline decltype(auto) extrac_expression(base_expression<Expr> const& base_expr) noexcept
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

		template <typename Key, typename = std::enable_if_t<is_key_type_v<Key>>>
		auto operator[] (Key&& key) const noexcept
		{
			// if the type of AnotherKey is std string compatible 
			using expression_t = table_expression<const_expression, exract_key_type_t<Key>>;
			return expression_t{ extrac_expression(*this), std::forward<Key>(key) };
		}

	protected:
		base_expression() noexcept = default;
	};


	// Key type has no interest on Key type calculation
	template <typename Expr, typename Key>
	class table_expression : public base_expression<table_expression<Expr, Key>>
	{
	public:
		using base_type = base_expression<table_expression<Expr, Key>>;
		using expression_type = Expr;
		using const_expression = std::add_const_t<Expr>;
		using key_type = Key;

		template <typename OtherExpr, typename Otherkey>
		friend class table_expression;

		static inline constexpr int rang = Expr::rang + 1;

		table_expression(const_expression const& expr, key_type key) noexcept
			: base_type()
			, expr_(expr)
			, key_(std::move(key))
		{
		}

		// perform the "table[key_] = value" semantic
		template <typename Value>
		void operator= (Value&& value) const
		{
			stack_guard<rang - 1> guard{ set(std::forward<Value>(value)) };
		}

		// perform the value = "table[key_]" semantic
		template <typename Value>
		operator Value() const
		{
			// push on stack
			auto ctx = get();

			stack_guard<rang> guard{ ctx };

			// cast to value type
			return stack_cast<Value>(ctx);
		}

		using base_type::operator[];

	private:
		template <typename OtherKey>
		auto get(OtherKey const& key) const
		{
			// push base expression on stack recursively
			auto ctx = get();

			// push the current expression
			fetch_field(ctx, key);

			// return context for dfs
			return ctx;
		}

		auto get() const
		{
			return expr_.get(key_);
		}

		template <typename OtherKey, typename Value>
		auto set(OtherKey const& key, Value&& value) const
		{
			auto ctx = get();
			set_field(ctx, key, std::forward<Value>(value));
			return ctx;
		}

		template <typename Value>
		auto set(Value&& value) const
		{
			return expr_.set(key_, std::forward<Value>(value));
		}

	private:
		const_expression const&	expr_;
		key_type				key_;
	};
}