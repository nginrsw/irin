/*
** $Id: lfunc.c $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in ilya.h
*/

#define lfunc_c
#define ILYA_CORE

#include "lprefix.h"


#include <stddef.h>

#include "ilya.h"

#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"



CClosure *ilyaF_newCclosure (ilya_State *L, int nupvals) {
  GCObject *o = ilyaC_newobj(L, ILYA_VCCL, sizeCclosure(nupvals));
  CClosure *c = gco2ccl(o);
  c->nupvalues = cast_byte(nupvals);
  return c;
}


LClosure *ilyaF_newLclosure (ilya_State *L, int nupvals) {
  GCObject *o = ilyaC_newobj(L, ILYA_VLCL, sizeLclosure(nupvals));
  LClosure *c = gco2lcl(o);
  c->p = NULL;
  c->nupvalues = cast_byte(nupvals);
  while (nupvals--) c->upvals[nupvals] = NULL;
  return c;
}


/*
** fill a closure with new closed upvalues
*/
void ilyaF_initupvals (ilya_State *L, LClosure *cl) {
  int i;
  for (i = 0; i < cl->nupvalues; i++) {
    GCObject *o = ilyaC_newobj(L, ILYA_VUPVAL, sizeof(UpVal));
    UpVal *uv = gco2upv(o);
    uv->v.p = &uv->u.value;  /* make it closed */
    setnilvalue(uv->v.p);
    cl->upvals[i] = uv;
    ilyaC_objbarrier(L, cl, uv);
  }
}


/*
** Create a new upvalue at the given level, and link it to the list of
** open upvalues of 'L' after entry 'prev'.
**/
static UpVal *newupval (ilya_State *L, StkId level, UpVal **prev) {
  GCObject *o = ilyaC_newobj(L, ILYA_VUPVAL, sizeof(UpVal));
  UpVal *uv = gco2upv(o);
  UpVal *next = *prev;
  uv->v.p = s2v(level);  /* current value lives in the stack */
  uv->u.open.next = next;  /* link it to list of open upvalues */
  uv->u.open.previous = prev;
  if (next)
    next->u.open.previous = &uv->u.open.next;
  *prev = uv;
  if (!isintwups(L)) {  /* thread not in list of threads with upvalues? */
    L->twups = G(L)->twups;  /* link it to the list */
    G(L)->twups = L;
  }
  return uv;
}


/*
** Find and reuse, or create if it does not exist, an upvalue
** at the given level.
*/
UpVal *ilyaF_findupval (ilya_State *L, StkId level) {
  UpVal **pp = &L->openupval;
  UpVal *p;
  ilya_assert(isintwups(L) || L->openupval == NULL);
  while ((p = *pp) != NULL && uplevel(p) >= level) {  /* search for it */
    ilya_assert(!isdead(G(L), p));
    if (uplevel(p) == level)  /* corresponding upvalue? */
      return p;  /* return it */
    pp = &p->u.open.next;
  }
  /* not found: create a new upvalue after 'pp' */
  return newupval(L, level, pp);
}


/*
** Call closing method for object 'obj' with error message 'err'. The
** boolean 'yy' controls whether the call is yieldable.
** (This fn assumes EXTRA_STACK.)
*/
static void callclosemethod (ilya_State *L, TValue *obj, TValue *err, int yy) {
  StkId top = L->top.p;
  const TValue *tm = ilyaT_gettmbyobj(L, obj, TM_CLOSE);
  setobj2s(L, top, tm);  /* will call metamethod... */
  setobj2s(L, top + 1, obj);  /* with 'self' as the 1st argument */
  setobj2s(L, top + 2, err);  /* and error msg. as 2nd argument */
  L->top.p = top + 3;  /* add fn and arguments */
  if (yy)
    ilyaD_call(L, top, 0);
  else
    ilyaD_callnoyield(L, top, 0);
}


/*
** Check whether object at given level has a close metamethod and raise
** an error if not.
*/
static void checkclosemth (ilya_State *L, StkId level) {
  const TValue *tm = ilyaT_gettmbyobj(L, s2v(level), TM_CLOSE);
  if (ttisnil(tm)) {  /* no metamethod? */
    int idx = cast_int(level - L->ci->func.p);  /* variable index */
    const char *vname = ilyaG_findlocal(L, L->ci, idx, NULL);
    if (vname == NULL) vname = "?";
    ilyaG_runerror(L, "variable '%s' got a non-closable value", vname);
  }
}


/*
** Prepare and call a closing method.
** If status is CLOSEKTOP, the call to the closing method will be pushed
** at the top of the stack. Otherwise, values can be pushed right after
** the 'level' of the upvalue being closed, as everything after that
** won't be used again.
*/
static void prepcallclosemth (ilya_State *L, StkId level, int status, int yy) {
  TValue *uv = s2v(level);  /* value being closed */
  TValue *errobj;
  if (status == CLOSEKTOP)
    errobj = &G(L)->nilvalue;  /* error object is nil */
  else {  /* 'ilyaD_seterrorobj' will set top to level + 2 */
    errobj = s2v(level + 1);  /* error object goes after 'uv' */
    ilyaD_seterrorobj(L, status, level + 1);  /* set error object */
  }
  callclosemethod(L, uv, errobj, yy);
}


/*
** Maximum value for deltas in 'tbclist', dependent on the type
** of delta. (This macro assumes that an 'L' is in scope where it
** is used.)
*/
#define MAXDELTA  \
	((256ul << ((sizeof(L->stack.p->tbclist.delta) - 1) * 8)) - 1)


