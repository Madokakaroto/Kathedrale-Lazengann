#pragma once

// カテドラル ラゼンガン

namespace kath
{
	class state : public base_expression<state>
	{
	public:
		using base_type = base_expression<state>;

		template <typename Expr, typename Key>
		friend class table_expression;

		inline static constexpr int rang = 0;

	public:
		state()
			: L(::luaL_newstate())
		{
			::luaL_openlibs(L);
		}

		~state() noexcept
		{
			destroy();
		}

		state(state const&) = delete;
		state(state&& other) noexcept
			: L(nullptr)
		{
			std::swap(L, other.L);
		}

		state& operator= (state const&) = delete;
		state& operator= (state&& other) noexcept
		{
			state temp{ std::move(*this) };
			std::swap(L, other.L);
		}

		using base_type::operator[];

	private:
		void destroy() noexcept
		{
			if(nullptr != L)
			{
				::lua_close(L);
				L = nullptr;
			}
		}

		auto get_state() const noexcept
		{
			return L;
		}

		template <typename Key>
		auto get(Key const& key) const -> context
		{
			context ctx = { L };
			fetch_global(ctx, key);
			return ctx;
		}

		template <typename Key, typename Value>
		auto set(Key const& key, Value&& value) const
		{
			context ctx = { L };
			set_global(ctx, key, std::forward<Value>(value));
			return ctx;
		}

	private:
		::lua_State*	L;
	};
}