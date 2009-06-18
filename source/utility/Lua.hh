/*
 * Copyright (C) 2008 Valentin Ziegler, ExactCODE GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2. A copy of the GNU General
 * Public License can be found in the file LICENSE.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT-
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 * Alternatively, commercial licensing options are available from the
 * copyright holder ExactCODE GmbH Germany.
 */

#ifndef __LUA_HH
#define __LUA_HH

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <string.h>
#include <string>
#include <vector>
#include <algorithm>

namespace LuaWrapper {

  class AutoReleaseItem;
  extern AutoReleaseItem* autoReleaseList;

  class LuaClassData
  {
  public:
    LuaClassData(const std::string& name)
    {
      handle = name;
    }

    void insertMetaTablePtr(void* ptr)
    {
      metaTablePtrs.push_back(ptr);
      std::sort(metaTablePtrs.begin(), metaTablePtrs.end());
    }


    bool isCompatible(void* ptr)
    {
      return std::binary_search(metaTablePtrs.begin(), metaTablePtrs.end(), ptr);
    }

    std::string handle;
    std::vector <void*> metaTablePtrs;
  };

  template <typename T>
  class LuaClass
  {
  public:
    LuaClass(const std::string& name)
    {
      if (!data) {
	data = new LuaClassData(name);
      }
    }
    
    ~LuaClass()
    {
      if (data) {
	delete data; data = 0;
      }
    }
    
    static const char* luahandle()
    {
      return data->handle.c_str();
    }
  
    static T* getPtr(lua_State* L, int index)
    {
      void** p = (void**)lua_touserdata(L, index);
      if (p)
	if (lua_getmetatable(L, index)) {
	  void* meta = (void*)lua_topointer(L, -1);
	  lua_pop(L, 1);
	  if (data->isCompatible(meta))
	    return (T*)*p;
	}
      luaL_typerror(L, index, luahandle());
      return 0;  /* avoid warnings */
    }

    static T** getRawPtr(lua_State* L, int index)
    {
      void** p = (void**)lua_touserdata(L, index);
      if (p)
	if (lua_getmetatable(L, index)) {
	  void* meta = (void*)lua_topointer(L, -1);
	  lua_pop(L, 1);
	  if (data->isCompatible(meta))
	    return (T**)p;
	}
      luaL_typerror(L, index, luahandle());
      return 0;  /* avoid warnings */
    }

    static void packPtr(lua_State* L, T* obj)
    {
      T** ptr = (T**)lua_newuserdata(L, sizeof(T*));
      *ptr = obj;
      luaL_getmetatable(L, luahandle());
      lua_setmetatable(L, -2);
    }

    static LuaClassData* data;
  };

  template <typename T>
  LuaClassData* LuaClass<T>::data = 0;


  class AutoReleaseItem
  {
  public:
    AutoReleaseItem() {
      this->next = autoReleaseList;
      autoReleaseList = this;
    }

    virtual ~AutoReleaseItem() {}
    AutoReleaseItem* next;
  };

  template <typename T>
  class EncapsulatedReleaseItem : public AutoReleaseItem
  {
  public:
    T t;
    EncapsulatedReleaseItem(const T& init)
      : t(init){}
  };

  template <typename T>
  class ReleaseReferenceReleaseItem : public AutoReleaseItem
  {
  public:
    T t;
    ReleaseReferenceReleaseItem(const T& init)
      : t(init){}
    ~ReleaseReferenceReleaseItem()
    {
      t.releaseReference();
    }
  };

  static inline void runAutoRelease()
  {
    while (autoReleaseList) {
      AutoReleaseItem* next = autoReleaseList->next;
      delete autoReleaseList;
      autoReleaseList = next;
    }
  }


  class LuaTable
  {
  public:
    int handle;
    lua_State* my_L;

    LuaTable() // stub ctor, for default initializers etc
    {
      my_L = 0; // make sure missuse of this constructor in code
      // be noticed quick ;)
      handle = 0;
    }

    LuaTable(lua_State* L) // create a new table and reference
    {
      lua_newtable(L);
      handle = luaL_ref(L, LUA_REGISTRYINDEX);
      my_L = L;
    }

    LuaTable(lua_State* L, int stackIndex) // reference a table on stack
    {
      luaL_checktype(L, stackIndex, LUA_TTABLE);
      lua_pushvalue(L, stackIndex);
      handle = luaL_ref(L, LUA_REGISTRYINDEX);
      my_L = L;
    }

    LuaTable(const LuaTable& src)
    {
      handle = src.handle;
      my_L = src.my_L;
    }

    void push()
    {
      lua_rawgeti(my_L, LUA_REGISTRYINDEX, handle);
    }

    void autoRelease()
    {
      new ReleaseReferenceReleaseItem<LuaTable>(*this);
    }

