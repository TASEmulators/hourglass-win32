/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(D3D9HOOKS_INCL) //&& !defined(UNITY_BUILD)
#define D3D9HOOKS_INCL

#include "../../external/d3d9.h"
#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"

void FakeBroadcastDisplayChange(int width, int height, int depth);

DEFINE_LOCAL_GUID(IID_IDirect3DDevice9,0xd0223b96,0xbf7a,0x43fd,0x92,0xbd,0xa4,0x3b,0xd,0x92,0xb9,0xeb);
DEFINE_LOCAL_GUID(IID_IDirect3DSwapChain9,0x794950f2,0xadfc,0x458a,0x90,0x5e,0x10,0xa1,0xb,0xb,0x50,0x3b);

static IDirect3DDevice9* pBackBufCopyOwner = NULL;
static IDirect3DSurface9* pBackBufCopy = NULL;


struct MyDirect3DDevice9
{
	static BOOL Hook(IDirect3DDevice9* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DDevice9, Present);
		rv |= VTHOOKFUNC(IDirect3DDevice9, Reset);
		rv |= VTHOOKFUNC(IDirect3DDevice9, GetSwapChain);
		rv |= VTHOOKFUNC(IDirect3DDevice9, CreateAdditionalSwapChain);
		//rv |= VTHOOKFUNC(IDirect3DDevice9, SetRenderTarget);
		//rv |= VTHOOKFUNC(IDirect3DDevice9, GetRenderTarget);
		
/*
		rv |= VTHOOKFUNC(IDirect3DDevice9, CopyRects);
		rv |= VTHOOKFUNC(IDirect3DDevice9, UpdateTexture);
		rv |= VTHOOKFUNC(IDirect3DDevice9, BeginScene);
		rv |= VTHOOKFUNC(IDirect3DDevice9, EndScene);
		rv |= VTHOOKFUNC(IDirect3DDevice9, Clear);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawIndexedPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawPrimitiveUP);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawIndexedPrimitiveUP);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawRectPatch);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawTriPatch);
*/
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DDevice9* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DDevice9* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Reset)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters);
	static HRESULT STDMETHODCALLTYPE MyReset(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(pPresentationParameters)
			pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // back buffer must be lockable, to allow for AVI recording
		HRESULT rv = Reset(pThis, pPresentationParameters);
		return rv;
	}

	static void PresentFrameBoundary(IDirect3DDevice9* pThis)
	{
		// are we (not) recording AVI?
		BOOL recordingAVIVideo = (tasflags.aviMode & 1);
		if(!recordingAVIVideo)
		{
			// if not recording AVI, it's a regular frame boundary.
			FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
		}
		else
		{
			// if we are, it's still a regular frame boundary,
			// but we prepare extra info for the AVI capture around it.
			DDSURFACEDESC desc = { sizeof(DDSURFACEDESC) };
			IDirect3DSurface9* pBackBuffer;
#if 0 // slow
			Lock(pThis, desc, pBackBuffer);
			FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
			pBackBuffer->UnlockRect();
#else // MUCH faster. TODO: in d3d7 and earlier, use Blt or whatever to get an equivalent speedup there
	#ifdef _DEBUG
			DWORD time1 = timeGetTime();
	#endif
			pThis->GetRenderTarget(0, &pBackBuffer);
			if(pBackBufCopyOwner != pThis /*&& pBackBufCopy*/)
			{
				//pBackBufCopy->Release();
				pBackBufCopy = NULL;
			}

			IDirect3DSurface9* pSurface = pBackBufCopy;
			if(!pSurface)
			{
				D3DSURFACE_DESC d3ddesc;
				pBackBuffer->GetDesc(&d3ddesc);
				if(SUCCEEDED(pThis->CreateOffscreenPlainSurface(d3ddesc.Width, d3ddesc.Height, d3ddesc.Format, D3DPOOL_SYSTEMMEM, &pBackBufCopy, NULL)))
				{
					pSurface = pBackBufCopy;
					pBackBufCopyOwner = pThis;
				}
				else
				{
					// fallback
					pSurface = pBackBuffer;
				}
			}
			if(pSurface != pBackBuffer)
				if(FAILED(pThis->GetRenderTargetData(pBackBuffer,pSurface)))
					pSurface = pBackBuffer;
			Lock(pThis, desc, pSurface, false);
	#ifdef _DEBUG
			DWORD time2 = timeGetTime();
			debugprintf("AVI: pre-copying pixel data took %d ticks\n", (int)(time2-time1));
	#endif
			FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
			pSurface->UnlockRect();
#endif
		}
	}

	static HRESULT(STDMETHODCALLTYPE *Present)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DDevice9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");

		HRESULT rv;
		if(ShouldSkipDrawing(true, false))
			rv = D3D_OK;
		else
			rv = Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

		PresentFrameBoundary(pThis);

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetSwapChain)(IDirect3DDevice9* pThis, UINT iSwapChain,IDirect3DSwapChain9** pSwapChain);
	static HRESULT STDMETHODCALLTYPE MyGetSwapChain(IDirect3DDevice9* pThis, UINT iSwapChain,IDirect3DSwapChain9** pSwapChain)
	{
		d3ddebugprintf(__FUNCTION__ "(%d) called.\n", iSwapChain);
		HRESULT rv = GetSwapChain(pThis, iSwapChain, pSwapChain);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IDirect3DSwapChain9, (LPVOID*)pSwapChain);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateAdditionalSwapChain)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain);
	static HRESULT STDMETHODCALLTYPE MyCreateAdditionalSwapChain(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(pPresentationParameters)
			pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // back buffer must be lockable, to allow for AVI recording
		HRESULT rv = CreateAdditionalSwapChain(pThis, pPresentationParameters, pSwapChain);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IDirect3DSwapChain9, (LPVOID*)pSwapChain);
		return rv;
	}

	// DrawPrimitive
	//{
	//	if(ShouldSkipDrawing(false, true))
	//		return D3D_OK;
	//}

	//static HRESULT(STDMETHODCALLTYPE *SetRenderTarget)(IDirect3DDevice9* pThis, IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pNewZStencil);
	//static HRESULT STDMETHODCALLTYPE MySetRenderTarget(IDirect3DDevice9* pThis, IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pNewZStencil)
	//{
	//	d3ddebugprintf(__FUNCTION__ "(0x%X, 0x%X, 0x%X) called.\n", pThis, pRenderTarget, pNewZStencil);
	//	HRESULT rv = SetRenderTarget(pThis, pRenderTarget, pNewZStencil);
	//	return rv;
	//}

	//static HRESULT(STDMETHODCALLTYPE *GetRenderTarget)(IDirect3DDevice9* pThis, IDirect3DSurface9** ppRenderTarget);
	//static HRESULT STDMETHODCALLTYPE MyGetRenderTarget(IDirect3DDevice9* pThis, IDirect3DSurface9** ppRenderTarget)
	//{
	//	d3ddebugprintf(__FUNCTION__ "(0x%X, 0x%X, 0x%X) called.\n", pThis, ppRenderTarget);
	//	HRESULT rv = GetRenderTarget(pThis, ppRenderTarget);
	//	d3ddebugprintf(__FUNCTION__ " returned 0x%X.\n", (SUCCEEDED(rv) && ppRenderTarget) ? *ppRenderTarget : NULL);
	//	return rv;
	//}

	static void Lock(IDirect3DDevice9* pThis, DDSURFACEDESC& desc, IDirect3DSurface9*& pBackBuffer, bool getBackBuffer=true)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(getBackBuffer)
			pThis->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer);

		D3DSURFACE_DESC d3ddesc;
		pBackBuffer->GetDesc(&d3ddesc);

		D3DLOCKED_RECT lr;
		pBackBuffer->LockRect(&lr,0,D3DLOCK_NOSYSLOCK|D3DLOCK_READONLY|D3DLOCK_NO_DIRTY_UPDATE);

		desc.lpSurface = lr.pBits;
		desc.dwWidth = d3ddesc.Width;
		desc.dwHeight = d3ddesc.Height;
		desc.lPitch = lr.Pitch;
		switch(d3ddesc.Format)
		{
		default:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			desc.ddpfPixelFormat.dwRGBBitCount = 32;
			desc.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
			desc.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
			desc.ddpfPixelFormat.dwBBitMask = 0x000000FF;
			break;
		case D3DFMT_A1R5G5B5:
		case D3DFMT_X1R5G5B5:
			desc.ddpfPixelFormat.dwRGBBitCount = 16;
			desc.ddpfPixelFormat.dwRBitMask = 0x7C00;
			desc.ddpfPixelFormat.dwGBitMask = 0x03E0;
			desc.ddpfPixelFormat.dwBBitMask = 0x001F;
			break;
		case D3DFMT_R5G6B5:
			desc.ddpfPixelFormat.dwRGBBitCount = 16;
			desc.ddpfPixelFormat.dwRBitMask = 0xF900;
			desc.ddpfPixelFormat.dwGBitMask = 0x07E0;
			desc.ddpfPixelFormat.dwBBitMask = 0x001F;
			break;
		//case D3DFMT_A2R10G10B10:
		}
	}

