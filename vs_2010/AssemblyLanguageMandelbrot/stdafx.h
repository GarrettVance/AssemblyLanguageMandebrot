// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <WinCon.h>
#include <iostream>
#include <vector>

#include <io.h>    // for console;
#include <fcntl.h>   // for console; 


// CRT's memory leak detection
#if defined(DEBUG) | defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <string>

#include <d3d9.h>
#include <d3dx9tex.h>
#include <dxerr.h>



