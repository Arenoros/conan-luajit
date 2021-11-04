#ifndef TARANTOOL_LUA_UTILS_H_INCLUDED
#define TARANTOOL_LUA_UTILS_H_INCLUDED
/*
 * Copyright 2010-2015, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

// #include <stdint.h>
// #include <string.h>
// #include <math.h> /* floor */

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

#include "../lua.h"
#include "../lauxlib.h" /* luaL_error */

/** \cond public */

/**
 * Checks whether a value on the Lua stack is a cdata.
 *
 * Unlike <luaL_checkcdata>() this function does not raise an
 * error. It is useful to raise a domain specific error.
 *
 * Lua API and module API don't expose LUA_TCDATA constant.
 * We have no guarantee that this constant will remain the same in
 * future LuaJIT versions. So this function should be used in
 * modules instead of `lua_type(L, idx) == LUA_TCDATA`.
 *
 * @param L    Lua state.
 * @param idx  Acceptable index on the Lua stack.
 *
 * @retval 1   If the value at the given index is a cdata.
 * @retval 0   Otherwise.
 */
LUA_API int
luaT_iscdata(struct lua_State *L, int idx);

/**
 * @brief Push cdata of given \a ctypeid onto the stack.
 * CTypeID must be used from FFI at least once. Allocated memory returned
 * uninitialized. Only numbers and pointers are supported.
 * @param L Lua State
 * @param ctypeid FFI's CTypeID of this cdata
 * @sa luaL_checkcdata
 * @return memory associated with this cdata
 */
LUA_API void *
luaT_pushcdata(struct lua_State *L, uint32_t ctypeid);

/**
 * @brief Checks whether the function argument idx is a cdata
 * @param L Lua State
 * @param idx stack index
 * @param ctypeid FFI's CTypeID of this cdata
 * @sa luaL_pushcdata
 * @return memory associated with this cdata
 */
LUA_API void *
luaT_checkcdata(struct lua_State *L, int idx, uint32_t *ctypeid);

/**
 * @brief Sets finalizer function on a cdata object.
 * Equivalent to call ffi.gc(obj, function).
 * Finalizer function must be on the top of the stack.
 * @param L Lua State
 * @param idx object
 */
LUA_API void
luaT_setcdatagc(struct lua_State *L, int idx);

/**
* @brief Return CTypeID (FFI) of given СDATA type
* @param L Lua State
* @param ctypename С type name as string (e.g. "struct request" or "uint32_t")
* @sa luaL_pushcdata
* @sa luaL_checkcdata
* @return CTypeID
*/
LUA_API uint32_t
luaT_ctypeid(struct lua_State *L, const char *ctypename);

/**
* @brief Declare symbols for FFI
* @param L Lua State
* @param ctypename C definitions, e.g "struct stat"
* @sa ffi.cdef(def)
* @retval 0 on success
* @retval LUA_ERRRUN, LUA_ERRMEM, LUA_ERRERR otherwise
*/
LUA_API int
luaT_cdef(struct lua_State *L, const char *ctypename);

/** \endcond public */

/**
 * @brief Return CTypeID (FFI) of given CDATA type,
 * register a metatable with \a methods to be
 * associated with every value of the given
 * type on its creation iva FFI.
 * @sa luaL_register_type
 * @sa luaL_ctypeid
 * @return CTypeID
 */
LUA_API uint32_t
luaT_metatype(struct lua_State *L, const char *ctypename,
	      const struct luaL_Reg *methods);

static inline lua_Integer
luaT_arrlen(struct lua_State *L, int idx)
{
	lua_Integer max = 0;
	lua_pushnil(L);
	while (lua_next(L, idx)) {
		lua_pop(L, 1); /* pop the value */
		if (lua_type(L, -1) != LUA_TNUMBER)
			continue;
		lua_Number k = lua_tonumber(L, -1);
		if (k <= max || floor(k) != k)
			continue;
		max = k;
	}
	return max;
}

static inline lua_Integer
luaT_maplen(struct lua_State *L, int idx)
{
	lua_Integer size = 0;
	lua_pushnil(L);
	while (lua_next(L, idx)) {
		lua_pop(L, 1); /* pop the value */
		size++;
	}
	return size;
}

