#include <kath.hpp>

class foo
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
	class bar
	{
    public:
		float d;
		std::string str;
	};

    class foo : public bar
    {
    };
}

int main(void)
{
	kath::state state;
	state["key"] = 1024;
	int a = state["key"];
	
	//kath::is_callable<bar>::value;

	//std::has_aggre<test::foo>::value
	//test::foo f = { 1.0f, "sdfsdf" };

    using foo_ptr = kath::ref_count_ptr<test::foo, kath::fast_refcount>;
    foo_ptr ptr{ new test::foo{} };
    {
        foo_ptr ptr1 = ptr;
    }
    

    kath::ref_count_ptr<test::bar, kath::fast_refcount> ptr2 = ptr;
    kath::weak_ptr<test::bar, kath::fast_refcount> ptr_w = ptr2;
    //std::is_trivially_destructible<void const>::value
    auto ptr3 = ptr_w.lock();

    ptr3->d;
	return 0;
}