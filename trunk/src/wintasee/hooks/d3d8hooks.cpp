/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(D3D8HOOKS_INCL) && !defined(UNITY_BUILD)
#define D3D8HOOKS_INCL

#include "../../external/d3d8.h"
#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"
#include <map>

void FakeBroadcastDisplayChange(int width, int height, int depth);

DEFINE_LOCAL_GUID(IID_IDirect3DDevice8,0x7385e5df,0x8fe8,0x41d5,0x86,0xb6,0xd7,0xb4,0x85,0x47,0xb6,0xcf);
DEFINE_LOCAL_GUID(IID_IDirect3DSwapChain8,0x928c088b,0x76b9,0x4c6b,0xa5,0x36,0xa5,0x90,0x85,0x38,0x76,0xcd);

static IDirect3DDevice8* pBackBufCopyOwner = NULL;
static IDirect3DSurface8* pBackBufCopy = NULL;

std::map<IDirect3DSwapChain8*,IDirect3DDevice8*> d3d8SwapChainToDeviceMap;

static bool d3d8BackBufActive = true;
static bool d3d8BackBufDirty = true;

struct MyDirect3DDevice8
{
	static BOOL Hook(IDirect3DDevice8* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DDevice8, Present);
		rv |= VTHOOKFUNC(IDirect3DDevice8, Reset);
		rv |= VTHOOKFUNC(IDirect3DDevice8, CreateAdditionalSwapChain);
		rv |= VTHOOKFUNC(IDirect3DDevice8, SetRenderTarget);
		rv |= VTHOOKFUNC(IDirect3DDevice8, GetRenderTarget);

		rv |= VTHOOKFUNC(IDirect3DDevice8, CopyRects);
		//rv |= VTHOOKFUNC(IDirect3DDevice8, UpdateTexture);
//		rv |= VTHOOKFUNC(IDirect3DDevice8, BeginScene);
//		rv |= VTHOOKFUNC(IDirect3DDevice8, EndScene);
		rv |= VTHOOKFUNC(IDirect3DDevice8, Clear);
		rv |= VTHOOKFUNC(IDirect3DDevice8, DrawPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDevice8, DrawIndexedPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDevice8, DrawPrimitiveUP);
		rv |= VTHOOKFUNC(IDirect3DDevice8, DrawIndexedPrimitiveUP);
//		//rv |= VTHOOKFUNC(IDirect3DDevice8, DrawRectPatch);
//		//rv |= VTHOOKFUNC(IDirect3DDevice8, DrawTriPatch);

		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DDevice8* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DDevice8* pThis, REFIID riid, void** ppvObj)
	{
		/*d3d*/debugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Reset)(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters);
	static HRESULT STDMETHODCALLTYPE MyReset(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(pPresentationParameters)
			pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // back buffer must be lockable, to allow for AVI recording
		d3d8BackBufActive = true;
		d3d8BackBufDirty = true;
		HRESULT rv = Reset(pThis, pPresentationParameters);
		return rv;
	}

	static void PresentFrameBoundary(IDirect3DDevice8* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect)
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
			IDirect3DSurface8* pBackBuffer;
#if 0 // slow
			Lock(pThis, desc, pBackBuffer, pSourceRect);
			FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
			pBackBuffer->UnlockRect();
