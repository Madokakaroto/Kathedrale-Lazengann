#pragma once

namespace kath
{
    template <typename T, typename RefCounter>
    class ref_count_ptr;

    template <typename T, typename RefCounter>
    class weak_ptr;

    template <typename T, typename RefCounter>
    class enable_ref_from_this;

    namespace detail
    {
        template <typename RefCounter>
        class ref_counter_wrapper : public RefCounter
        {
        public:
            ref_counter_wrapper() = default;
            virtual ~ref_counter_wrapper() = default;

            virtual void destroy_instance() = 0;
            virtual void destroy_counter() = 0;

            void notify_decref()
            {
                if(this->decref() == 0)
                {
                    destroy_instance();
                    notify_decwref();
                }
            }

            void notify_decwref()
            {
                if(this->decwref() == 0)
                {
                    destroy_counter();
                }
            }
        };

        template <typename T, typename RefCounter>
        class ref_count_obj : public ref_counter_wrapper<RefCounter>
        {
            static_assert(!std::is_void_v<T>, "");
            using element_type = T;

        public:
            template <typename ... Args>
            ref_count_obj(Args&& ... args)
            {
                // it is better to use () than {} here
                auto ptr = reinterpret_cast<void*>(&storatge_);
                new (ptr) element_type (std::forward<Args>(args)...);
            }

            void destroy_instance() override
            {
                if constexpr(!std::is_trivially_destructible_v<T>)
                {
                    auto ptr = reinterpret_cast<T*>(&storatge_);
                    ptr->~T();
                }
            }

            void destroy_counter() override
            {
                delete this;
            }

            auto get_pointer() const noexcept
            {
                return reinterpret_cast<T*>(&storatge_);
            }

        private:
            std::aligned_union_t<1, T>  storatge_;
        };

        template <typename T, typename RefCounter>
        class ref_count : public ref_counter_wrapper<RefCounter>
        {
        public:
            template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
            explicit ref_count(_T* ptr)
                : ptr_(ptr)
            {
            }

            void destroy_instance() override
            {
                if(ptr_)
                {
                    delete ptr_;
                    ptr_ = nullptr;
                }
            }

            void destroy_counter() override
            {
                delete this;
            }

        private:
            T* ptr_;
        };

        template <typename T, typename RefCounter>
        class ref_count_ptr_base
        {
        protected:
            using element_type = std::remove_all_extents_t<T>;
            using ref_counter = RefCounter;
            using ref_count_interface = ref_counter_wrapper<RefCounter>;
            template <typename _T, typename _RefCounter> friend class ref_count_ptr_base;
            template <typename _T, typename _RefCounter> friend class ref_count_ptr;
            template <typename _T, typename _RefCounter> friend class weak_ptr;
            template <typename _T> using base_ptr_alias = ref_count_ptr_base<_T, ref_counter>;
            template <typename _T> using ref_count_ptr_alias = ref_count_ptr<_T, ref_counter>;
            template <typename _T> using weak_ptr_alias = weak_ptr<_T, ref_counter>;

            ref_count_ptr_base() = default;
            ~ref_count_ptr_base() = default;
            ref_count_ptr_base(ref_count_ptr_base const&) = delete;
            ref_count_ptr_base& operator=(ref_count_ptr_base const&) = delete;

            element_type*           ptr_        = nullptr;
            ref_count_interface*    counter_    = nullptr;

            void KATH_set_impl(element_type* ptr, ref_count_interface* counter)
            {
                ptr_ = ptr;
                counter_ = counter;
            }

            template <typename _T>
            void KATH_copy_construct_from(ref_count_ptr_alias<_T> const& other)
            {
                if(other.counter_)
                {
                    other.counter_->incref();
                }

                ptr_ = other.ptr_;
                counter_ = other.counter_;
            }

            template <typename _T>
            void KATH_move_construct_from(base_ptr_alias<_T>&& other)
            {
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;

                std::swap(counter_, other.counter_);
            }

