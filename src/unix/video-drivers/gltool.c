/**
 * gltool.c
 *
 * Copyright (C) 2001  Sven Goethel
 *
 * GNU Library General Public License 
 * as published by the Free Software Foundation
 *
 * http://www.gnu.org/copyleft/lgpl.html
 * General dynamical loading OpenGL (GL/GLU) support for:
 *
 *
 * <OS - System>          <#define>  commentary
 * -----------------------------------------------
 * GNU/Linux, Unices/X11  _X11_      (loads glx also)
 * Macinstosh OS9         _MAC_OS9_
 * Macinstosh OSX         _MAC_OSX_
 * Win32                  _WIN32_
 *
 */

#include "gltool.h"

#include "gl-disp-var.hc"
#include "glu-disp-var.hc"

#ifdef _X11_
	#include "glxtool.h"
#endif

#ifdef _WIN32_
	#include "wgltool.h"
#endif

static int _glLibsLoaded = 0;

#ifdef _WIN32_
  static HMODULE hDLL_OPENGL32 = 0;
  static HMODULE hDLL_OPENGLU32 = 0;
#endif

#ifdef _X11_
  static void *libHandleGLX=0;
  static void *libHandleGL=0;
  static void *libHandleGLU=0;
#endif

#ifdef _MAC_OS9_
  Ptr glLibMainAddr = 0;
  CFragConnectionID glLibConnectId = 0;
#endif

static int gl_begin_ctr;

const char * GLTOOL_USE_GLLIB  = "GLTOOL_USE_GLLIB";
const char * GLTOOL_USE_GLULIB = "GLTOOL_USE_GLULIB";

void LIBAPIENTRY print_gl_error (const char *msg, const char *file, int line, GLenum errorcode)
{
  if (errorcode != GL_NO_ERROR)
  {
    const char *errstr = (const char *) disp__gluErrorString (errorcode);
    if (errstr != 0)
      fprintf (stderr, "\n\n****\n**%s %s:%d>0x%X %s\n****\n", msg, file, line, errorcode, errstr);
    else
      fprintf (stderr, "\n\n****\n**%s %s:%d>0x%X <unknown>\n****\n", msg, file, line, errorcode);
  }
}

#ifdef _WIN32_
void LIBAPIENTRY check_wgl_error (HWND wnd, const char *file, int line)
{
  LPVOID lpMsgBuf;

  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		 0, GetLastError (), MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
		 (LPTSTR) & lpMsgBuf, 0, 0);	// Display the string.

  fprintf (stderr, "\n\n****\n** %s:%d>%s\n****\n", file, line, lpMsgBuf);

  // Free the buffer.
  LocalFree (lpMsgBuf);
}
#endif

void LIBAPIENTRY check_gl_error (const char *file, int line)
{
  print_gl_error("GLCHECK", file, line, disp__glGetError());
}

void LIBAPIENTRY __sglBegin(const char * file, int line, GLenum mode)
{
  	print_gl_error("GL-PreBEGIN-CHECK", file, line, disp__glGetError());

	if(gl_begin_ctr!=0)
	{
		printf("\n\n****\n** GL-BEGIN-ERROR %s:%d> glBegin was called %d times (reset now)\n****\n", 
			file, line, gl_begin_ctr);
		gl_begin_ctr=0;
	} else
		gl_begin_ctr++;

	disp__glBegin(mode);
}

void LIBAPIENTRY __sglEnd(const char * file, int line)
{
	if(gl_begin_ctr!=1)
	{
		printf("\n\n****\n** GL-END-ERROR %s:%d> glBegin was called %d times (reset now)\n****\n", 
			file, line, gl_begin_ctr);
		gl_begin_ctr=1;
	} else
		gl_begin_ctr--;

	disp__glEnd();

  	print_gl_error("GL-PostEND-CHECK", file, line, disp__glGetError());
}

/** 
 * call this function only if you 
 * are not within a glBegin/glEnd block,
 * or you think your are not ;-)
 */