LUA_API void
luaT_register_type(struct lua_State *L, const char *type_name,
		   const struct luaL_Reg *methods);


LUA_API void
luaT_register_module(struct lua_State *L, const char *modname,
		     const struct luaL_Reg *methods);

/** \cond public */

/**
 * Push uint64_t onto the stack
 *
 * @param L is a Lua State
 * @param val is a value to push
 */
LUA_API void
luaT_pushuint64(struct lua_State *L, uint64_t val);

/**
 * Push int64_t onto the stack
 *
 * @param L is a Lua State
 * @param val is a value to push
 */
LUA_API void
luaT_pushint64(struct lua_State *L, int64_t val);

/**
 * Checks whether the argument idx is a uint64 or a convertable string and
 * returns this number.
 * \throws error if the argument can't be converted.
 */
LUA_API uint64_t
luaT_checkuint64(struct lua_State *L, int idx);

/**
 * Checks whether the argument idx is a int64 or a convertable string and
 * returns this number.
 * \throws error if the argument can't be converted.
 */
LUA_API int64_t
luaT_checkint64(struct lua_State *L, int idx);

/**
 * Checks whether the argument idx is a uint64 or a convertable string and
 * returns this number.
 * \return the converted number or 0 of argument can't be converted.
 */
LUA_API uint64_t
luaT_touint64(struct lua_State *L, int idx);

/**
 * Checks whether the argument idx is a int64 or a convertable string and
 * returns this number.
 * \return the converted number or 0 of argument can't be converted.
 */
LUA_API int64_t
luaT_toint64(struct lua_State *L, int idx);

/**
 * Like lua_tolstring, but supports metatables, booleans and nil properly.
 */
LUA_API const char *
luaT_tolstring(lua_State *L, int idx, size_t *ssize);

/**
 * Check whether a Lua object is a function or has
 * metatable/metatype with a __call field.
 *
 * Note: It does not check type of __call metatable/metatype
 * field.
 */
LUA_API int
luaT_iscallable(lua_State *L, int idx);

/**
 * Push ffi's NULL (cdata<void *>: NULL) onto the stack.
 * Can be used as replacement of nil in Lua tables.
 * @param L stack
 */

// LUALIB_API int
// luaL_checkconstchar(struct lua_State *L, int idx, const char **res,
// 		    uint32_t *cdata_type_p)
// {
// 	if (lua_type(L, idx) != LUA_TCDATA)
// 		return -1;
// 	uint32_t cdata_type;
// 	void *cdata = luaL_checkcdata(L, idx, &cdata_type);
// 	if (cdata_type != CTID_CHAR_PTR && cdata_type != CTID_CONST_CHAR_PTR)
// 		return -1;
// 	*res = cdata != NULL ? *(const char **) cdata : NULL;
// 	*cdata_type_p = cdata_type;
// 	return 0;
// }

// LUALIB_API int
// tarantool_lua_utils_init(struct lua_State *L)
// {
// 	/* Create NULL constant */
// 	*(void **) luaL_pushcdata(L, CTID_P_VOID) = NULL;
// 	luaL_nil_ref = luaL_ref(L, LUA_REGISTRYINDEX);

// 	CTID_CHAR_PTR = luaL_ctypeid(L, "char *");
// 	assert(CTID_CHAR_PTR != 0);
// 	CTID_CONST_CHAR_PTR = luaL_ctypeid(L, "const char *");
// 	assert(CTID_CONST_CHAR_PTR != 0);

// 	return 0;
// }

// LUALIB_API int
// luaL_isnull(struct lua_State *L, int idx)
// {
// 	if (lua_type(L, idx) == LUA_TCDATA) {
// 		GCcdata *cd = cdataV(L->base + idx - 1);
// 		return cd->ctypeid == CTID_P_VOID &&
// 			*(void **)cdataptr(cd) == NULL;
// 	}
// 	return 1;
// }
// LUALIB_API void
// luaL_pushnull(struct lua_State *L)
// {
// 	lua_rawgeti(L, LUA_REGISTRYINDEX, luaL_nil_ref);
// }

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */


#endif /* TARANTOOL_LUA_UTILS_H_INCLUDED */