            template <typename _T>
            void KATH_weak_construct_from(base_ptr_alias<_T> const& other)
            {
                if(other.counter_)
                {
                    other.counter_->incwref();
                }

                ptr_ = other.ptr_;
                counter_ = other.counter_;
            }

            template <typename _T>
            bool KATH_construct_from_weak(weak_ptr_alias<_T> const& other)
            {
                if(other.counter_ && other.counter_->incref_nz())
                {
                    ptr_ = other.ptr_;
                    counter_ = other.counter_;
                    return true;
                }

                return false;
            }

            void KATH_decref()
            {
                if(counter_)
                    counter_->notify_decref();
            }

            void KATH_decwref()
            {
                if(counter_)
                    counter_->notify_decwref();
            }

            void KATH_swap(ref_count_ptr_base& other)
            {
                std::swap(ptr_, other.ptr_);
                std::swap(counter_, other.counter_);
            }

            element_type* get() const noexcept
            {
                return ptr_;
            }

            auto use_count() const noexcept
            {
                using value_type = typename RefCounter::value_type;
                return counter_ ? counter_->use_count() : value_type{ 0 };
            }
        };

        template <typename T, typename = void>
        struct can_be_refered : std::false_type {};

        template <typename T>
        struct can_be_refered<T, std::void_t<typename T::KATH_ref_enabled>> : std::true_type {};

        template <typename T>
        inline constexpr bool can_be_refered_v = can_be_refered<T>::value;
    }
    
    class fast_refcount
    {
    public:
        using value_type = uint32_t;

        fast_refcount() = default;

        void incref() noexcept
        {
            users_++;
        }

        void incwref() noexcept
        {
            weaks_++;
        }

        auto decref() noexcept
        {
            return --users_;
        }

        auto decwref() noexcept
        {
            return --weaks_;
        }

        auto use_count() noexcept
        {
            return users_;
        }

        auto weak_count() noexcept
        {
            return weaks_;
        }

        bool incref_nz() noexcept
        {
            if(0 == users_)
                return false;
            users_++;
            return true;
        }

    private:
        uint32_t users_ = 1;
        uint32_t weaks_ = 1;
    };

    class shared_refcount
    {
    public:
        using value_type = uint32_t;

        void incref() noexcept
        {
            users_.fetch_add(1);
        }

        void incwref() noexcept
        {
            weaks_.fetch_add(1);
        }

        auto decref() noexcept
        {
            return users_.fetch_sub(1);
        }

        auto decwref() noexcept
        {
            return weaks_.fetch_sub(1);
        }

        auto use_count() noexcept
        {
            return users_.load();
        }

        auto weak_count() noexcept
        {
            return weaks_.load();
        }

        bool incref_nz() noexcept
        {
            auto users = users_.load();
            if(0 == users)
                return false;
            while(!users_.compare_exchange_weak(users, users + 1));
            return true;
        }

    private:
        std::atomic<uint32_t>   users_ = 1;
        std::atomic<uint32_t>   weaks_ = 1;
    };

    template <typename T, typename RefCounter>
    class ref_count_ptr : public detail::ref_count_ptr_base<T, RefCounter>
    {
        using base_type = detail::ref_count_ptr_base<T, RefCounter>;
        using ref_count_interface = typename base_type::ref_count_interface;
        template <typename _T, typename _RefCounter> friend class ref_count_ptr;
        template <typename _T, typename _RefCounter> friend class detail::ref_count_ptr_base;
        template <typename _T> using ref_count_ptr_alias = ref_count_ptr<_T, RefCounter>;
        template <typename _T> using weak_ptr_alias = weak_ptr<_T, RefCounter>;
        template <typename _T, typename _RefCounter, typename ... Args>
        friend ref_count_ptr<_T, _RefCounter> make_ref(Args&& ... args);

    public:
        using element_type = typename base_type::element_type;
        using ref_counter = typename base_type::ref_counter;

