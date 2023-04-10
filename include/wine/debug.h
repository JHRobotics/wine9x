/*
 * Wine debugging interface
 *
 * Copyright 1999 Patrik Stridvall
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#ifndef __WINE_WINE_DEBUG_H
#define __WINE_WINE_DEBUG_H

#include <stdarg.h>
#ifndef VBOX_WINE_WITHOUT_LIBWINE
#include <windef.h>
#else
# include <iprt/win/windows.h>
#endif
#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

#ifdef __WINE_WINE_TEST_H
#error This file should not be used in Wine tests
#endif

#ifdef VBOX_WINE_WITH_IPRT
# include <iprt/assert.h>
//#else
//# define AssertBreakpoint() do { } while (0)
//# define Assert(_expr) do { } while (0)
//# ifdef DEBUG_misha
//#  include <iprt/cdefs.h>
//# endif
#endif
#ifdef __cplusplus
extern "C" {
#endif

struct _GUID;

#if !defined(__cplusplus) && !defined(inline)
# define inline _inline
#endif


/*
 * Internal definitions (do not use these directly)
 */

enum __wine_debug_class
{
    __WINE_DBCL_FIXME,
    __WINE_DBCL_ERR,
    __WINE_DBCL_WARN,
    __WINE_DBCL_TRACE,

//    __WINE_DBCL_INIT = 7  /* lazy init flag */
    __WINE_DBCL_INIT = 15  /* flags with trace */
};

struct __wine_debug_channel
{
    unsigned char flags;
    /*char name[15];*/
    char name[16];
};

void wine_debug_logs_init(const char *logname);
void wine_debug_logs_close();

#ifdef VBOX_WINE_BEAK_ON_TRACE
extern DWORD g_VDbgBreakOnTrace;
#endif
#ifndef WINE_NO_TRACE_MSGS
# ifdef VBOX_WINE_BEAK_ON_TRACE
#  define __WINE_GET_DEBUGGING_TRACE(dbch) ((g_VDbgBreakOnTrace ? (RT_BREAKPOINT(),NULL) : NULL), ((dbch)->flags & (1 << __WINE_DBCL_TRACE)))
# else
# define __WINE_GET_DEBUGGING_TRACE(dbch) ((dbch)->flags & (1 << __WINE_DBCL_TRACE))
# endif
#else
# define __WINE_GET_DEBUGGING_TRACE(dbch) 0
#endif

#ifndef WINE_NO_DEBUG_MSGS
# define __WINE_GET_DEBUGGING_WARN(dbch)  ((dbch)->flags & (1 << __WINE_DBCL_WARN))
# define __WINE_GET_DEBUGGING_FIXME(dbch) ((dbch)->flags & (1 << __WINE_DBCL_FIXME))
#else
# define __WINE_GET_DEBUGGING_WARN(dbch)  0
# define __WINE_GET_DEBUGGING_FIXME(dbch) 0
#endif

/* define error macro regardless of what is configured */
#define __WINE_GET_DEBUGGING_ERR(dbch)  ((dbch)->flags & (1 << __WINE_DBCL_ERR))

#define __WINE_GET_DEBUGGING(dbcl,dbch)  __WINE_GET_DEBUGGING##dbcl(dbch)