    void releaseReference()
    {
      luaL_unref(my_L, LUA_REGISTRYINDEX, handle);
      my_L = 0;
    }
    
    bool valid() const
    {
      return my_L != 0;
    }
    
    operator bool() const
    {
      return valid();
    }
    
    bool exists(const char* key)
    {
      push();
      lua_getfield(my_L, -1, key);
      bool result = (lua_isnil(my_L, -1) == 0);
      lua_pop(my_L, 2);
      return result;
    }
    
    bool exists(int ikey)
    {
      push();
      lua_pushinteger(my_L, ikey);
      lua_gettable(my_L, -2);
      bool result = (lua_isnil(my_L, -1) == 0);
      lua_pop(my_L, 2);
      return result;
    }

    template <typename T> T get(const char* key);
    template <typename T> T get(int ikey);

    template <typename T> void set(const char* key, T obj);
    template <typename T> void set(int ikey, T obj);
    
    template <typename T> T defaultGet(const char* key, T def)
    {
      if (exists(key))
        return get<T>(key);
      set<T>(key, def);
      return def;
    }

   template <typename T> T defaultGet(int ikey, T def)
    {
      if (exists(ikey))
        return get<T>(ikey);
      set<T>(ikey, def);
      return def;
    }
    
  };

  class LuaFunctionBase
  {
  public:
    virtual bool prepareStack(lua_State* L) = 0;
    virtual void cleanStack(lua_State* L) = 0;
    int addValues;
  };

  class Global : public LuaFunctionBase
  {
  public:
    Global(const char* function_name)
    {
      addValues = 0;
      name = function_name;
    }

    virtual bool prepareStack(lua_State* L)
    {
      lua_getfield(L, LUA_GLOBALSINDEX, name);
      if (!lua_isfunction(L, -1)) {
	lua_pop(L, 1);
	return false;
      }
      return true;
    }

    virtual void cleanStack(lua_State* L) {}
    const char* name;
  };

  class Method : public LuaFunctionBase
  {
  public:
    Method(LuaTable obj, const char* function_name)
      : table(obj)
    {
      addValues = 1;
      name = function_name;
    }
    
    virtual bool prepareStack(lua_State* L)
    {
      if (table.my_L != L)
	return false;

      table.push();
      lua_getfield(L, -1, name);
      if (!lua_isfunction(L, -1)) {
	lua_pop(L, 2);
	return false;
      }
      lua_pushvalue(L, -2); // first arg is self
      return true;
    }

    virtual void cleanStack(lua_State* L)
    {
      lua_pop(L, 1); // remove table reference from stack
    }

    const char* name;
    LuaTable table;
  };
  
  class LuaFunction : public LuaFunctionBase
  {
  public:
    int handle;
    lua_State* my_L;

    LuaFunction() // stub ctor, for default initializers etc
    {
      my_L = 0; // make sure missuse of this constructor in code
      // be noticed quick ;)
      handle = 0;
      addValues = 0;
    }

    LuaFunction(lua_State* L, int stackIndex) // reference a function on stack
    {
      luaL_checktype(L, stackIndex, LUA_TFUNCTION);
      lua_pushvalue(L, stackIndex);
      handle = luaL_ref(L, LUA_REGISTRYINDEX);
      my_L = L;
      addValues = 0;
    }

    LuaFunction(const LuaFunction& src)
    {
      handle = src.handle;
      my_L = src.my_L;
      addValues = 0;
    }

    virtual bool prepareStack(lua_State* L)
    {
      if (my_L != L)
	return false;
      lua_rawgeti(my_L, LUA_REGISTRYINDEX, handle);
      return true;
    }

    virtual void cleanStack(lua_State* L)
    {
    }

    void releaseReference()
    {
      luaL_unref(my_L, LUA_REGISTRYINDEX, handle);
      my_L = 0;
    }
  };





  template <typename T, T RET>
  class DefaultConst
  {
  public:
    static const T ret = RET;
  };
  
  template <typename T>
  class DefaultInitializer
  {
  public:
    DefaultInitializer() : ret() {};
    T ret;
  };
  

  template <typename T>
  class Unpack {};

  template <>
  class Unpack<int>
  {
  public:
    static int convert(lua_State* L, int index)
    {
      return (int)luaL_checkinteger(L, index);
    }
  };

  template <>
  class Unpack<unsigned int>
  {
  public:
    static int convert(lua_State* L, int index)
    {
      return (unsigned int)luaL_checkinteger(L, index);
    }
  };

  template <>
  class Unpack<double>
  {
  public:
    static double convert(lua_State* L, int index)
    {
      return (double)luaL_checknumber(L, index);
    }
  };

