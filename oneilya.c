/*
** Ilya core, libraries, and interpreter in a single file.
** Compiling just this file generates a complete Ilya stand-alone
** program:
**
** $ gcc -O2 -std=c99 -o ilya oneilya.c -lm
**
** or
**
** $ gcc -O2 -std=c89 -DILYA_USE_C89 -o ilya oneilya.c -lm
**
*/

/* default is to build the full interpreter */
#ifndef MAKE_LIB
#ifndef MAKE_ILYAC
#ifndef MAKE_ILYA
#define MAKE_ILYA
#endif
#endif
#endif


/*
** Choose suitable platform-specific features. Default is no
** platform-specific features. Some of these options may need extra
** libraries such as -ldl -lreadline -lncurses
*/
#if 0
#define ILYA_USE_LINUX
#define ILYA_USE_MACOSX
#define ILYA_USE_POSIX
#define ILYA_ANSI
#endif


/* no need to change anything below this line ----------------------------- */

#include "lprefix.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* setup for ilyaconf.h */
#define ILYA_CORE
#define ILYA_LIB
#define ltable_c
#define lvm_c
#include "ilyaconf.h"

/* do not export internal symbols */
#undef ILYAI_FUNC
#undef ILYAI_DDEC
#undef ILYAI_DDEF
#define ILYAI_FUNC	static
#define ILYAI_DDEC(def)	/* empty */
#define ILYAI_DDEF	static

/* core -- used by all */
#include "lzio.c"
#include "lctype.c"
#include "lopcodes.c"
#include "lmem.c"
#include "lundump.c"
#include "ldump.c"
#include "lstate.c"
#include "lgc.c"
#include "llex.c"
#include "lcode.c"
#include "lparser.c"
#include "ldebug.c"
#include "lfunc.c"
#include "lobject.c"
#include "ltm.c"
#include "lstring.c"
#include "ltable.c"
#include "ldo.c"
#include "lvm.c"
#include "lapi.c"

/* auxiliary library -- used by all */
#include "lauxlib.c"

/* standard library  -- not used by ilyac */
#ifndef MAKE_ILYAC
#include "lbaselib.c"
#include "lcorolib.c"
#include "ldblib.c"
#include "liolib.c"
#include "lmathlib.c"
#include "loadlib.c"
#include "loslib.c"
#include "lstrlib.c"
#include "ltablib.c"
#include "lutf8lib.c"
#include "linit.c"
#endif

/* ilya */
#ifdef MAKE_ILYA
#include "ilya.c"
#endif

/* ilyac */
#ifdef MAKE_ILYAC
#include "ilyac.c"
#endif
