/*
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

/*
 * Purpose of this file :
 *
 * To hold COMPILER-SPECIFIC portion of STLport settings.
 * In general, user should not edit this file unless
 * using the compiler not recognized below.
 *
 * If your compiler is not being recognized yet,
 * please look for definitions of macros in stl_mycomp.h,
 * copy stl_mycomp.h to stl_YOUR_COMPILER_NAME,
 * adjust flags for your compiler, and add  <include config/stl_YOUR_COMPILER_NAME>
 * to the secton controlled by unique macro defined internaly by your compiler.
 *
 * To change user-definable settings, please edit <user_config.h>
 *
 */

#ifndef __stl_config__system_h
#define __stl_config__system_h

#if defined (__sun)
#  include <stlport/stl/config/_solaris.h>
#  if defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  elif defined (__SUNPRO_CC)
#    include <stlport/stl/config/_sunprocc.h>
/*
#  ifdef __KCC
#    include <stlport/stl/config/_kai.h>
#  endif
*/
#  elif defined (__APOGEE__)  /* Apogee 4.x */
#    include <stlport/stl/config/_apcc.h>
#  elif defined (__FCC_VERSION) /* Fujitsu Compiler, v4.0 assumed */
#    include <stlport/stl/config/_fujitsu.h>
#  endif
#elif defined (__hpux)
#  include <stlport/stl/config/_hpux.h>
#  if defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  elif defined (__HP_aCC)
#    include <stlport/stl/config/_hpacc.h>
#  endif
#elif defined (linux) || defined (__linux__)
#  include <stlport/stl/config/_linux.h>
/* Intel's icc define __GNUC__! */
#  if defined (__INTEL_COMPILER)
#    include <stlport/stl/config/_icc.h>
#  elif defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  endif
/*
#  ifdef __KCC
#    include <stlport/stl/config/_kai.h>
#  endif
*/
#elif defined (__FreeBSD__)
#  include <stlport/stl/config/_freebsd.h>
#  if defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  endif
#elif defined (__OpenBSD__)
#  include <stlport/stl/config/_openbsd.h>
#  if defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  endif
#elif defined (N_PLAT_NLM) /* Novell NetWare */
#  include <stlport/stl/config/_netware.h>
#  ifdef __MWERKS__ /* Metrowerks CodeWarrior */
#    include <stlport/stl/config/_mwccnlm.h>
#  endif
#elif defined (__sgi) /* IRIX? */
#  define _STLP_PLATFORM "SGI Irix"
#  if defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  else
#    include <stlport/stl/config/_sgi.h>
#  endif
#elif defined (__OS400__) /* AS/400 C++ */
#  define _STLP_PLATFORM "OS 400"
#  if defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  else
#    include <stlport/stl/config/_as400.h>
#  endif
#elif defined (_AIX)
#  include <stlport/stl/config/_aix.h>
#  if defined (__xlC__) || defined (__IBMC__) || defined ( __IBMCPP__ )
     /* AIX xlC, Visual Age C++ , OS-390 C++ */
#    include <stlport/stl/config/_ibm.h>
#  endif
#elif defined (_CRAY) /* Cray C++ 3.4 or 3.5 */
#  define _STLP_PLATFORM "Cray"
#  include <config/_cray.h>
#elif defined (__DECCXX) || defined (__DECC)
#  define _STLP_PLATFORM "DECC"
#  ifdef __vms
#    include <stlport/stl/config/_dec_vms.h>
#  else
#    include <stlport/stl/config/_dec.h>
#  endif
#elif defined (macintosh) || defined (_MAC)
#  include <stlport/stl/config/_mac.h>
#  if defined (__MWERKS__)
#    include <stlport/stl/config/_mwerks.h>
#  elif defined (__MRC__) || (defined (__SC__) && (__SC__ >= 0x882))
     /* Apple MPW SCpp 8.8.2, Apple MPW MrCpp 4.1.0 */
#    include <stlport/stl/config/_apple.h>
#  endif
#elif defined (__APPLE__)
#  include <stlport/stl/config/_macosx.h>
#  ifdef __GNUC__
#    include <stlport/stl/config/_gcc.h>
#  endif
#elif defined (__CYGWIN__)
#  include <stlport/stl/config/_cygwin.h>
#  if defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  endif
#elif defined (__MINGW32__)
#  define _STLP_PLATFORM "MinGW"
#  if defined (__GNUC__)
#    include <stlport/stl/config/_gcc.h>
#  endif
#  include <stlport/stl/config/_windows.h>
#elif defined (_WIN32) || defined (__WIN32) || defined (WIN32) || defined (__WIN32__) || \
      defined (__WIN16) || defined (WIN16) || defined (_WIN16)
#  if defined ( __BORLANDC__ )  /* Borland C++ ( 4.x - 5.x ) */
#    include <stlport/stl/config/_bc.h>
#  elif defined (__WATCOM_CPLUSPLUS__) || defined (__WATCOMC__)  /* Watcom C++ */
#    include <stlport/stl/config/_watcom.h>
#  elif defined (__COMO__) || defined (__COMO_VERSION_)
#    include <stlport/stl/config/_como.h>
#  elif defined (__DMC__)   /* Digital Mars C++ */
#    include <stlport/stl/config/_dm.h>
#  elif defined (__SC__) && (__SC__ < 0x800) /* Symantec 7.5 */
#    include <stlport/stl/config/_symantec.h>
#  elif defined (__ICL) /* Intel reference compiler for Win */
#    include <stlport/stl/config/_intel.h>
#  elif defined (__MWERKS__)
#    include <stlport/stl/config/_mwerks.h>
#  elif defined (_MSC_VER) && (_MSC_VER >= 1200) && defined (UNDER_CE)
     /* Microsoft eMbedded Visual C++ 3.0, 4.0 (.NET) */
#    include <stlport/stl/config/_evc.h>
#  elif defined (_MSC_VER)
    /* Microsoft Visual C++ 6.0, 7.0, 7.1, 8.0 */
#    include <stlport/stl/config/_msvc.h>
#    if defined (__GCCXML__)
#        define _STLP_CALL
#    endif
#  endif

#  include <stlport/stl/config/_windows.h>
#else
#  error Unknown platform !!
#endif

#if !defined (_STLP_COMPILER)
/* Unable to identify the compiler, issue error diagnostic.
 * Edit <config/stl_mycomp.h> to set STLport up for your compiler. */
#  include <stlport/stl/config/stl_mycomp.h>
#endif

#endif /* __stl_config__system_h */
