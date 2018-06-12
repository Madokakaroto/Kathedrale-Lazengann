#pragma once

#include <lua.hpp>
#include <type_traits>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>
#include <map>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <memory>
#include <atomic>

namespace kath
{
	template <typename T, typename RefCounter>
	class ref_count_ptr;

	template <typename T, typename RefCounter>
	class weak_ptr; 

	template <typename ... Rets>
	struct lua_callable;

	template <typename Func>
	struct lua_cfunctor;

	template <typename Expr>
	class base_expression;

	template <typename Expr, typename Key>
	class index_expression;

	template <typename Expr, typename Invoker, typename ... Args>
	class invoke_expression;

	template <typename Expr, typename Key>
	class table_proxy;

	class state;

	namespace ext
	{
		template <typename T, typename = void>
		struct manipulate_type;
	}
}