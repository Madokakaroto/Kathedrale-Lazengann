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
        static_assert(meta_and_v<negation<std::is_pointer<Expr>>, negation<std::is_reference<Expr>>>);
        using expression_type = Expr;
        using const_expression = std::add_const_t<Expr>;

        template <typename Key>
        auto access_field(lua_State* L, Key&& key) const noexcept
        {
            using forward_key_type = exract_key_type_t<Key>;
            using table_expression_t = table_expression<Expr, forward_key_type>;
            return table_expression_t{ L, extrac_expression(*this), std::forward<forward_key_type>(key) };
        }

    protected:
        base_expression() = default;
    };

    template <typename Expr>
    class terminate_expression
    {
    public:
        static_assert(meta_and_v<negation<std::is_pointer<Expr>>, negation<std::is_reference<Expr>>>);
        using expression_type = Expr;
        using const_expression = std::add_const_t<Expr>;

        terminate_expression(terminate_expression const&) = delete;
        terminate_expression& operator=(terminate_expression const&) = delete;

    protected:
        terminate_expression() = default;
    };

    // lazy invoke
    template <typename Expr, typename Invoker, typename ... Args>
    class invoke_expression : public terminate_expression<invoke_expression<Expr, Invoker>>
    {
    public:
        using expression_type = Expr;
        using const_expression = std::add_const_t<Expr>;
        using tuple_type = std::tuple<Args...>;
    
        template <typename OtherExpr, typename Key>
        class table_expression;

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
    class table_expression : public base_expression<table_expression<Expr, Key>>
    {
    public:
        using expression_type = Expr;
        using const_expression = std::add_const_t<Expr>;
        using key_type = Key;

        template <typename OtherExpr, typename OtherKey>
        friend class table_expression;

        template <typename OtherExpr, typename Invoker, typename ... Args>
        friend class invoke_expression;

        friend class lua_value;

    public:
        table_expression(lua_State* L, const_expression& expr, key_type key)
            : L_(L)
            , expr_(expr)
            , key_(key)
        {}

        template <typename Value>
        table_expression& operator= (Value&& v)
        {
            expr_.set_field(L_, key_, std::forward<Value>(v));
            return *this;
        }

        template <typename Value, typename = disable_if_t<std::is_same_v<Value, lua_value>>>
        operator Value() const
        {
            fetch(L_);
            return kath::stack_get<Value>(L_);
        }

        template <typename Key>
        auto operator[](Key&& key) const noexcept
        {
            return expr_.access_field(L_, std::forward<Key>(key));
        }

        // lazy invoke
        template <typename ... Args>
        auto operator() (Args&& ... args) const noexcept
            -> invoke_expression<table_expression, lua_pcall, Args...>
        {
            return { L_, *this, std::forward<Args>(args)... };
        }

        // immediate invoke
        template <typename ... Rets, typename ... Args>
        decltype(auto) operator()(type_list<Rets...>, Args&& ... args) const
        {
            return pcall<Rets...>(std::forward<Args>(args)...);
        }

        template <typename ... Rets, typename ... Args>
        decltype(auto) pcall(Args&& ... args) const
        {
            using result_type = std::tuple<Rets...>;
        
            fetch(L_);
            return lua_pcall::do_call<result_type>(L_, std::forward_as_tuple(std::forward<Args>(args)...));
        }

    private:
        void fetch(lua_State* L) const
        {
            expr_.fetch_field(L, key_);
        }

        lua_State* get_state() const noexcept
        {
            return L_;
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

    private:
        lua_State*          L_;
        const_expression&	expr_;
        key_type			key_;
    };
}