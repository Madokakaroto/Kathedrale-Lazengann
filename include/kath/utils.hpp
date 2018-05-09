#pragma once

namespace kath
{
	template<int rang>
	class stack_guard
	{
	public:
		stack_guard(lua_State* L)
			: L(L)
		{
		}
		
		~stack_guard() noexcept
		{
			if constexpr(rang > 0)
				::lua_pop(L, rang);
		}

	private:
		lua_State* L;
	};
}