    public: // constructor
        ref_count_ptr() = default;

        ref_count_ptr(ref_count_ptr const& other)
        {
            this->KATH_copy_construct_from(other);
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        ref_count_ptr(ref_count_ptr_alias<_T> const& other)
        {
            this->KATH_copy_construct_from(other);
        }

        ref_count_ptr(ref_count_ptr&& other)
        {
            this->KATH_move_construct_from(std::move(other));
        }
        
        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        ref_count_ptr(ref_count_ptr_alias<_T>&& other)
        {
            this->KATH_move_construct_from(std::move(other));
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        ref_count_ptr(weak_ptr_alias<_T> const& other)
        {
            if(!this->KATH_construct_from_weak(other))
            {
                // TODO ... propreate exception
                throw std::bad_weak_ptr{};
            }
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        explicit ref_count_ptr(_T* ptr)
        {
            using ref_counter_interface_t = detail::ref_count<element_type, RefCounter>;

            assert(ptr);
            /*this->KATH_set_impl(ptr, new ref_counter_interface_t{ ptr });
            init_enable_ref_from_this(*this, ptr);*/
            set_ptr_and_enable_ref(ptr, new ref_counter_interface_t{ ptr });
        }

        ~ref_count_ptr()
        {
            this->KATH_decref();
        }

    public: // operator
        ref_count_ptr& operator=(ref_count_ptr const& other)
        {
            ref_count_ptr{ other }.swap(*this);
            return *this;
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        ref_count_ptr& operator= (ref_count_ptr_alias<_T> const& other)
        {
            ref_count_ptr{ other }.swap(*this);
            return *this;
        }

        ref_count_ptr& operator= (ref_count_ptr&& other)
        {
            ref_count_ptr{ std::move(other) }.swap(*this);
            return *this;
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        ref_count_ptr& operator= (ref_count_ptr_alias<_T>&& other)
        {
            ref_count_ptr{ std::move(other) }.swap(*this);
            return *this;
        }

        element_type& operator* () const noexcept
        {
            return *get();
        }

        element_type* operator-> () const noexcept
        {
            return get();
        }

    public:
        using base_type::get;

        void swap(ref_count_ptr& other)
        {
            this->KATH_swap(other);
        }

        void reset()
        {
            ref_count_ptr{}.swap(*this);
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        void swap(_T* ptr)
        {
            ref_count_ptr{ ptr }.swap(*this);
        }

    private:
        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        void set_ptr_and_enable_ref(_T* ptr, ref_count_interface* counter)
        {
            assert(counter);
            this->KATH_set_impl(ptr, counter);
            init_enable_ref_from_this(*this, ptr);
        }

        template <typename _T, typename Pointer>
        void init_enable_ref_from_this(ref_count_ptr_alias<_T> const& rptr, Pointer ptr)
        {
            if constexpr(detail::can_be_refered_v<std::remove_pointer_t<Pointer>>)
            {
                if(ptr && ptr->weak_ptr_.expired())
                {
                    ptr->weak_ptr_ = reinterpret_cast<ref_count_ptr_alias<std::remove_const_t<_T>> const&>(rptr);
                }
            }
        }
    };

    template <typename T, typename RefCounter>
    class weak_ptr : public detail::ref_count_ptr_base<T, RefCounter>
    {
        using base_type = detail::ref_count_ptr_base<T, RefCounter>;
        using ref_counter = RefCounter;
        template <typename _T> using ref_count_ptr_alias = ref_count_ptr<_T, ref_counter>;
        template <typename _T> using weak_ptr_alias = weak_ptr<_T, ref_counter>;

    public:
        using element_type = typename base_type::element_type;

    public:
        // default constructor
        weak_ptr() = default;

        // copy constructor
        weak_ptr(weak_ptr const& other)
        {
            this->KATH_weak_construct_from(other);
        }
        
        // construct from weak ptr
        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        weak_ptr(weak_ptr_alias<_T> const& other)
        {
            this->KATH_weak_construct_from(other.lock());
        }

        weak_ptr(weak_ptr&& other)
        {
            this->KATH_move_construct_from(std::move(other));
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        weak_ptr(weak_ptr_alias<_T>&& other)
        {
            this->KATH_weak_construct_from(other.lock());
            other.reset();
        }

        // construct from ref count ptr
        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        weak_ptr(ref_count_ptr_alias<_T> const& other)
        {
            this->KATH_weak_construct_from(other);
        }

        ~weak_ptr()
        {
            this->KATH_decwref();
        }

    public: // operator
        weak_ptr& operator=(weak_ptr const& other)
        {
            weak_ptr{ other }.swap(*this);
            return *this;
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        weak_ptr& operator=(weak_ptr_alias<_T> const& other)
        {
            weak_ptr{ other }.swap(*this);
            return *this;
        }

        weak_ptr& operator= (weak_ptr&& other)
        {
            weak_ptr{ std::move(other) }.swap(*this);
            return *this;
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        weak_ptr& operator= (weak_ptr_alias<_T>&& other)
        {
            weak_ptr{ std::move(other) }.swap(*this);
            return *this;
        }

        template <typename _T, typename = decltype(std::declval<T*&>() = std::declval<_T*>())>
        weak_ptr& operator= (ref_count_ptr_alias<_T>&& other)
        {
            weak_ptr{ other }.swap(*this);
            return *this;
        }

    public: // public interface
        auto lock() const noexcept
        {
            using ref_count_ptr_t = ref_count_ptr<T, RefCounter>;

            ref_count_ptr_t ret;
            ret.KATH_construct_from_weak(*this);
            return ret;
        }

        void reset() noexcept
        {
            weak_ptr{}.swap(*this);
        }

        void swap(weak_ptr& other) noexcept
        {
            this->KATH_swap(other);
        }

        bool expired() const noexcept
        {
            return this->use_count() == 0;
        }
    };

    template <typename T, typename RefCounter>
    class enable_ref_from_this
    {
        template <typename _T> using ref_count_ptr_alias = ref_count_ptr<_T, RefCounter>;
        template <typename _T> using weak_ptr_alias = weak_ptr<_T, RefCounter>;
        template <typename _T, typename _RefCounter>
        friend class ref_count_ptr;

    public:
        using KATH_ref_enabled = T;

        constexpr enable_ref_from_this() noexcept = default;
        enable_ref_from_this(enable_ref_from_this const&) noexcept = default;
        enable_ref_from_this& operator=(enable_ref_from_this const&) noexcept = default;
        ~enable_ref_from_this() = default;
    
    public:
        // TODO
        //auto ref_from_this()
        //{
        //    return ref_count_ptr_alias<T>{ weak_ptr_ };
        //}

        //auto ref_from_this() const
        //{
        //    return ref_count_ptr_alias<std::add_const_t<T>>{ weak_ptr_ };
        //}

        auto ref_from_this() const
        {
            return ref_count_ptr_alias<T>{ weak_ptr_ };
        }

        //auto weak_from_this()
        //{
        //    return weak_ptr_alias<T>{ weak_ptr_ };
        //}

        //auto weak_from_this() const
        //{
        //    return weak_ptr_alias<T const>{ weak_ptr_ };
        //}

        auto weak_from_this() const
        {
            return weak_ptr_alias<T>{ weak_ptr_ };
        }

    protected:
        mutable weak_ptr<T, RefCounter> weak_ptr_;
    };

    template <typename T, typename RefCounter, typename ... Args>
    inline static ref_count_ptr<T, RefCounter> make_ref(Args&& ... args)
    {
        auto ref_count_obj = new detail::ref_count_obj<T, RefCounter>{ std::forward<Args>(args)... };
        ref_count_ptr<T, RefCounter> ptr;
        ptr.set_ptr_and_enable_ref(ref_count_obj->get_pointer(), ref_count_obj);
        return ptr;
    }
}