/*
** $Id: lprefix.h $
** Definitions for Lua code that must come before any other header file
** See Copyright Notice in lua.h
*/

#ifndef lprefix_h
#define lprefix_h


/*
** Allows POSIX/XSI stuff
*/
#if !defined(LUA_USE_C89)	/* { */

#if !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE           600
#elif _XOPEN_SOURCE == 0
#undef _XOPEN_SOURCE  /* use -D_XOPEN_SOURCE=0 to undefine it */
#endif

/*
** Allows manipulation of large files in gcc and some other compilers
*/
#if !defined(LUA_32BITS) && !defined(_FILE_OFFSET_BITS)
#define _LARGEFILE_SOURCE       1
#define _FILE_OFFSET_BITS       64
#endif

#endif				/* } */


/*
** Windows stuff
*/
#if defined(_WIN32)	/* { */

#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS  /* avoid warnings about ISO C functions */
#endif

#include "utf8_prefix.h"

#endif			/* } */

#include "luai_devent.h"
#include <stdlib.h>

#if !defined(NDEBUG)

#include "lauxlib.h"
inline void _bee_lua_assert(const char* message, const char* file, unsigned line) {
    fprintf(stderr, "(%s:%d) %s\n", file, line, message);
    fflush(stderr);
    abort();
}
inline void _bee_lua_apicheck(lua_State* L, const char* message, const char* file, unsigned line) {
    fprintf(stderr, "(%s:%d) %s\n", file, line, message);
    fflush(stderr);
    if (!lua_checkstack(L, LUA_MINSTACK)) {
        abort();
    }
    luaL_traceback(L, L, 0, 0);
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    fflush(stderr);
    lua_pop(L, 1);
    abort();
}
#    if defined(lua_assert)
#        undef lua_assert
#    endif
#    if defined(luai_apicheck)
#        undef luai_apicheck
#    endif
#    define lua_assert(expression)       (void)((!!(expression)) || (_bee_lua_assert  (   #expression, __FILE__, (unsigned)(__LINE__)), 0))
#    define luai_apicheck(l, expression) (void)((!!(expression)) || (_bee_lua_apicheck(l, #expression, __FILE__, (unsigned)(__LINE__)), 0))
#endif

#define l_randomizePivot(L) (~0)

#if defined(_MSC_VER) && !defined(__SANITIZE_ADDRESS__)

#include "fast_setjmp.h"

#define LUAI_THROW(L,c) fast_longjmp((c)->b, 1)
#define LUAI_TRY(L,c,a) if (fast_setjmp((c)->b) == 0) { a }
#define luai_jmpbuf     jmp_buf

#endif

#endif