  template <>
  class Unpack<bool>
  {
  public:
    static bool convert(lua_State* L, int index)
    {
      // Why the heck this function is not there ????
      // return (bool)luaL_checkboolean(L, index);
      luaL_checktype(L, index, LUA_TBOOLEAN);
      return (bool)lua_toboolean(L, index);
    }
  };

  template <>
  class Unpack<const char*>
  {
  public:
    static const char* convert(lua_State* L, int index)
    {
      return luaL_checkstring(L, index);
    }
  };

  template <>
  class Unpack<std::string>
  {
  public:
    static std::string convert(lua_State* L, int index)
    {
      return std::string(luaL_checkstring(L, index));
    }
  };

  template <>
  class Unpack<std::string&>
  {
  public:
    static std::string& convert(lua_State* L, int index)
    {
      typedef EncapsulatedReleaseItem<std::string> AutoRelease;
      AutoRelease* rel = new AutoRelease(std::string(luaL_checkstring(L, index)));
      return rel->t;
    }
  };

  template <>
  class Unpack<const std::string&>
  {
  public:
    static const std::string& convert(lua_State* L, int index)
    {
      return Unpack<std::string&>::convert(L, index);
    }
  };

  template <>
  class Unpack<LuaTable>
  {
  public:
    static LuaTable convert(lua_State* L, int index)
    {
      return LuaTable(L, index);
    }
  };
  
  template <>
  class Unpack<LuaFunction>
  {
  public:
    static LuaFunction convert(lua_State* L, int index)
    {
      return LuaFunction(L, index);
    }
  };


  template <typename T>
  class Unpack<T&>
  {
  public:
    static T& convert(lua_State* L, int index)
    {
      return *LuaClass<T>::getPtr(L, index);
    }
  };

  template <typename T>
  class Unpack<const T&>
  {
  public:
    static const T& convert(lua_State* L, int index)
    {
      return *LuaClass<T>::getPtr(L, index);
    }
  };

  template <typename T>
  class Unpack<T*>
  {
  public:
    static T* convert(lua_State* L, int index)
    {
      return LuaClass<T>::getPtr(L, index);
    }
  };

  template <typename T>
  class Pack {};

  template <>
  class Pack<int>
  {
  public:
    static void convert(lua_State* L, int value)
    {
      lua_pushinteger(L, value);
    }
  };

  template <>
  class Pack<unsigned int>
  {
  public:
    static void convert(lua_State* L, unsigned int value)
    {
      lua_pushinteger(L, value);
    }
  };

  template <>
  class Pack<double>
  {
  public:
    static void convert(lua_State* L, double value)
    {
      lua_pushnumber(L, value);
    }
  };

  template <>
  class Pack<bool>
  {
  public:
    static void convert(lua_State* L, bool value)
    {
      lua_pushboolean(L, value);
    }
  };

  template <>
  class Pack<const char*>
  {
  public:
    static void convert(lua_State* L, const char* value)
    {
      lua_pushstring(L, value);
    }
  };

  template <>
  class Pack<std::string>
  {
  public:
    static void convert(lua_State* L, std::string value)
    {
      lua_pushstring(L, value.c_str());
    }
  };

  template <>
  class Pack<std::string&>
  {
  public:
    static void convert(lua_State* L, std::string value)
    {
      lua_pushstring(L, value.c_str());
    }
  };

  template <>
  class Pack<const std::string&>
  {
  public:
    static void convert(lua_State* L, std::string value)
    {
      lua_pushstring(L, value.c_str());
    }
  };

  template <>
  class Pack<LuaTable>
  {
  public:
    static void convert(lua_State* L, LuaTable value)
    {
      value.push();
    }
  };
  
  template <>
  class Pack<LuaFunction>
  {
  public:
    static void convert(lua_State* L, LuaFunction value)
    {
      value.prepareStack(L);
    }
  };
 

  template <typename T>
  class Pack <T*>
  {
  public:
    static void convert(lua_State* L, T* obj)
    {
      LuaClass<T>::packPtr(L, obj);
    }
  };

  template <typename T>
  class Pack <T&>
  {
  public:
    static void convert(lua_State* L, T& obj)
    {
      LuaClass<T>::packPtr(L, *obj);
    }
  };


  template <typename T> T LuaTable::get(const char* key)
  {
    push();
    lua_getfield(my_L, -1, key);
    T ret = Unpack<T>::convert(my_L, -1);
    lua_pop(my_L, 2);
    return ret;
  }

  template <typename T> T LuaTable::get(int ikey)
  {
    push();
    lua_pushinteger(my_L, ikey);
    lua_gettable(my_L, -2);
    T ret = Unpack<T>::convert(my_L, -1);
    lua_pop(my_L, 2);
    return ret;
  }