#else // MUCH faster.
	#ifdef _DEBUG
			DWORD time1 = timeGetTime();
	#endif
			pThis->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer);
			if(pBackBufCopyOwner != pThis /*&& pBackBufCopy*/)
			{
				//pBackBufCopy->Release();
				pBackBufCopy = NULL;
			}
			IDirect3DSurface8* pSurface = pBackBufCopy;
			if(!pSurface)
			{
				D3DSURFACE_DESC d3ddesc;
				pBackBuffer->GetDesc(&d3ddesc);
				if(SUCCEEDED(pThis->CreateImageSurface(d3ddesc.Width, d3ddesc.Height, d3ddesc.Format, &pBackBufCopy)))
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
				if(FAILED(CopyRects(pThis, pBackBuffer,pSourceRect,pSourceRect?1:0,pSurface,NULL)))
					pSurface = pBackBuffer;
			Lock(pThis, desc, pSurface, pSourceRect, false);
	#ifdef _DEBUG
			DWORD time2 = timeGetTime();
			debugprintf("AVI: pre-copying pixel data took %d ticks\n", (int)(time2-time1));
	#endif
			FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
			pSurface->UnlockRect();
#endif
		}
	}

	static HRESULT(STDMETHODCALLTYPE *Present)(IDirect3DDevice8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DDevice8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");

		HRESULT rv;
		if(ShouldSkipDrawing(d3d8BackBufDirty, !d3d8BackBufDirty))
			rv = D3D_OK;
		else
			rv = Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

		if(d3d8BackBufActive || d3d8BackBufDirty)
		{
			PresentFrameBoundary(pThis, pSourceRect, pDestRect);
			d3d8BackBufDirty = false;
		}

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateAdditionalSwapChain)(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain8** pSwapChain);
	static HRESULT STDMETHODCALLTYPE MyCreateAdditionalSwapChain(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain8** pSwapChain)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(pPresentationParameters)
			pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // back buffer must be lockable, to allow for AVI recording
		HRESULT rv = CreateAdditionalSwapChain(pThis, pPresentationParameters, pSwapChain);
		if(SUCCEEDED(rv))
		{
			HookCOMInterface(IID_IDirect3DSwapChain8, (LPVOID*)pSwapChain);
			if(pSwapChain)
				d3d8SwapChainToDeviceMap[*pSwapChain] = pThis;
		}
		return rv;
	}

	// DrawPrimitive
	//{
	//	if(ShouldSkipDrawing(false, true))
	//		return D3D_OK;
	//}

	static HRESULT(STDMETHODCALLTYPE *SetRenderTarget)(IDirect3DDevice8* pThis, IDirect3DSurface8* pRenderTarget,IDirect3DSurface8* pNewZStencil);
	static HRESULT STDMETHODCALLTYPE MySetRenderTarget(IDirect3DDevice8* pThis, IDirect3DSurface8* pRenderTarget,IDirect3DSurface8* pNewZStencil)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pRenderTarget);
		HRESULT rv = SetRenderTarget(pThis, pRenderTarget, pNewZStencil);
		IDirect3DSurface8* pBackBuffer;
		if(SUCCEEDED(pThis->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer)))
			d3d8BackBufActive = (pRenderTarget == pBackBuffer);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetRenderTarget)(IDirect3DDevice8* pThis, IDirect3DSurface8** ppRenderTarget);
	static HRESULT STDMETHODCALLTYPE MyGetRenderTarget(IDirect3DDevice8* pThis, IDirect3DSurface8** ppRenderTarget)
	{
		HRESULT rv = GetRenderTarget(pThis, ppRenderTarget);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called, returned 0x%X\n", ppRenderTarget, (SUCCEEDED(rv) && ppRenderTarget) ? *ppRenderTarget : NULL);
		return rv;
	}

	static void Lock(IDirect3DDevice8* pThis, DDSURFACEDESC& desc, IDirect3DSurface8*& pBackBuffer, CONST RECT* pSourceRect=NULL, bool getBackBuffer=true)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(getBackBuffer)
			pThis->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer);

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
			desc.ddpfPixelFormat.dwRBitMask = 0xF800;
			desc.ddpfPixelFormat.dwGBitMask = 0x07E0;
			desc.ddpfPixelFormat.dwBBitMask = 0x001F;
			break;
		//case D3DFMT_A2R10G10B10:
		}

		if(pSourceRect)
		{
			desc.dwWidth = min(desc.dwWidth, (DWORD)(pSourceRect->right - pSourceRect->left));
			desc.dwHeight = min(desc.dwHeight, (DWORD)(pSourceRect->bottom - pSourceRect->top));
		}
	}

    static HRESULT(STDMETHODCALLTYPE *CopyRects)(IDirect3DDevice8* pThis, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray);
    static HRESULT STDMETHODCALLTYPE MyCopyRects(IDirect3DDevice8* pThis, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = CopyRects(pThis, pSourceSurface,pSourceRectsArray,cRects,pDestinationSurface,pDestPointsArray);
		IDirect3DSurface8* pBuffer;
		if(!d3d8BackBufDirty && SUCCEEDED(pThis->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBuffer)) && pDestinationSurface == pBuffer)
			d3d8BackBufDirty = true;
		// NYI. maybe need to call PresentFrameBoundary in some cases (if pDestinationSurface can be the front buffer)
		//else if(SUCCEEDED(pThis->GetFrontBuffer(&pBuffer)) && pDestinationSurface == pBuffer)
		//	PresentFrameBoundary(pThis);
		return rv;
	}

 //   static HRESULT(STDMETHODCALLTYPE *BeginScene)(IDirect3DDevice8* pThis);
 //   static HRESULT STDMETHODCALLTYPE MyBeginScene(IDirect3DDevice8* pThis)
	//{
	//	d3ddebugprintf(__FUNCTION__ " called.\n");
	//	HRESULT rv = BeginScene(pThis);
	//	return rv;
	//}

 //   static HRESULT(STDMETHODCALLTYPE *EndScene)(IDirect3DDevice8* pThis);
 //   static HRESULT STDMETHODCALLTYPE MyEndScene(IDirect3DDevice8* pThis)
	//{
	//	d3ddebugprintf(__FUNCTION__ " called.\n");
	//	HRESULT rv = EndScene(pThis);
	//	return rv;
	//}

    static HRESULT(STDMETHODCALLTYPE *Clear)(IDirect3DDevice8* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
    static HRESULT STDMETHODCALLTYPE MyClear(IDirect3DDevice8* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = Clear(pThis, Count,pRects,Flags,Color,Z,Stencil);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawPrimitive)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
    static HRESULT STDMETHODCALLTYPE MyDrawPrimitive(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitive(pThis, PrimitiveType,StartVertex,PrimitiveCount);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawIndexedPrimitive)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE primitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount);
    static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitive(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE primitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitive(pThis, primitiveType,minIndex,NumVertices,startIndex,primCount);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawPrimitiveUP)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
    static HRESULT STDMETHODCALLTYPE MyDrawPrimitiveUP(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitiveUP(pThis, PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawIndexedPrimitiveUP)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
    static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitiveUP(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitiveUP(pThis, PrimitiveType,MinVertexIndex,NumVertexIndices,PrimitiveCount,pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
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
#define PARAMS IDirect3DDevice8* pThis, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray
#define ARGS pThis,pSourceSurface,pSourceRectsArray,cRects,pDestinationSurface,pDestPointsArray
    AUTOMETHOD(CopyRects)

//#undef ARGS
//#undef PARAMS
//#define PARAMS IDirect3DDevice8* pThis, IDirect3DBaseTexture8* pSourceTexture,IDirect3DBaseTexture8* pDestinationTexture
//#define ARGS pThis,pSourceTexture,pDestinationTexture
//    AUTOMETHOD(UpdateTexture)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis
#define ARGS pThis
    AUTOMETHOD(BeginScene)
    AUTOMETHOD(EndScene)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil
#define ARGS pThis,Count,pRects,Flags,Color,Z,Stencil
    AUTOMETHOD(Clear)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount
#define ARGS pThis,PrimitiveType,StartVertex,PrimitiveCount
    AUTOMETHOD(DrawPrimitive)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount
#define ARGS pThis,PrimitiveType,minIndex,NumVertices,startIndex,primCount
    AUTOMETHOD(DrawIndexedPrimitive)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride
#define ARGS pThis,PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride
    AUTOMETHOD(DrawPrimitiveUP)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride
#define ARGS pThis,PrimitiveType,MinVertexIndex,NumVertexIndices,PrimitiveCount,pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride
    AUTOMETHOD(DrawIndexedPrimitiveUP)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo
#define ARGS pThis,Handle,pNumSegs,pRectPatchInfo
    AUTOMETHOD(DrawRectPatch)

#undef ARGS
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo
#define ARGS pThis,Handle,pNumSegs,pTriPatchInfo
    AUTOMETHOD(DrawTriPatch)

*/



};

HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::QueryInterface)(IDirect3DDevice8* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::Present)(IDirect3DDevice8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::Reset)(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::CreateAdditionalSwapChain)(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain8** pSwapChain) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::SetRenderTarget)(IDirect3DDevice8* pThis, IDirect3DSurface8* pRenderTarget,IDirect3DSurface8* pNewZStencil) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::GetRenderTarget)(IDirect3DDevice8* pThis, IDirect3DSurface8** ppRenderTarget) = 0;

HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::CopyRects)(IDirect3DDevice8* pThis, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray) = 0;
//HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::BeginScene)(IDirect3DDevice8* pThis) = 0;
//HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::EndScene)(IDirect3DDevice8* pThis) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::DrawIndexedPrimitive)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE primitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::Clear)(IDirect3DDevice8* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::DrawPrimitive)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::DrawPrimitiveUP)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::DrawIndexedPrimitiveUP)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) = 0;


/*

#undef AUTOMETHOD
#define AUTOMETHOD(x) HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::x)(PARAMS) = 0;
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray
    AUTOMETHOD(CopyRects)
#undef PARAMS
//#define PARAMS IDirect3DDevice8* pThis, IDirect3DBaseTexture8* pSourceTexture,IDirect3DBaseTexture8* pDestinationTexture
//    AUTOMETHOD(UpdateTexture)
//#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis
    AUTOMETHOD(BeginScene)
    AUTOMETHOD(EndScene)
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil
    AUTOMETHOD(Clear)
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount
    AUTOMETHOD(DrawPrimitive)
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount
    AUTOMETHOD(DrawIndexedPrimitive)
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride
    AUTOMETHOD(DrawPrimitiveUP)
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride
    AUTOMETHOD(DrawIndexedPrimitiveUP)
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo
    AUTOMETHOD(DrawRectPatch)
#undef PARAMS
#define PARAMS IDirect3DDevice8* pThis, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo
    AUTOMETHOD(DrawTriPatch)

*/