#define __WINE_IS_DEBUG_ON(dbcl,dbch) \
  (__WINE_GET_DEBUGGING##dbcl(dbch) && (__wine_dbg_get_channel_flags(dbch) & (1 << __WINE_DBCL##dbcl)))

#ifdef __GNUC__

#define __WINE_DPRINTF(dbcl,dbch) \
  do { if(__WINE_GET_DEBUGGING(dbcl,(dbch))) { \
       struct __wine_debug_channel * const __dbch = (dbch); \
       const enum __wine_debug_class __dbcl = __WINE_DBCL##dbcl; \
       __WINE_DBG_LOG

#define __WINE_DBG_LOG(args...) \
    wine_dbg_log( __dbcl, __dbch, __FUNCTION__, args); } } while(0)

#define __WINE_PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))


#ifdef WINE_NO_TRACE_MSGS
#define WINE_TRACE(args...) do { } while(0)
#define WINE_TRACE_(ch) WINE_TRACE
#endif

#ifdef WINE_NO_DEBUG_MSGS
#define WINE_WARN(args...) do { } while(0)
#define WINE_WARN_(ch) WINE_WARN
#define WINE_FIXME(args...) do { } while(0)
#define WINE_FIXME_(ch) WINE_FIXME
#endif

#elif defined(__SUNPRO_C)

#define __WINE_DPRINTF(dbcl,dbch) \
  do { if(__WINE_GET_DEBUGGING(dbcl,(dbch))) { \
       struct __wine_debug_channel * const __dbch = (dbch); \
       const enum __WINE_DEBUG_CLASS __dbcl = __WINE_DBCL##dbcl; \
       __WINE_DBG_LOG

#define __WINE_DBG_LOG(...) \
   wine_dbg_log( __dbcl, __dbch, __func__, __VA_ARGS__); } } while(0)

#define __WINE_PRINTF_ATTR(fmt,args)

#ifdef WINE_NO_TRACE_MSGS
#define WINE_TRACE(...) do { } while(0)
#define WINE_TRACE_(ch) WINE_TRACE
#endif

#ifdef WINE_NO_DEBUG_MSGS
#define WINE_WARN(...) do { } while(0)
#define WINE_WARN_(ch) WINE_WARN
#define WINE_FIXME(...) do { } while(0)
#define WINE_FIXME_(ch) WINE_FIXME
#endif

#else  /* !__GNUC__ && !__SUNPRO_C */

#define __WINE_DPRINTF(dbcl,dbch) \
    (!__WINE_GET_DEBUGGING(dbcl,(dbch)) || \
     (wine_dbg_log(__WINE_DBCL##dbcl,(dbch),__FILE__,"%d: ",__LINE__) == -1)) ? \
     (void)0 : (void)wine_dbg_printf

#define __WINE_PRINTF_ATTR(fmt, args)

#endif  /* !__GNUC__ && !__SUNPRO_C */

struct __wine_debug_functions
{
    char * (*get_temp_buffer)( size_t n );
    void   (*release_temp_buffer)( char *buffer, size_t n );
    const char * (*dbgstr_an)( const char * s, int n );
    const char * (*dbgstr_wn)( const WCHAR *s, int n );
    int (*dbg_vprintf)( const char *format, va_list args );
    int (*dbg_vlog)( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                     const char *function, const char *format, va_list args );
};

extern unsigned char __wine_dbg_get_channel_flags( struct __wine_debug_channel *channel );
extern int __wine_dbg_set_channel_flags( struct __wine_debug_channel *channel,
                                         unsigned char set, unsigned char clear );
extern void __wine_dbg_set_functions( const struct __wine_debug_functions *new_funcs,
                                      struct __wine_debug_functions *old_funcs, size_t size );

/*
 * Exported definitions and macros
 */

/* These functions return a printable version of a string, including
   quotes.  The string will be valid for some time, but not indefinitely
   as strings are re-used.  */
extern const char *wine_dbgstr_an( const char * s, int n );
extern const char *wine_dbgstr_wn( const WCHAR *s, int n );
extern const char *wine_dbg_sprintf( const char *format, ... ) __WINE_PRINTF_ATTR(1,2);

extern int wine_dbg_printf( const char *format, ... ) __WINE_PRINTF_ATTR(1,2);
extern int wine_dbg_log( enum __wine_debug_class cls, struct __wine_debug_channel *ch, const char *func,
                         const char *format, ... ) __WINE_PRINTF_ATTR(4,5);

static inline const char *wine_dbgstr_a( const char *s )
{
    return wine_dbgstr_an( s, -1 );
}

static inline const char *wine_dbgstr_w( const WCHAR *s )
{
    return wine_dbgstr_wn( s, -1 );
}

static inline const char *wine_dbgstr_guid( const GUID *id )
{
    if (!id) return "(null)";
    if (!((ULONG_PTR)id >> 16)) return wine_dbg_sprintf( "<guid-0x%04hx>", (WORD)(ULONG_PTR)id );
    return wine_dbg_sprintf( "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                             id->Data1, id->Data2, id->Data3,
                             id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                             id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
}

static inline const char *wine_dbgstr_point( const POINT *pt )
{
    if (!pt) return "(null)";
    return wine_dbg_sprintf( "(%d,%d)", pt->x, pt->y );
}

static inline const char *wine_dbgstr_size( const SIZE *size )
{
    if (!size) return "(null)";
    return wine_dbg_sprintf( "(%d,%d)", size->cx, size->cy );
}

static inline const char *wine_dbgstr_rect( const RECT *rect )
{
    if (!rect) return "(null)";
    return wine_dbg_sprintf( "(%d,%d)-(%d,%d)", rect->left, rect->top,
                             rect->right, rect->bottom );
}

static inline const char *wine_dbgstr_longlong( ULONGLONG ll )
{
    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        return wine_dbg_sprintf( "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll );
    else return wine_dbg_sprintf( "%lx", (unsigned long)ll );
}

#ifndef WINE_TRACE
#define WINE_TRACE                 __WINE_DPRINTF(_TRACE,__wine_dbch___default)
#define WINE_TRACE_(ch)            __WINE_DPRINTF(_TRACE,&__wine_dbch_##ch)
#endif
#define WINE_TRACE_ON(ch)          __WINE_IS_DEBUG_ON(_TRACE,&__wine_dbch_##ch)

#ifndef WINE_WARN
#define WINE_WARN                  __WINE_DPRINTF(_WARN,__wine_dbch___default)
#define WINE_WARN_(ch)             __WINE_DPRINTF(_WARN,&__wine_dbch_##ch)
#endif
#define WINE_WARN_ON(ch)           __WINE_IS_DEBUG_ON(_WARN,&__wine_dbch_##ch)

#ifndef WINE_FIXME
#define WINE_FIXME                 __WINE_DPRINTF(_FIXME,__wine_dbch___default)
#define WINE_FIXME_(ch)            __WINE_DPRINTF(_FIXME,&__wine_dbch_##ch)
#endif
#define WINE_FIXME_ON(ch)          __WINE_IS_DEBUG_ON(_FIXME,&__wine_dbch_##ch)

#define WINE_ERR                   __WINE_DPRINTF(_ERR,__wine_dbch___default)
#define WINE_ERR_(ch)              __WINE_DPRINTF(_ERR,&__wine_dbch_##ch)
#define WINE_ERR_ON(ch)            __WINE_IS_DEBUG_ON(_ERR,&__wine_dbch_##ch)

#define WINE_DECLARE_DEBUG_CHANNEL(ch) \
    static struct __wine_debug_channel __wine_dbch_##ch = { /*was: ~0, VBox:*/ UINT8_MAX, #ch }
#define WINE_DEFAULT_DEBUG_CHANNEL(ch) \
    static struct __wine_debug_channel __wine_dbch_##ch = { /*was: ~0, VBox:*/ UINT8_MAX, #ch }; \
    static struct __wine_debug_channel * const __wine_dbch___default = &__wine_dbch_##ch

#define WINE_DPRINTF               wine_dbg_printf
#define WINE_MESSAGE               wine_dbg_printf

#ifdef __WINESRC__
/* Wine uses shorter names that are very likely to conflict with other software */

static inline const char *debugstr_an( const char * s, int n ) { return wine_dbgstr_an( s, n ); }
static inline const char *debugstr_wn( const WCHAR *s, int n ) { return wine_dbgstr_wn( s, n ); }
static inline const char *debugstr_guid( const struct _GUID *id ) { return wine_dbgstr_guid(id); }
static inline const char *debugstr_a( const char *s )  { return wine_dbgstr_an( s, -1 ); }
static inline const char *debugstr_w( const WCHAR *s ) { return wine_dbgstr_wn( s, -1 ); }

#define TRACE                      WINE_TRACE
#define TRACE_(ch)                 WINE_TRACE_(ch)
#define TRACE_ON(ch)               WINE_TRACE_ON(ch)

#define WARN                       WINE_WARN
#define WARN_(ch)                  WINE_WARN_(ch)
#define WARN_ON(ch)                WINE_WARN_ON(ch)

#define FIXME                      WINE_FIXME
#define FIXME_(ch)                 WINE_FIXME_(ch)
#define FIXME_ON(ch)               WINE_FIXME_ON(ch)

#undef ERR  /* Solaris got an 'ERR' define in <sys/reg.h> */
#define ERR                        WINE_ERR
#define ERR_(ch)                   WINE_ERR_(ch)
#define ERR_ON(ch)                 WINE_ERR_ON(ch)

#define DPRINTF                    WINE_DPRINTF
#define MESSAGE                    WINE_MESSAGE

#endif /* __WINESRC__ */

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINE_DEBUG_H */