/*
** Insert a variable in the list of to-be-closed variables.
*/
void ilyaF_newtbcupval (ilya_State *L, StkId level) {
  ilya_assert(level > L->tbclist.p);
  if (l_isfalse(s2v(level)))
    return;  /* false doesn't need to be closed */
  checkclosemth(L, level);  /* value must have a close method */
  while (cast_uint(level - L->tbclist.p) > MAXDELTA) {
    L->tbclist.p += MAXDELTA;  /* create a dummy node at maximum delta */
    L->tbclist.p->tbclist.delta = 0;
  }
  level->tbclist.delta = cast(unsigned short, level - L->tbclist.p);
  L->tbclist.p = level;
}


void ilyaF_unlinkupval (UpVal *uv) {
  ilya_assert(upisopen(uv));
  *uv->u.open.previous = uv->u.open.next;
  if (uv->u.open.next)
    uv->u.open.next->u.open.previous = uv->u.open.previous;
}


/*
** Close all upvalues up to the given stack level.
*/
void ilyaF_closeupval (ilya_State *L, StkId level) {
  UpVal *uv;
  StkId upl;  /* stack index pointed by 'uv' */
  while ((uv = L->openupval) != NULL && (upl = uplevel(uv)) >= level) {
    TValue *slot = &uv->u.value;  /* new position for value */
    ilya_assert(uplevel(uv) < L->top.p);
    ilyaF_unlinkupval(uv);  /* remove upvalue from 'openupval' list */
    setobj(L, slot, uv->v.p);  /* move value to upvalue slot */
    uv->v.p = slot;  /* now current value lives here */
    if (!iswhite(uv)) {  /* neither white nor dead? */
      nw2black(uv);  /* closed upvalues cannot be gray */
      ilyaC_barrier(L, uv, slot);
    }
  }
}


/*
** Remove first element from the tbclist plus its dummy nodes.
*/
static void poptbclist (ilya_State *L) {
  StkId tbc = L->tbclist.p;
  ilya_assert(tbc->tbclist.delta > 0);  /* first element cannot be dummy */
  tbc -= tbc->tbclist.delta;
  while (tbc > L->stack.p && tbc->tbclist.delta == 0)
    tbc -= MAXDELTA;  /* remove dummy nodes */
  L->tbclist.p = tbc;
}


/*
** Close all upvalues and to-be-closed variables up to the given stack
** level. Return restored 'level'.
*/
StkId ilyaF_close (ilya_State *L, StkId level, int status, int yy) {
  ptrdiff_t levelrel = savestack(L, level);
  ilyaF_closeupval(L, level);  /* first, close the upvalues */
  while (L->tbclist.p >= level) {  /* traverse tbc's down to that level */
    StkId tbc = L->tbclist.p;  /* get variable index */
    poptbclist(L);  /* remove it from list */
    prepcallclosemth(L, tbc, status, yy);  /* close variable */
    level = restorestack(L, levelrel);
  }
  return level;
}


Proto *ilyaF_newproto (ilya_State *L) {
  GCObject *o = ilyaC_newobj(L, ILYA_VPROTO, sizeof(Proto));
  Proto *f = gco2p(o);
  f->k = NULL;
  f->sizek = 0;
  f->p = NULL;
  f->sizep = 0;
  f->code = NULL;
  f->sizecode = 0;
  f->lineinfo = NULL;
  f->sizelineinfo = 0;
  f->abslineinfo = NULL;
  f->sizeabslineinfo = 0;
  f->upvalues = NULL;
  f->sizeupvalues = 0;
  f->numparams = 0;
  f->flag = 0;
  f->maxstacksize = 0;
  f->locvars = NULL;
  f->sizelocvars = 0;
  f->linedefined = 0;
  f->lastlinedefined = 0;
  f->source = NULL;
  return f;
}


lu_mem ilyaF_protosize (Proto *p) {
  lu_mem sz = cast(lu_mem, sizeof(Proto))
            + cast_uint(p->sizep) * sizeof(Proto*)
            + cast_uint(p->sizek) * sizeof(TValue)
            + cast_uint(p->sizelocvars) * sizeof(LocVar)
            + cast_uint(p->sizeupvalues) * sizeof(Upvaldesc);
  if (!(p->flag & PF_FIXED)) {
    sz += cast_uint(p->sizecode) * sizeof(Instruction);
    sz += cast_uint(p->sizelineinfo) * sizeof(lu_byte);
    sz += cast_uint(p->sizeabslineinfo) * sizeof(AbsLineInfo);
  }
  return sz;
}


void ilyaF_freeproto (ilya_State *L, Proto *f) {
  if (!(f->flag & PF_FIXED)) {
    ilyaM_freearray(L, f->code, cast_sizet(f->sizecode));
    ilyaM_freearray(L, f->lineinfo, cast_sizet(f->sizelineinfo));
    ilyaM_freearray(L, f->abslineinfo, cast_sizet(f->sizeabslineinfo));
  }
  ilyaM_freearray(L, f->p, cast_sizet(f->sizep));
  ilyaM_freearray(L, f->k, cast_sizet(f->sizek));
  ilyaM_freearray(L, f->locvars, cast_sizet(f->sizelocvars));
  ilyaM_freearray(L, f->upvalues, cast_sizet(f->sizeupvalues));
  ilyaM_free(L, f);
}


/*
** Look for n-th lock variable at line 'line' in fn 'func'.
** Returns NULL if not found.
*/
const char *ilyaF_getlocalname (const Proto *f, int local_number, int pc) {
  int i;
  for (i = 0; i<f->sizelocvars && f->locvars[i].startpc <= pc; i++) {
    if (pc < f->locvars[i].endpc) {  /* is variable active? */
      local_number--;
      if (local_number == 0)
        return getstr(f->locvars[i].varname);
    }
  }
  return NULL;  /* not found */
}

