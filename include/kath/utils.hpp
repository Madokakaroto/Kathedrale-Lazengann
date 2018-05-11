#pragma once

namespace kath
{
	struct stack_guard
	{
		stack_guard(lua_State* L)
			: L_(L)
			, top_(::lua_gettop(L))
		{}

		~stack_guard()
		{
			::lua_settop(L_, top_);
		}

		stack_guard(stack_guard const&) = delete;
		stack_guard(stack_guard&&) = delete;

		stack_guard& operator=(stack_guard const&) = delete;
		stack_guard& operator=(stack_guard&&) = delete;
	
		lua_State*	L_;
		int			top_;
	};
}