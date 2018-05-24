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

    std::cout << kath::can_be_referenced_from_this<test::bar>::value << std::endl;
    std::cout << kath::can_be_referenced_from_this<test::foo>::value << std::endl;

	return 0;
}