struct MyDirect3DSwapChain8
{
	static BOOL Hook(IDirect3DSwapChain8* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DSwapChain8, Present);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DSwapChain8* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DSwapChain8* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Present)(IDirect3DSwapChain8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags);
	static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DSwapChain8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");

		HRESULT rv;
		if(ShouldSkipDrawing(true, false))
			rv = D3D_OK;
		else
			rv = Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

		IDirect3DDevice8* pDevice;
		//if(SUCCEEDED(pThis->GetDevice(&pDevice)))
		if(0 != (pDevice = d3d8SwapChainToDeviceMap[pThis]))
			MyDirect3DDevice8::PresentFrameBoundary(pDevice,pSourceRect,pDestRect);
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyDirect3DSwapChain8::QueryInterface)(IDirect3DSwapChain8* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DSwapChain8::Present)(IDirect3DSwapChain8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) = 0;





DEFINE_LOCAL_GUID(IID_IDirect3D8,0x1dd9e8da,0x1c77,0x4d40,0xb0,0xcf,0x98,0xfe,0xfd,0xff,0x95,0x12);

struct MyDirect3D8
{
	static BOOL Hook(IDirect3D8* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3D8, CreateDevice);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3D8* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3D8* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateDevice)(IDirect3D8* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice8** ppReturnedDeviceInterface);
	static HRESULT STDMETHODCALLTYPE MyCreateDevice(IDirect3D8* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice8** ppReturnedDeviceInterface)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		//BOOL wasWindowed;
		if(pPresentationParameters) // presentparams
		{
			pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // back buffer must be lockable, to allow for AVI recording (TODO: maybe that's not true anymore except if the usual method fails... if there's a significant performance penalty for this flag (which there might not be) then there should be an option to disable it)
			//wasWindowed = pPresentationParameters->Windowed;
			if(tasflags.forceWindowed && !pPresentationParameters->Windowed)
			{
				pPresentationParameters->Windowed = TRUE;
				pPresentationParameters->FullScreen_RefreshRateInHz = 0;
				pPresentationParameters->FullScreen_PresentationInterval = 0;

				fakeDisplayWidth = pPresentationParameters->BackBufferWidth;
				fakeDisplayHeight = pPresentationParameters->BackBufferHeight;
				fakePixelFormatBPP = (pPresentationParameters->BackBufferFormat == D3DFMT_R5G6B5 || pPresentationParameters->BackBufferFormat == D3DFMT_X1R5G5B5 || pPresentationParameters->BackBufferFormat == D3DFMT_A1R5G5B5) ? 16 : 32;
				fakeDisplayValid = TRUE;
				FakeBroadcastDisplayChange(fakeDisplayWidth,fakeDisplayHeight,fakePixelFormatBPP);
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
			HookCOMInterface(IID_IDirect3DDevice8, (LPVOID*)ppReturnedDeviceInterface);
		//if(pPresentationParameters)
		//	pPresentationParameters->Windowed = wasWindowed;
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyDirect3D8::QueryInterface)(IDirect3D8* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3D8::CreateDevice)(IDirect3D8* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice8** ppReturnedDeviceInterface) = 0;



HOOKFUNC IDirect3D8* WINAPI MyDirect3DCreate8(UINT SDKVersion)
{
	debuglog(LCF_D3D, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	IDirect3D8* rv = Direct3DCreate8(SDKVersion);
	if(SUCCEEDED(rv))
		HookCOMInterface(IID_IDirect3D8, (void**)&rv);
	curtls.callerisuntrusted--;
	return rv;
}


bool HookCOMInterfaceD3D8(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	if(!ppvOut)
		return true;

	switch(riid.Data1)
	{
		VTHOOKRIID3(IDirect3D8,MyDirect3D8);
		VTHOOKRIID3(IDirect3DDevice8,MyDirect3DDevice8);
		VTHOOKRIID3(IDirect3DSwapChain8,MyDirect3DSwapChain8);

		default: return false;
	}
	return true;
}

void ApplyD3D8Intercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, D3D8, Direct3DCreate8),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
