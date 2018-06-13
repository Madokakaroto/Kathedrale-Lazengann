#include <iostream>
#include <kath.hpp>
#include <limits>

class foo : public std::enable_shared_from_this<foo>
{
public:
	void operator() ()
	{
	}
};

class bar
{
	int a;
};

struct fee
{
};

namespace test
{
	class bar : public kath::enable_ref_from_this<bar, kath::fast_refcount>
	{
    public:
		float d;
		std::string str;
	};

    class foo : public bar
    {
    };

    class fee
    {
    public:
        void test(int a, double b)
        {
            auto temp = a + b;
            std::cout << temp << std::endl;
        }
    };

    static int add(int a, int b)
    {
        return a + b;
    }
}

template <typename T>
using ref_count_ptr = kath::ref_count_ptr<T, kath::fast_refcount>;

template <typename T>
using weak_ptr = kath::weak_ptr<T, kath::fast_refcount>;

template <typename ... Rets, typename ... Args>
auto pcall(Args&& ... args)
{
	auto a = sizeof...(Rets);
	auto b = sizeof...(Args);

	return std::tuple<Rets...>{};
}

int main(void)
{
    //auto r =  kath::is_string_view<std::string_view>::value;
    //r = kath::detail::is_instance_of_std_string<std::string>::value;

    //std::cout << r << std::endl;

    //return 0;
    
    try {
	/*kath::state state;
	state["key"] = 1024;
	int a = state["key"];*/

		char* key = new char[4];
		key[0] = 'k';
		key[1] = 'e';
		key[2] = 'y';
		key[3] = '\0';

		kath::lua lua;
		lua["key"] = 1024;
		int a = lua[key];

		delete[] key;

		std::cout<< "Hello ...."  << std::endl; 
    
		{
			auto ptr = new test::foo{};
			ref_count_ptr<test::foo> sptr1{ ptr };
		}

		{
			auto ptr = new test::foo const{};
			ref_count_ptr<test::foo const> rptr1{ ptr };
			auto rptr2 = ptr->ref_from_this();
			auto wptr = ptr->weak_from_this();
		}

		std::cout << "End...." << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cout << e.what() << std::endl;
    }

    // test compile
    lua_State* L = nullptr;
    kath::stack_push(L, [](int a, int b){ return a + b; });

    {
        using std::placeholders::_1;
        using std::placeholders::_2;

        auto tuple = std::make_tuple(1, _2, _1);

       // using type = kath::detail::placeholders_to_indices<decltype(tuple)>;

    }


    // test bind
    {
        using namespace std::placeholders;
        test::fee fee;

        auto bind0 = kath::bind(&test::fee::test, fee, _1, _2);
        auto bind1 = kath::bind(&test::fee::test, _3, _2, _1);
        auto bind2 = kath::bind(&test::fee::test);
        auto bind3 = kath::bind(test::add, _2, _1);
        auto bind4 = kath::bind([](int a, int b){ return a + b; }, _1 , _2);
        
        bind0(1024, 5.13);
        bind1(1.1111, 22222, fee);
        bind2(&fee, 2222, 3.33333);

        std::cout << kath::is_callable_v<decltype(bind0)> << std::endl;
        std::cout << kath::is_callable_v<decltype(bind1)> << std::endl;
        std::cout << kath::is_callable_v<decltype(bind2)> << std::endl;

        lua_State* L = nullptr;
        kath::stack_push(L, bind0);

        auto lambda = [](int a, int b){ return a + b; };
        using int_int_tuple = kath::callable_traits<decltype(lambda)>::args_pack;
        int_int_tuple ii = { 1, 2 };

        auto overload = kath::overload( 
            [](int a, int b){ return a + b; }, 
            [](int a, int b, int c){ return a + b + c; });
        kath::stack_push(L, overload);

        auto overload_ptr = &overload;
        kath::stack_push(L, overload_ptr);
    }

    // test compile kath::stack_push
    {
        lua_State* L = nullptr;
        kath::stack_push(L, kath::nil);
        kath::stack_push(L, 'c');
        kath::stack_push(L, true);
        kath::stack_push(L, std::numeric_limits<char>::max());
        kath::stack_push(L, std::numeric_limits<int8_t>::max());
        kath::stack_push(L, std::numeric_limits<int16_t>::max());
        kath::stack_push(L, std::numeric_limits<int32_t>::max());
        kath::stack_push(L, std::numeric_limits<int64_t>::max());
        kath::stack_push(L, std::numeric_limits<uint8_t>::max());
        kath::stack_push(L, std::numeric_limits<uint16_t>::max());
        kath::stack_push(L, std::numeric_limits<uint32_t>::max());
        kath::stack_push(L, std::numeric_limits<uint64_t>::max());

        char const* c_str = "Hello world";
        std::string std_str = c_str;
        kath::stack_push(L, "Hello world");
        kath::stack_push(L, c_str);
        kath::stack_push(L, std_str);
        kath::stack_push(L, std::move(std_str));
    }

    // test compile kath::stack_get
    {
        lua_State* L = nullptr;
        auto a = kath::stack_get<int&>(L);
        auto b = kath::stack_get<int const&>(L);
    }

    {
        lua_State* L = nullptr;
        auto a = kath::stack_check<int&>(L);
    }

    // test multi return 
    {
        kath::is_valid_tuple<int>::value;
        kath::is_valid_tuple<std::pair<int, bool>>::value;
        kath::is_valid_tuple<std::array<int, 5>>::value;
        kath::is_valid_tuple<std::tuple<>>::value;
        kath::is_valid_tuple<std::tuple<int, double>>::value;
    }

	// test pcall
	{
		kath::lua lua;
		int a = lua["function1"]();

        int b;
		kath::tie(a, b)  = lua["function2"]();

        {
             auto [a, b] = lua["function3"].pcall<int, int>(1, 2, 3);
             auto [c, d] = lua["talbe"]["function"](KATH_RESULT_T(int, int), 1, 2, 3);
        }
       
	}
	return 0;
}