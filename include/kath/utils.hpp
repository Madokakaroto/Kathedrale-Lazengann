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

    template <typename ... Args>
    inline static auto tie(Args& ... args) noexcept -> tuple<Args&...>
    {
        return { args... };
    }

    // for tie
    template <typename ... Args>
    class tuple
    {
        using value_tuple = std::tuple<std::remove_reference_t<Args>...>;

    public:
        tuple(Args ... args)
            : t_(args...)
        {
        }

        template <typename InvokeExpr>
        void operator= (InvokeExpr const& expr)
        {
            t_ = static_cast<value_tuple>(expr);
        }

    private:
        std::tuple<Args...> t_; 
    };

    template <typename ... Args>
    struct type_list {};
}

#ifndef KATH_MAKE_TYPE_LIST
#define KATH_MAKE_TYPE_LIST(...)  kath::type_list<__VA_ARGS__>{}
#endif

#ifndef KATH_RESULT_T
#define KATH_RESULT_T KATH_MAKE_TYPE_LIST
#endif