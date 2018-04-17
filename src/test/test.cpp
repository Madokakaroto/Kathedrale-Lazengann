#include "kath.hpp"

int main(void)
{
	kath::state state;
	state["key"] = 1024;
	int a = state["key"];
	
	return 0;
}