  template <typename T> void LuaTable::set(const char* key, T obj)
  {
    push();
    Pack<T>::convert(my_L, obj);
    lua_setfield(my_L, -2, key);
    lua_pop(my_L, 1);
  }

  template <typename T> void LuaTable::set(int ikey, T obj)
  {
    push();
    lua_pushinteger(my_L, ikey);
    Pack<T>::convert(my_L, obj);
    lua_settable(my_L, -3);
    lua_pop(my_L, 1);
  }

  template <typename ORIG, typename ALIAS>
  class UnpackTypedef
  {
  public:
    static ALIAS convert(lua_State* L, int index)
    {
      return (ALIAS)Unpack<ORIG>::convert(L, index);
    }
  };

  template <typename ORIG, typename ALIAS>
  class PackTypedef
  {
  public:
    static void convert(lua_State* L, ALIAS value)
    {
      Pack<ORIG>::convert(L, (ORIG)value);
    }
  };


#include "LuaWrappers.hh"

  // does not need to be expanded in LuaWrappers.hh hackery, since
  // dtors never take arguments ;)
  template <typename OBJ>
  class DtorWrapper
  {
  public:
    typedef OBJ myobjectT;

    static int Wrapper(lua_State* L)
    {
      OBJ** obj = LuaClass<OBJ>::getRawPtr(L, 1);
      if (*obj) {
	delete *obj; *obj = 0;
      }
      return 0;
    }

    static const bool hasmeta = true;
    static const bool noindex = false;
  };



  template <typename WRAPPER>
  class ExportToLua {
  public:
    typedef typename WRAPPER::myobjectT objectT;

    ExportToLua(lua_State* L, const char* module_name,  const char* function_name)
    {
      //std::cout << "register " << function_name << std::endl;
      luaL_Reg entry[2] = {{function_name, WRAPPER::Wrapper}, {NULL, NULL}};
      if (WRAPPER::hasmeta) {
	luaL_getmetatable(L, LuaClass<objectT>::luahandle());
	if (strncmp(function_name, "__", 2) != 0 && !WRAPPER::noindex) {
	  lua_getfield(L, -1, "__index");
	  luaL_register(L, 0, entry);
	  lua_pop(L, 2);
	} else { // allow direct registration of ctors and __* methods
	  luaL_register(L, 0, entry);
	  lua_pop(L, 1);
	}
      } else {
	luaL_register(L, module_name, entry);
      }
    }
  };

  template <typename T>
  void DeclareToLua(lua_State* L, const char* module_name = 0)
  {
    luaL_newmetatable(L, LuaClass<T>::luahandle());

    if (module_name) {
      // drop a reference to the metatable into the module table
      luaL_findtable(L, LUA_GLOBALSINDEX, module_name, 1);
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, LuaClass<T>::luahandle());
      lua_pop(L, 1);
    }

    // create index table
    lua_newtable(L);
    lua_setfield(L, -2, "__index");

    // remember metatable raw pointer for later type checks
    LuaClass<T>::data->insertMetaTablePtr((void*)lua_topointer(L, -1));
    lua_pop(L, 1);
  }

  template <typename BASE, typename DERIVED>
  void InheritMeta(lua_State* L)
  {
    luaL_getmetatable(L, LuaClass<DERIVED>::luahandle()); // -5
    lua_getfield(L, -1, "__index"); // -4
    luaL_getmetatable(L, LuaClass<BASE>::luahandle()); // -3
    lua_getfield(L, -1, "__index"); // -2

    lua_pushnil(L);  /* -1 first key */
    while (lua_next(L, -2) != 0) {
      lua_pushvalue(L, -2); // copy key and move down
      lua_insert(L, -3);
      lua_settable(L, -6); // DERIVED __index at -6
    }
    lua_pop(L, 5);
  }

  template <typename BASE, typename DERIVED>
  void AllowDowncast(lua_State* L)
  {
    luaL_getmetatable(L, LuaClass<DERIVED>::luahandle());
    LuaClass<BASE>::data->insertMetaTablePtr((void*)lua_topointer(L, -1));
    lua_pop(L, 1);
  }

  template <typename C>
  void RegisterValue(lua_State* L, const char* module_name, const char* name, C value)
  {
    luaL_findtable(L, LUA_GLOBALSINDEX, module_name, 1);
    Pack<C>::convert(L, value);
    lua_setfield(L, -2, name);
    lua_pop(L, 1);
  }
}


// some convenient defines:
#define ToLuaTypedef(ORIG, ALIAS) \
namespace LuaWrapper { \
  template <> class Unpack<ALIAS>:public UnpackTypedef<ORIG, ALIAS>{}; \
  template <> class Pack<ALIAS>:public PackTypedef<ORIG, ALIAS>{}; }


#endif
