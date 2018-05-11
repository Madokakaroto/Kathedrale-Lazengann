#pragma once

// router
namespace kath
{
	class dispather
	{
	public:
		dispather() = default;

		dispather(dispather const&) = delete;
		dispather& operator= (dispather const&) = delete;

	private:
		void init(lua_State* L)
		{
			// register function
			//::lua_pushcfunction(L, kathedral_lazengann);
			//::lua_setglobal(L, "kathedral_lazengann");
			
			// create metatable
			//auto metable_script = 
			//	"kath_functable = {" 
			//		"__call = function(table, ...)"
			//			"kathedral_lazengann(table.__signature, ...)"
			//		"end"
			//	"}";
			//luaL_dostring(L, metable_script);
		}

	private:
		std::vector<std::function<int(lua_State*)>> functors_;
	};
}