void LIBAPIENTRY checkGlBeginEndBalance(const char *file, int line)
{
	if(gl_begin_ctr!=0)
	{
		printf("\n****\n** GL-BeginEnd-ERROR %s:%d> glBegin was called %d times\n****\n", 
			file, line, gl_begin_ctr);
	}
  	print_gl_error("GL-BeginEnd-CHECK", file, line, disp__glGetError());
}

void LIBAPIENTRY showGlBeginEndBalance(const char *file, int line)
{
	printf("\n****\n** GL-BeginEnd %s:%d> glBegin was called %d times\n****\n", 
		file, line, gl_begin_ctr);
}

int LIBAPIENTRY unloadGLLibrary ()
{
#ifdef _WIN32_
      if(hDLL_OPENGL32!=NULL)
      {
		FreeLibrary(hDLL_OPENGL32);
		hDLL_OPENGL32=NULL;
      }
      if(hDLL_OPENGLU32!=NULL)
      {
		FreeLibrary(hDLL_OPENGLU32);
		hDLL_OPENGLU32=NULL;
      }
#endif

#ifdef _X11_
      if(libHandleGL!=NULL)
      {
	      dlclose (libHandleGL);
	      libHandleGL = NULL;
      }
      if(libHandleGLU!=NULL)
      {
	      dlclose (libHandleGLU);
	      libHandleGLU = NULL;
      }
      if(libHandleGLX!=NULL)
      {
	      dlclose (libHandleGLX);
	      libHandleGLX = NULL;
      }
#endif

#ifdef _MAC_OS9_
      if (glLibConnectId!=NULL)
      {
	(void) CloseConnection(&glLibConnectId);
	glLibConnectId=0;
      }
      glLibMainAddr = 0;
#endif

      _glLibsLoaded = 0;
      return 1;

}

int LIBAPIENTRY loadGLLibrary (const char * libGLName, const char * libGLUName)
{
  const char *envGLName  = NULL;
  const char *envGLUName = NULL;
#ifdef _MAC_OS9_
  Str255 errName;
  OSErr returnError=fragNoErr;
#endif

  if(_glLibsLoaded) return 1;

  envGLName = getenv(GLTOOL_USE_GLLIB);
  envGLUName = getenv(GLTOOL_USE_GLULIB);

  if(envGLName!=NULL)
  {
  	libGLName = envGLName;
	printf("GLTOOL: using env's GLTOOL_USE_GLLIB = %s\n", libGLName);
  }

  if(envGLUName!=NULL)
  {
  	libGLUName = envGLUName;
	printf("GLTOOL: using env's GLTOOL_USE_GLULIB = %s\n", libGLUName);
  }

#ifdef _WIN32_
  if(hDLL_OPENGL32!=NULL) return 1;

  hDLL_OPENGL32 = LoadLibrary (libGLName);
  hDLL_OPENGLU32 = LoadLibrary (libGLUName);

  if (hDLL_OPENGL32 == NULL)
  {
      printf ("GLERROR: cannot access OpenGL library %s\n", libGLName);
      fflush (NULL);
      return 0;
  }

  if (hDLL_OPENGLU32 == NULL)
  {
      printf ("GLERROR: cannot access GLU library %s\n", libGLUName);
      fflush (NULL);
      return 0;
  }

#endif

#ifdef _X11_
  if(libHandleGL!=NULL) return 1;

#ifdef SUN_FORTE_DLOPEN_LIBCRUN
  {
     void *libcrun;

     libcrun = dlopen (SUN_FORTE_DLOPEN_LIBCRUN, RTLD_LAZY | RTLD_GLOBAL);
     if (libcrun == NULL)
     {
        printf ("GLERROR: cannot access library %s\n", SUN_FORTE_DLOPEN_LIBCRUN);
        printf ("GLERROR: dlerror() returns [%s]\n", dlerror());
        fflush (NULL);
        return 0;
     }
  }
#endif

  libHandleGL = dlopen (libGLName, RTLD_LAZY | RTLD_GLOBAL);
  if (libHandleGL == NULL)
  {
      printf ("GLERROR: cannot access OpenGL library %s\n", libGLName);
      printf ("GLERROR: dlerror() returns [%s]\n", dlerror());
      fflush (NULL);
      return 0;
  }

  libHandleGLU = dlopen (libGLUName, RTLD_LAZY | RTLD_GLOBAL);
  if (libHandleGLU == NULL)
  {
      printf ("GLERROR: cannot access GLU library %s\n", libGLUName);
      printf ("GLERROR: dlerror() returns [%s]\n", dlerror());
      fflush (NULL);
      return 0;
  }

  libHandleGLX = dlopen (GLXLIB_NAME, RTLD_LAZY | RTLD_GLOBAL);
  if (libHandleGLX == NULL)
  {
      printf ("GLINFO: cannot access GLX library %s directly ...\n", GLXLIB_NAME);
      printf ("GLERROR: dlerror() returns [%s]\n", dlerror());
      fflush (NULL);
  }

#endif

#ifdef _MAC_OS9_
        returnError =
                GetSharedLibrary("\pOpenGLLibrary",
                               kPowerPCCFragArch,
                               kReferenceCFrag,
                               &glLibConnectId,
                               &glLibMainAddr,
                   errName);
 
        if (returnError != fragNoErr)
        {
                printf ("GetSharedLibrary Err(%d): Ahhh!  Didn't find LIBRARY !\n",
                        returnError);
		return 0;
        }
#endif

#ifdef _MAC_OSX_
  printf ("GLINFO: loadGLLibrary - no special code implemented !\n");
#else
  printf ("GLINFO: loaded OpenGL library %s!\n", libGLName);
  printf ("GLINFO: loaded GLU    library %s!\n", libGLUName);
#endif
  fflush (NULL);
  
  _glLibsLoaded = 1;

  return 1;
}

