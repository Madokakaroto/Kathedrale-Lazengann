#pragma once

namespace kath
{
	enum class data_type_t : int32_t
	{
		nil = LUA_TNIL,
		boolean = LUA_TBOOLEAN,
		lightuserdata = LUA_TLIGHTUSERDATA,
		number = LUA_TNUMBER,
		string = LUA_TSTRING,
		table = LUA_TTABLE,
		function = LUA_TFUNCTION,
		userdata = LUA_TUSERDATA,
		thread = LUA_TTHREAD,
	};

	inline constexpr struct nil_t {} nil;

	template <typename Expr>
	class base_expression;

	template <typename Expr, typename Key>
	class table_expression;

	class state;
}