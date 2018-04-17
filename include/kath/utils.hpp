#pragma once

namespace kath
{
	template<int rang>
	class stack_guard
	{
	public:
		stack_guard(context const& ctx)
			: ctx_(ctx)
		{}
		
		~stack_guard() noexcept
		{
			if constexpr(rang > 0)
			::lua_pop(ctx_.L, 0);
		}

	private:
		context const& ctx_;
	};
}