void * LIBAPIENTRY getGLProcAddressHelper 
	(const char * libGLName, const char * libGLUName,
         const char *func, int *method, int debug, int verbose)
{
  void * funcPtr = NULL;
  int lmethod;

#ifdef _WIN32_
  static int __firstAccess = 1;

  if(!loadGLLibrary (libGLName, libGLUName))
  	return NULL;

  if (disp__wglGetProcAddress == NULL && __firstAccess)
  {
	  disp__wglGetProcAddress = ( PROC  (CALLBACK *)(LPCSTR) )
      	GetProcAddress (hDLL_OPENGL32, "wglGetProcAddress");

      if (disp__wglGetProcAddress != NULL /* && verbose */)
      {
			printf ("GLINFO: found wglGetProcAddress in %s\n", libGLName);
			fflush (NULL);
      }

      if (disp__wglGetProcAddress == NULL)
      {
	printf ("GLINFO: can't find wglGetProcAddress in %s\n", libGLName);
      }
  }
  __firstAccess = 0;

  if (disp__wglGetProcAddress != NULL)
	  funcPtr = disp__wglGetProcAddress (func);

  if (funcPtr == NULL)
  {
    lmethod = 2;

    funcPtr = GetProcAddress (hDLL_OPENGL32, func);
  }
  else
    lmethod = 1;

  if (funcPtr == NULL)
  {
    lmethod = 3;

    funcPtr = GetProcAddress (hDLL_OPENGLU32, func);
  }

#endif

#ifdef _X11_
  typedef void *(CALLBACK * procPtr) (const GLubyte *);

  /*
   * void (*glXGetProcAddressARB(const GLubyte *procName))
   */
  static int __firstAccess = 1;

  if(!loadGLLibrary (libGLName, libGLUName))
  	return NULL;

  if (disp__glXGetProcAddress == NULL && __firstAccess)
  {
      disp__glXGetProcAddress = (procPtr) dlsym (libHandleGL, SYMBOL_PREFIX
        "glXGetProcAddressARB");

      if (disp__glXGetProcAddress != NULL && verbose)
      {
	printf ("GLINFO: found glXGetProcAddressARB in %s\n", libGLName);
	fflush (NULL);
      }

      if (disp__glXGetProcAddress == NULL)
      {
	disp__glXGetProcAddress = (procPtr) dlsym (libHandleGL, SYMBOL_PREFIX
	  "glXGetProcAddressEXT");

	if (disp__glXGetProcAddress != NULL && verbose)
	{
	  printf ("GLINFO: found glXGetProcAddressEXT in %s\n", libGLName);
	  fflush (NULL);
	}
      }

      if (disp__glXGetProcAddress == NULL)
      {
	disp__glXGetProcAddress = (procPtr) dlsym (libHandleGL, SYMBOL_PREFIX
	  "glXGetProcAddress");

	if (disp__glXGetProcAddress != NULL && verbose)
	{
	  printf ("GLINFO: found glXGetProcAddress in %s\n", libGLName);
	  fflush (NULL);
	}
      }

      if (disp__glXGetProcAddress == NULL)
      {
	printf
	  ("GLINFO: cannot find glXGetProcAddress* in OpenGL library %s\n", libGLName);
	fflush (NULL);
	if (libHandleGLX != NULL)
	{
	  disp__glXGetProcAddress = (procPtr) dlsym (libHandleGLX, SYMBOL_PREFIX
	    "glXGetProcAddressARB");

	  if (disp__glXGetProcAddress != NULL && verbose)
	  {
	    printf ("GLINFO: found glXGetProcAddressARB in %s\n", GLXLIB_NAME);
	    fflush (NULL);
	  }

	  if (disp__glXGetProcAddress == NULL)
	  {
	    disp__glXGetProcAddress = (procPtr) dlsym (libHandleGLX,
	      SYMBOL_PREFIX "glXGetProcAddressEXT");

	    if (disp__glXGetProcAddress != NULL && verbose)
	    {
	      printf ("GLINFO: found glXGetProcAddressEXT in %s\n", GLXLIB_NAME);
	      fflush (NULL);
	    }
	  }

	  if (disp__glXGetProcAddress == NULL)
	  {
	    disp__glXGetProcAddress = (procPtr) dlsym (libHandleGLX,
	      SYMBOL_PREFIX "glXGetProcAddress");

	    if (disp__glXGetProcAddress != NULL && verbose)
	    {
	      printf ("GLINFO: found glXGetProcAddress in %s\n", GLXLIB_NAME);
	      fflush (NULL);
	    }
	  }
	  if (disp__glXGetProcAddress == NULL)
	  {
	    printf ("GLINFO: cannot find glXGetProcAddress* in GLX library %s\n", GLXLIB_NAME);
	    fflush (NULL);
	  }
	}
      }
  }
  __firstAccess = 0;

  if (disp__glXGetProcAddress != NULL)
    funcPtr = disp__glXGetProcAddress ((const unsigned char *) func);

  if (funcPtr == NULL)
  {
    lmethod = 2;
    funcPtr = dlsym (libHandleGL, func);
  }
  else
    lmethod = 1;

  if (funcPtr == NULL)
  {
    lmethod = 3;
    funcPtr = dlsym (libHandleGLU, func);
  }
#endif

#ifdef _MAC_OS9_
        Str255 errName;
        Str255 funcName;
        CFragSymbolClass glLibSymClass = 0;
        OSErr returnError=fragNoErr;
        #ifndef NDEBUG
		static int firstTime = 1;
        #endif
 
        if(!loadGLLibrary (libGLName, libGLUName))
  		return NULL;

	c2pstrcpy ( funcName, func );

	#ifndef NDEBUG
	 if(firstTime)
	 {
		PrintSymbolNamesByConnection (glLibConnectId);
		firstTime=0;
	 }
	 funcPtr = (void *)
		SeekSymbolNamesByConnection (glLibConnectId, funcName);
	#endif

	if(funcPtr==NULL)
	{
	  returnError =
	    FindSymbol (glLibConnectId, funcName,
			&funcPtr, & glLibSymClass );
	    lmethod=2;
	}
	#ifndef NDEBUG
	 else lmethod=3;
	#endif

	if (returnError != fragNoErr)
	{
	  printf ("GetSharedLibrary Err(%d): Ahhh!  Didn't find SYMBOL: %s !\n",
		returnError, func);
	}
#endif

#ifdef _MAC_OSX_
    char underscoreName[256];
    strcpy(underscoreName, "_");
    strcat(underscoreName, func);
 
    if ( NSIsSymbolNameDefined(underscoreName) ) {
        NSSymbol sym = NSLookupAndBindSymbol(underscoreName);
        funcPtr = (void *)NSAddressOfSymbol(sym);
    }
 
#endif

  if (funcPtr == NULL)
  {
    if (debug || verbose)
    {
      printf ("GLINFO: %s (%d): not implemented !\n", func, lmethod);
      fflush (NULL);
    }
  }
  else if (verbose)
  {
    printf ("GLINFO: %s (%d): loaded !\n", func, lmethod);
    fflush (NULL);
  }
  if (method != NULL)
    *method = lmethod;
  return funcPtr;
}