/*

#undef AUTOMETHOD
#define AUTOMETHOD(x) \
	static HRESULT(STDMETHODCALLTYPE *x)(PARAMS); \
	static HRESULT STDMETHODCALLTYPE My##x(PARAMS) \
	{ \
		debugprintf(__FUNCTION__ "(0x%X) called.\n", pThis); \
		HRESULT rv = x(ARGS); \
		return rv; \
	}

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPointsArray
#define ARGS pThis,pSourceSurface,pSourceRectsArray,cRects,pDestinationSurface,pDestPointsArray
    AUTOMETHOD(CopyRects)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture
#define ARGS pThis,pSourceTexture,pDestinationTexture
    AUTOMETHOD(UpdateTexture)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis
#define ARGS pThis
    AUTOMETHOD(BeginScene)
    AUTOMETHOD(EndScene)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil
#define ARGS pThis,Count,pRects,Flags,Color,Z,Stencil
    AUTOMETHOD(Clear)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount
#define ARGS pThis,PrimitiveType,StartVertex,PrimitiveCount
    AUTOMETHOD(DrawPrimitive)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount
#define ARGS pThis,PrimitiveType,minIndex,NumVertices,startIndex,primCount
    AUTOMETHOD(DrawIndexedPrimitive)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride
#define ARGS pThis,PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride
    AUTOMETHOD(DrawPrimitiveUP)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride
#define ARGS pThis,PrimitiveType,MinVertexIndex,NumVertexIndices,PrimitiveCount,pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride
    AUTOMETHOD(DrawIndexedPrimitiveUP)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo
#define ARGS pThis,Handle,pNumSegs,pRectPatchInfo
    AUTOMETHOD(DrawRectPatch)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo
#define ARGS pThis,Handle,pNumSegs,pTriPatchInfo
    AUTOMETHOD(DrawTriPatch)

*/



};

HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::QueryInterface)(IDirect3DDevice9* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::Present)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::Reset)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::GetSwapChain)(IDirect3DDevice9* pThis, UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::CreateAdditionalSwapChain)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) = 0;
//HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::SetRenderTarget)(IDirect3DDevice9* pThis, IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pNewZStencil) = 0;
//HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::GetRenderTarget)(IDirect3DDevice9* pThis, IDirect3DSurface9** ppRenderTarget) = 0;

/*

#undef AUTOMETHOD
#define AUTOMETHOD(x) HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::x)(PARAMS) = 0;
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPointsArray
    AUTOMETHOD(CopyRects)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture
    AUTOMETHOD(UpdateTexture)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis
    AUTOMETHOD(BeginScene)
    AUTOMETHOD(EndScene)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil
    AUTOMETHOD(Clear)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount
    AUTOMETHOD(DrawPrimitive)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount
    AUTOMETHOD(DrawIndexedPrimitive)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride
    AUTOMETHOD(DrawPrimitiveUP)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride
    AUTOMETHOD(DrawIndexedPrimitiveUP)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo
    AUTOMETHOD(DrawRectPatch)
#undef PARAMS
#define PARAMS IDirect3DDevice9* pThis, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo
    AUTOMETHOD(DrawTriPatch)

*/


