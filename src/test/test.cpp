#include "kath.hpp"

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
	class foo
	{
		float d;
		std::string str;
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

	return 0;
}