void LIBAPIENTRY fetch_GL_FUNCS (const char * libGLName, 
			         const char * libGLUName, int force)
{
  static int _firstRun = 1;

  if(force)
  {
	unloadGLLibrary();
        _firstRun = 1;
  }

  if(!_firstRun)
  	return;

  if(!loadGLLibrary (libGLName, libGLUName))
  	return;

  #define GET_GL_PROCADDRESS(a) getGLProcAddressHelper (libGLName, libGLUName, (SYMBOL_PREFIX a), NULL, 1, 0);

  #include "gl-disp-fetch.hc"
  #include "glu-disp-fetch.hc"

  _firstRun=0;

#ifdef _X11_
  fetch_GLX_FUNCS (libGLName, libGLUName, force);
#endif

#ifdef _WIN32_
  fetch_WGL_FUNCS (libGLName, libGLUName, force);
#endif

}

#ifdef _MAC_OS9_

#ifndef NDEBUG

static void PrintSymbolNamesByConnection (CFragConnectionID myConnID)
{
       long           myIndex;
       long           myCount;       /*number of exported symbols in fragment*/
       OSErr          myErr;
       Str255         myName;        /*symbol name*/
       Ptr            myAddr;        /*symbol address*/
       CFragSymbolClass       myClass;       /*symbol class*/
       static char buffer[256];

       myErr = CountSymbols(myConnID, &myCount);
       if (!myErr)
          for (myIndex = 1; myIndex <= myCount; myIndex++)
             {
                myErr = GetIndSymbol(myConnID, myIndex, myName, 
                                        &myAddr, &myClass);
                if (!myErr)
		{
			p2cstrcpy (buffer, myName);
								 
            printf("%d/%d: class %d - name %s\n", 
		   		myIndex, myCount, myClass, buffer);
		}
             }
}

static Ptr SeekSymbolNamesByConnection (CFragConnectionID myConnID, Str255 name)
{
       long           myIndex;
       long           myCount;       /*number of exported symbols in fragment*/
       OSErr          myErr;
       Str255         myName;        /*symbol name*/
       Ptr            myAddr;        /*symbol address*/
       CFragSymbolClass       myClass;       /*symbol class*/

       myErr = CountSymbols(myConnID, &myCount);
       if (!myErr)
          for (myIndex = 1; myIndex <= myCount; myIndex++)
             {
                myErr = GetIndSymbol(myConnID, myIndex, myName, 
                                        &myAddr, &myClass);
                if (!myErr && EqualString (myName, name, true, true) == true )
			return myAddr;
             }
}

#endif /* ifndef NDEBUG */

#endif /* ifdef _MAC_OS9_ */