struct MyDirect3DSwapChain9
{
	static BOOL Hook(IDirect3DSwapChain9* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DSwapChain9, Present);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DSwapChain9* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DSwapChain9* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Present)(IDirect3DSwapChain9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags);
	static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DSwapChain9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");

		HRESULT rv;
		if(ShouldSkipDrawing(true, false))
			rv = D3D_OK;
		else
			rv = Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

		IDirect3DDevice9* pDevice;
		if(SUCCEEDED(pThis->GetDevice(&pDevice)))
			MyDirect3DDevice9::PresentFrameBoundary(pDevice);
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyDirect3DSwapChain9::QueryInterface)(IDirect3DSwapChain9* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DSwapChain9::Present)(IDirect3DSwapChain9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) = 0;




DEFINE_LOCAL_GUID(IID_IDirect3D9,0x81bdcbca,0x64d4,0x426d,0xae,0x8d,0xad,0x1,0x47,0xf4,0x27,0x5c);

struct MyDirect3D9
{
	static BOOL Hook(IDirect3D9* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3D9, CreateDevice);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3D9* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3D9* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateDevice)(IDirect3D9* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface);
	static HRESULT STDMETHODCALLTYPE MyCreateDevice(IDirect3D9* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		//BOOL wasWindowed;
		if(pPresentationParameters)
		{
			pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // back buffer must be lockable, to allow for AVI recording
			if(tasflags.forceWindowed && !pPresentationParameters->Windowed)
			{
				pPresentationParameters->Windowed = TRUE;
				pPresentationParameters->FullScreen_RefreshRateInHz = 0;
				extern HWND gamehwnd;
				
				fakeDisplayWidth = pPresentationParameters->BackBufferWidth;
				fakeDisplayHeight = pPresentationParameters->BackBufferHeight;
				fakePixelFormatBPP = (pPresentationParameters->BackBufferFormat == D3DFMT_R5G6B5 || pPresentationParameters->BackBufferFormat == D3DFMT_X1R5G5B5 || pPresentationParameters->BackBufferFormat == D3DFMT_A1R5G5B5) ? 16 : 32;
				fakeDisplayValid = TRUE;
				if(gamehwnd)
					MakeWindowWindowed(gamehwnd, fakeDisplayWidth, fakeDisplayHeight);
				
				D3DDISPLAYMODE display_mode;
				if(SUCCEEDED(pThis->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &display_mode)))
				{
					pPresentationParameters->BackBufferFormat = display_mode.Format;
					// in case
					pPresentationParameters->BackBufferWidth = display_mode.Width;
					pPresentationParameters->BackBufferHeight = display_mode.Height;
				}
				pPresentationParameters->BackBufferCount = 1;
			}
			else if(!tasflags.forceWindowed && !pPresentationParameters->Windowed)
			{
				FakeBroadcastDisplayChange(pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight, (pPresentationParameters->BackBufferFormat == D3DFMT_R5G6B5 || pPresentationParameters->BackBufferFormat == D3DFMT_X1R5G5B5 || pPresentationParameters->BackBufferFormat == D3DFMT_A1R5G5B5) ? 16 : 32);
			}
		}
		HRESULT rv = CreateDevice(pThis, Adapter,DeviceType,hFocusWindow,BehaviorFlags,pPresentationParameters,ppReturnedDeviceInterface);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IDirect3DDevice9, (LPVOID*)ppReturnedDeviceInterface);
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyDirect3D9::QueryInterface)(IDirect3D9* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3D9::CreateDevice)(IDirect3D9* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface) = 0;


HOOKFUNC IDirect3D9* WINAPI MyDirect3DCreate9(UINT SDKVersion)
{
	debuglog(LCF_D3D, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	IDirect3D9* rv = Direct3DCreate9(SDKVersion);
	if(SUCCEEDED(rv))
		HookCOMInterface(IID_IDirect3D9, (void**)&rv);
	curtls.callerisuntrusted--;
	return rv;
}


bool HookCOMInterfaceD3D9(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	if(!ppvOut)
		return true;

	switch(riid.Data1)
	{
		VTHOOKRIID3(IDirect3D9,MyDirect3D9);
		VTHOOKRIID3(IDirect3DDevice9,MyDirect3DDevice9);
		VTHOOKRIID3(IDirect3DSwapChain9,MyDirect3DSwapChain9);

		default: return false;
	}
	return true;
}

void ApplyD3D9Intercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, D3D9, Direct3DCreate9),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
