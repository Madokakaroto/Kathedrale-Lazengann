#include "kath.hpp"

class foo
{
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
	};
}

int main(void)
{
	kath::state state;
	state["key"] = 1024;
	int a = state["key"];
	
	return 0;
}