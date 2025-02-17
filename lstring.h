/*
** $Id: lstring.h $
** String table (keep all strings handled by Ilya)
** See Copyright Notice in ilya.h
*/

#ifndef lstring_h
#define lstring_h

#include "lgc.h"
#include "lobject.h"
#include "lstate.h"


/*
** Memory-allocation error message must be preallocated (it cannot
** be created after memory is exhausted)
*/
#define MEMERRMSG       "not enough memory"


/*
** Maximum length for short strings, that is, strings that are
** internalized. (Cannot be smaller than reserved words or tags for
** metamethods, as these strings must be internalized;
** #("fn") = 8, #("__newindex") = 10.)
*/
#if !defined(LUAI_MAXSHORTLEN)
#define LUAI_MAXSHORTLEN	40
#endif


/*
** Size of a short TString: Size of the header plus space for the string
** itself (including final '\0').
*/
#define sizestrshr(l)  \
	(offsetof(TString, contents) + ((l) + 1) * sizeof(char))


#define luaS_newliteral(L, s)	(luaS_newlstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1))


/*
** test whether a string is a reserved word
*/
#define isreserved(s)	(strisshr(s) && (s)->extra > 0)


/*
** equality for short strings, which are always internalized
*/
#define eqshrstr(a,b)	check_exp((a)->tt == ILYA_VSHRSTR, (a) == (b))


LUAI_FUNC unsigned luaS_hash (const char *str, size_t l, unsigned seed);
LUAI_FUNC unsigned luaS_hashlongstr (TString *ts);
LUAI_FUNC int luaS_eqlngstr (TString *a, TString *b);
LUAI_FUNC void luaS_resize (ilya_State *L, int newsize);
LUAI_FUNC void luaS_clearcache (global_State *g);
LUAI_FUNC void luaS_init (ilya_State *L);
LUAI_FUNC void luaS_remove (ilya_State *L, TString *ts);
LUAI_FUNC Udata *luaS_newudata (ilya_State *L, size_t s,
                                              unsigned short nuvalue);
LUAI_FUNC TString *luaS_newlstr (ilya_State *L, const char *str, size_t l);
LUAI_FUNC TString *luaS_new (ilya_State *L, const char *str);
LUAI_FUNC TString *luaS_createlngstrobj (ilya_State *L, size_t l);
LUAI_FUNC TString *luaS_newextlstr (ilya_State *L,
		const char *s, size_t len, ilya_Alloc falloc, void *ud);
LUAI_FUNC size_t luaS_sizelngstr (size_t len, int kind);

#endif
