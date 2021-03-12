#pragma once

#if !defined(WIN32_LEAN_AND_MEAN) 
#define WIN32_LEAN_AND_MEAN
#endif

#define RENDER_DEBUG
#define DX_SAFE_RELEASED(ptr) if(nullptr != ptr) {ptr->Release(); ptr = nullptr;}

#define INITGUID
#include "Thirdparty/Dx12Helper/d3d12.h"
#include <cguid.h>
#include <dxgi.h>   // shared library `used across multiple dx graphical interfaces
#include <dxgidebug.h>  // debug utility (mostly used for reporting and analytics)
#include <string>
//#include <dxgi1_4.h>
//#include <dxcapi.h>
//#pragma comment( lib, "dxcompiler.lib" )  
//#pragma comment( lib, "d3d12.lib" )      
//#pragma comment( lib, "dxgi.lib" )       

typedef unsigned int uint;