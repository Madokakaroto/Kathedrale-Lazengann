#include <kath.hpp>

class foo : std::enable_shared_from_this<foo>
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

int main(void)
{
	kath::state state;
	state["key"] = 1024;
	int a = state["key"];
	
    {
        auto ptr = new foo const{};
        std::shared_ptr<foo const> sptr{ ptr };
    }
    
    {
        auto ptr = new foo{};
        std::shared_ptr<foo const> sptr1{ ptr };
        std::shared_ptr<foo> sptr2{ ptr };
    }

    {
        auto ptr = new test::foo{};
        ref_count_ptr<test::foo> ptr1{ ptr };
        ref_count_ptr<test::foo const> ptr2{ ptr };
    }

    {
        auto ptr = new test::foo const{};
        ref_count_ptr<test::foo const> ptr2{ ptr };
    }
	return 0;
}