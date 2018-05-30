#include <iostream>
#include <kath.hpp>

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

int main(void)
{
    try {
	/*kath::state state;
	state["key"] = 1024;
	int a = state["key"];*/

    //std::cout << "a = " << a << std::endl;

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
    //lua_State* L = nullptr;
    //kath::stack_push(L, [](int a, int b){ return a + b; });

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

        //std::function<int(int,int)> ddd= [](int a, int b){ return a + b; };
        
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

        //using callable_traits_t = kath::callable_traits<decltype(overload)>;
        //std::is_same<lua_CFunction, callable_traits_t::signature_type>::value;
    }

	return 0;
}