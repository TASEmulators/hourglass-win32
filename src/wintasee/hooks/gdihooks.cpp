/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(GDIHOOKS_INCL) && !defined(UNITY_BUILD)
#define GDIHOOKS_INCL


#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"
#include "../../shared/winutil.h"
#include <windowsx.h>

#include "../phasedetection.h"
static PhaseDetector s_gdiPhaseDetector;



// TODO: declare these in headers like openglhooks.h?
// I don't like having lots of almost-empty header files, though.
bool PresentOGLD3D();
void FakeBroadcastDisplayChange(int width, int height, int depth);


static void FrameBoundaryDIBitsToAVI(const void* bits, const BITMAPINFO& bmi)
{
	DDSURFACEDESC desc = { sizeof(DDSURFACEDESC) };
	desc.lpSurface = const_cast<LPVOID>(bits);
	desc.dwWidth = bmi.bmiHeader.biWidth;
	desc.dwHeight = bmi.bmiHeader.biHeight;
	desc.lPitch = bmi.bmiHeader.biWidth * (bmi.bmiHeader.biBitCount >> 3);
	// correct for upside-down bitmaps.
	if((LONG)desc.dwHeight < 0)
	{
		// FillFrame won't understand negative height.
		desc.dwHeight = -(LONG)desc.dwHeight;
	}
	else
	{
		// give WriteAVIFrame negative pitch.
		(char*&)desc.lpSurface += (desc.dwHeight - 1) * desc.lPitch;
		desc.lPitch = -desc.lPitch;
	}
	desc.ddpfPixelFormat.dwRGBBitCount = bmi.bmiHeader.biBitCount;

	bool valid = true;
	switch(bmi.bmiHeader.biCompression)
	{
	case BI_RGB:
		switch(bmi.bmiHeader.biBitCount)
		{
		case 32:
		case 24:
			desc.ddpfPixelFormat.dwRBitMask = 0xFF0000;
			desc.ddpfPixelFormat.dwGBitMask = 0x00FF00;
			desc.ddpfPixelFormat.dwBBitMask = 0x0000FF;
			break;
		case 16: // RGB555
			desc.ddpfPixelFormat.dwRBitMask = 0x7C00;
			desc.ddpfPixelFormat.dwGBitMask = 0x03E0;
			desc.ddpfPixelFormat.dwBBitMask = 0x001F;
			break;
		case 8:
			//descExtra.paletteData = (PALETTEENTRY*)bmi.bmiColors;
			//descExtra.paletteEntryCount = bmi.bmiHeader.biClrUsed;
			//break;
			// FALLTHROUGH because NYI
		default:
			valid = false;
			break;
		}
		break;
	case BI_BITFIELDS:
		// probably RGB565
		desc.ddpfPixelFormat.dwRBitMask = 0[(DWORD*)bmi.bmiColors];
		desc.ddpfPixelFormat.dwGBitMask = 1[(DWORD*)bmi.bmiColors];
		desc.ddpfPixelFormat.dwBBitMask = 2[(DWORD*)bmi.bmiColors];
		break;
	default:
		valid = false;
		break;
	}

	if(valid)
	{
		FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
	}
	else
	{
		debuglog(LCF_GDI|LCF_ERROR, __FUNCTION__": unhandled bit count %d with biCompression=%d\n", bmi.bmiHeader.biBitCount, bmi.bmiHeader.biCompression);
		FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
	}
}

union FULLBITMAPINFO
{
	BITMAPINFO bmi;
    RGBQUAD remainingColors [255];
};

static void FrameBoundaryHDCtoAVI(HDC hdc,int xSrc,int ySrc,int xRes,int yRes)
{
	HBITMAP bitmap = (HBITMAP)GetCurrentObject(hdc,OBJ_BITMAP);
	FULLBITMAPINFO fbmi = {sizeof(BITMAPINFOHEADER)};
	BITMAPINFO& bmi = *(BITMAPINFO*)&fbmi;
	GetDIBits(hdc, bitmap, 0, 0, 0, &bmi, DIB_RGB_COLORS);

	static char* bits = NULL;
	static unsigned int bitsAllocated = 0;
	if(bitsAllocated < bmi.bmiHeader.biSizeImage)
	{
		bits = (char*)realloc(bits, bmi.bmiHeader.biSizeImage);
		bitsAllocated = bmi.bmiHeader.biSizeImage;
	}

	int height = bmi.bmiHeader.biHeight;
	if(height < 0) height = -height;
	GetDIBits(hdc, bitmap, 0, height, bits, &bmi, DIB_RGB_COLORS);
	FrameBoundaryDIBitsToAVI(bits, bmi);
}



int depth_SwapBuffers = 0;
HOOKFUNC BOOL MySwapBuffers(HDC hdc)
{
	depth_SwapBuffers++;
	//if(!usingSDLOrDD)
	if(!redrawingScreen)
	{
//		int localFramecount = framecount;
//		if(localFramecount == framecount)
		if(!PresentOGLD3D())
		{
			SwapBuffers(hdc);
		// TODO: FrameBoundaryHDCtoAVI here?
			FrameBoundary();
		}
	}
	//else
	{
// maybe this branch is just broken?
// rescue: the beagles crashes with no clear callstack if we get here.
// disabled for now.
		//SwapBuffers(hdc);
	}
	depth_SwapBuffers--;

	return TRUE;
}


static HDC s_hdcSrcSaved;
static HDC s_hdcDstSaved;
static bool s_gdiPendingRefresh;
bool RedrawScreenGDI();


HOOKFUNC BOOL WINAPI MyStretchBlt(
	HDC hdcDest,
	int nXOriginDest, int nYOriginDest,
	int nWidthDest, int nHeightDest,
	HDC hdcSrc,
	int nXOriginSrc, int nYOriginSrc,
	int nWidthSrc, int nHeightSrc,
	DWORD dwRop)
{
//	debuglog(LCF_GDI|LCF_FRAME|LCF_FREQUENT, __FUNCTION__ " called.\n");
//	_asm {int 3};

	bool isFrameBoundary = false;
	if(!usingSDLOrDD /*&& !inPauseHandler*/ && !redrawingScreen)
	{
		if(dwRop == SRCCOPY && s_gdiPhaseDetector.AdvanceAndCheckCycleBoundary(MAKELONG(nXOriginDest,nYOriginDest)))
		{
			if(tls.peekedMessage && VerifyIsTrustedCaller(!tls.callerisuntrusted))
			{
				HWND hwnd = WindowFromDC(hdcDest);
				if(hwnd /*&& !hwndRespondingToPaintMessage[hwnd]*/)
				{
// 					debugprintf("WINDOW EQUALS VERY 0x%X\n", hwnd);
					isFrameBoundary = true;
				}
			}
		}
	}

	debuglog(LCF_GDI|(isFrameBoundary?LCF_FRAME:LCF_FREQUENT), __FUNCTION__ " called.\n");


	BOOL rv;
	if(!ShouldSkipDrawing(false, true))
	{
		if(s_gdiPendingRefresh && !redrawingScreen)
		{
			redrawingScreen = true;
			RedrawScreenGDI();
			redrawingScreen = false;
		}
		rv = StretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, dwRop);
	}
	else
		s_gdiPendingRefresh = true;

	if(isFrameBoundary)
	{
		tls.peekedMessage = FALSE;
		if(!(tasflags.aviMode & 1))
			FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
		else
			FrameBoundaryHDCtoAVI(hdcSrc,nXOriginSrc,nYOriginSrc,nWidthSrc,nHeightSrc);
		s_hdcSrcSaved = hdcSrc;
		s_hdcDstSaved = hdcDest;
	}

	return rv;
}
HOOKFUNC BOOL WINAPI MyBitBlt(
	HDC hdcDest,
	int nXDest, int nYDest,
	int nWidth, int nHeight,
	HDC hdcSrc,
	int nXSrc, int nYSrc,
	DWORD dwRop)
{
	//debuglog(LCF_GDI|LCF_FRAME|LCF_FREQUENT, "(0x%X,%d,%d,%d,%d,0x%X,%d,%d,0x%X) called.\n", hdcDest,nXDest,nYDest,nWidth,nHeight,hdcSrc,nXSrc,nYSrc,dwRop);
//	_asm {int 3};

	bool isFrameBoundary = false;
	if(!usingSDLOrDD /*&& !inPauseHandler*/ && !redrawingScreen)
	{
		if(dwRop == SRCCOPY && s_gdiPhaseDetector.AdvanceAndCheckCycleBoundary(MAKELONG(nXDest,nYDest)))
		{
			if(tls.peekedMessage && VerifyIsTrustedCaller(!tls.callerisuntrusted))
			{
				HWND hwnd = WindowFromDC(hdcDest);
				if(hwnd /*&& !hwndRespondingToPaintMessage[hwnd]*/)
				{
// 					debugprintf("WINDOW EQUALS VERY 0x%X\n", hwnd);
					isFrameBoundary = true;
				}
			}
		}
	}

	debuglog(LCF_GDI|(isFrameBoundary?LCF_FRAME:LCF_FREQUENT), __FUNCTION__ " called.\n");



	BOOL rv;
	if(!ShouldSkipDrawing(false, true))
	{
		if(s_gdiPendingRefresh && !redrawingScreen)
		{
			redrawingScreen = true;
			RedrawScreenGDI();
			redrawingScreen = false;
		}
		if(!fakeDisplayValid)
		{
			rv = BitBlt(hdcDest,nXDest,nYDest,nWidth,nHeight,hdcSrc,nXSrc,nYSrc,dwRop);
		}
		else
		{
			HWND hwnd = WindowFromDC(hdcDest);
			RECT rect;
			if(!GetClientRect(hwnd, &rect) || (rect.right == fakeDisplayWidth && rect.bottom == fakeDisplayHeight))
			{
				rv = BitBlt(hdcDest,nXDest,nYDest,nWidth,nHeight,hdcSrc,nXSrc,nYSrc,dwRop);
			}
			else
			{
				// support resized fake-fullscreen windows in games like Lyle in Cube Sector
				HDC hdcTemp = 0;
				HDC hdc = hdcDest;
				if(rect.right > fakeDisplayWidth || rect.bottom > fakeDisplayHeight)
				{
					// sidestep clip region (it can't be expanded without switching HDCs)
					hdcTemp = GetDC(hwnd);
					hdc = hdcTemp;
				}
				// iffy, sprites leave pixels behind occasionally at non-integral scales
				int x0 = (nXDest * rect.right) / fakeDisplayWidth;
				int y0 = (nYDest * rect.bottom) / fakeDisplayHeight;
				int x1 = ((nXDest+nWidth) * rect.right) / fakeDisplayWidth;
				int y1 = ((nYDest+nHeight) * rect.bottom) / fakeDisplayHeight;
				//debugprintf("%dx%d -> %dx%d ... (%d,%d,%d,%d) -> (%d,%d,%d,%d)\n", fakeDisplayWidth, fakeDisplayHeight, rect.right, rect.bottom,  nXDest,nYDest,nXDest+nWidth,nYDest+nHeight, x0,y0,x1,y1);
				rv = StretchBlt(hdc,x0,y0,x1-x0,y1-y0,hdcSrc,nXSrc,nYSrc,nWidth,nHeight,dwRop);
				if(hdcTemp)
					ReleaseDC(hwnd, hdcTemp);
			}
		}
	}
	else
		s_gdiPendingRefresh = true;

	//debugprintf("rv = 0x%X, le=0x%X\n", rv, GetLastError());

	//if(isFrameBoundary)
	//	FrameBoundary();

	//debugprintf("AZX: 0x%X, %d %d, %d %d, 0x%X, %d %d, 0x%X\n", hdcDest,nXDest,nYDest,nWidth,nHeight,hdcSrc,nXSrc,nYSrc,dwRop);
	//HWND hwnd = WindowFromDC(hdcDest);
	//RECT rect;
	//if(GetClientRect(hwnd, &rect))
	//{
	//	debugprintf("SMC: 0x%X, %d,%d,%d,%d\n", hwnd, rect.left,rect.top,rect.right,rect.bottom);
	//}
	////int a = GetDeviceCaps(hdcSrc, TECHNOLOGY);
	////int b = GetDeviceCaps(hdcDest, TECHNOLOGY);
	////int c = GetDeviceCaps(hdcSrc, HORZRES);
	////int d = GetDeviceCaps(hdcDest, HORZRES);
	////int e = GetDeviceCaps(hdcSrc, VERTRES);
	////int f = GetDeviceCaps(hdcDest, VERTRES);
	////debugprintf("AZX: %d %d, %d %d, %d %d\n", a,b,c,d,e,f);

	if(isFrameBoundary)
	{
		tls.peekedMessage = FALSE;
		if(!(tasflags.aviMode & 1))
			FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
		else
			FrameBoundaryHDCtoAVI(hdcSrc,nXSrc,nYSrc,nWidth,nHeight);
		s_hdcSrcSaved = hdcSrc;
		s_hdcDstSaved = hdcDest;
	}
//cmdprintf("SHORTTRACE: 3,50");



	return rv;

//
////	debugprintf(__FUNCTION__ " called.\n");
//	BOOL rv = BitBlt(hdcDest,nXDest,nYDest,nWidth,nHeight,hdcSrc,nXSrc,nYSrc,dwRop);
//	DWORD lasterr = GetLastError();
//	if(!rv)
//		debugprintf("ERROR: 0x%X\n", lasterr);
////	BOOL rv = StretchBlt(hdcDest,0,0,100,100,hdcSrc,0,0,100,100,dwRop);
////	_asm {int 3};
////	debugprintf("BitBlt(hdcDest=0x%X, hdcSrc=0x%X, nXDest=%d, nYDest=%d, nWidth=%d, nHeight=%d, nXSrc=%d, nYSrc=%d, dwRop=0x%X)\n",  hdcDest, hdcSrc, nXDest, nYDest, nWidth, nHeight, nXSrc, nYSrc, dwRop);
//	if(extHWnd && 0)
//	{
//		HDC dc = GetDC(extHWnd);
//		RECT rect;
//		GetClientRect(extHWnd, &rect);
//		//BOOL rv = StretchBlt(dc,0,0,rect.right,rect.bottom,hdcSrc,nXSrc,nYSrc,nXDest,nYDest,dwRop);
//		BOOL rv = StretchBlt(dc,0,0,rect.right,rect.bottom,hdcSrc,nXSrc,nYSrc,nWidth,nHeight,dwRop);
//		ReleaseDC(extHWnd, dc);
//	}
//	FrameBoundary();
//	return rv;
}

bool RedrawScreenGDI()
{
	s_gdiPendingRefresh = false;
	if(!s_hdcSrcSaved)
		return false;
	RECT rect;
	if(!GetClientRect(WindowFromDC(s_hdcDstSaved), &rect))
		return false;
	BITMAP srcBitmap;
	if(!GetObject((HBITMAP)GetCurrentObject(s_hdcSrcSaved,OBJ_BITMAP), sizeof(BITMAP), &srcBitmap))
		return false;
	MyStretchBlt(s_hdcDstSaved,0,0,rect.right,rect.bottom,s_hdcSrcSaved,0,0,srcBitmap.bmWidth,srcBitmap.bmHeight,SRCCOPY);
	return true;
}


HOOKFUNC int WINAPI MySetDIBitsToDevice(HDC hdc, int xDest, int yDest, DWORD w, DWORD h, int xSrc, int ySrc, UINT StartScan, UINT cLines, CONST VOID * lpvBits, CONST BITMAPINFO * lpbmi, UINT ColorUse)
{
	int rv = SetDIBitsToDevice(hdc, xDest, yDest, w, h, xSrc, ySrc, StartScan, cLines, lpvBits, lpbmi, ColorUse);
	if(!usingSDLOrDD /*&& !inPauseHandler*/ && !redrawingScreen)
	{
		if(rv != 0 && rv != GDI_ERROR && VerifyIsTrustedCaller(!tls.callerisuntrusted) && s_gdiPhaseDetector.AdvanceAndCheckCycleBoundary(MAKELONG(xDest,yDest)))
		{
			if(!(tasflags.aviMode & 1))
				FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
			else
				FrameBoundaryDIBitsToAVI(lpvBits,*lpbmi);
		}
	}
	return rv;
}

HOOKFUNC int WINAPI MyStretchDIBits(HDC hdc, int xDest, int yDest, int DestWidth, int DestHeight, int xSrc, int ySrc, int SrcWidth, int SrcHeight, CONST VOID * lpBits, CONST BITMAPINFO * lpbmi, UINT iUsage, DWORD rop)
{
	int rv = StretchDIBits(hdc, xDest, yDest, DestWidth, DestHeight, xSrc, ySrc, SrcWidth, SrcHeight, lpBits, lpbmi, iUsage, rop);
	if(!usingSDLOrDD /*&& !inPauseHandler*/ && !redrawingScreen)
	{
		if(rv != 0 && rv != GDI_ERROR && rop == SRCCOPY && VerifyIsTrustedCaller(!tls.callerisuntrusted) && s_gdiPhaseDetector.AdvanceAndCheckCycleBoundary(MAKELONG(xDest,yDest)))
		{
			if(!(tasflags.aviMode & 1))
				FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
			else
				FrameBoundaryDIBitsToAVI(lpBits,*lpbmi);
		}
	}
	return rv;
}



// TODO: DrawDib support needed?




HOOKFUNC int WINAPI MyChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR* pfd)
{
	debuglog(LCF_GDI, __FUNCTION__ " called.\n");
	int rv = ChoosePixelFormat(hdc,pfd);
	return rv;
}
int depth_SetPixelFormat = 0;
HOOKFUNC BOOL WINAPI MySetPixelFormat(HDC hdc, int format, CONST PIXELFORMATDESCRIPTOR * pfd)
{
	debuglog(LCF_UNTESTED|LCF_GDI, __FUNCTION__ " called.\n");
	BOOL rv;
	if(!tasflags.forceWindowed)
	{
		depth_SetPixelFormat++;
		rv = SetPixelFormat(hdc,format,pfd);
		depth_SetPixelFormat--;
	}
	else
		rv = TRUE;
	return rv;
}
HOOKFUNC COLORREF WINAPI MyGetPixel(HDC hdc, int xx, int yy)
{
	debuglog(LCF_TODO|LCF_UNTESTED|LCF_DESYNC|LCF_GDI, __FUNCTION__ " called.\n");
	COLORREF rv = GetPixel(hdc,xx,yy);
	//debugprintf("XXX: 0x%08X", rv);
	return rv;
}

HOOKFUNC int WINAPI MyGetDeviceCaps(HDC hdc, int index)
{
//	debuglog(LCF_TODO|LCF_UNTESTED|LCF_DESYNC|LCF_GDI, __FUNCTION__ "(%d) called.\n", index);
	int rv;
	bool got = false;
	if(fakeDisplayValid)
	{
		switch(index)
		{
		case HORZRES: case DESKTOPHORZRES: rv = fakeDisplayWidth; got = true; break;
		case VERTRES: case DESKTOPVERTRES: rv = fakeDisplayHeight; got = true; break;
		case BITSPIXEL: rv = fakePixelFormatBPP; got = true; break;
		case VREFRESH: rv = fakeDisplayRefresh; got = true; break;
		}
	}
	if(!got)
	{
		switch(index)
		{
		case TECHNOLOGY: rv = DT_RASDISPLAY; got = true; break;
		case PLANES: rv = 1; got = true; break;
		case LOGPIXELSX: case LOGPIXELSY: rv = 96; got = true; break;
		//case BITSPIXEL: // user-specified default? but that's what their display setting is anyway.
		case VREFRESH: {
			// TODO: unduplicate, see GetMonitorFrequency
			int dist50 = abs((tasflags.framerate % 25) - 12);
			int dist60 = abs((tasflags.framerate % 15) - 8) * 3 / 2;
			if(dist60 <= dist50)
				rv = 60;
			else if(tasflags.framerate < 75)
				rv = 50;
			else
				rv = 100;
			got = true;
			} break;
		}
	}
	if(!got)
	{
		rv = GetDeviceCaps(hdc,index);
	}
	debuglog((got?0:(LCF_TODO|LCF_UNTESTED|LCF_DESYNC))|LCF_GDI, __FUNCTION__ "(%d) called, returned %d (0x%X).\n", index, rv, rv);
	//debuglog(LCF_GDI, __FUNCTION__ "(%d) returned %d (0x%X).\n", index, rv, rv);
	return rv;
}

HOOKFUNC HFONT WINAPI MyCreateFontIndirectA(CONST LOGFONTA *lplf)
{
	debuglog(LCF_GDI, __FUNCTION__ " called (quality=%d).\n", lplf->lfQuality);
	if(lplf->lfQuality != NONANTIALIASED_QUALITY)
		((LOGFONTA*)lplf)->lfQuality = ANTIALIASED_QUALITY; // disable ClearType so it doesn't get into AVIs
	HFONT rv = CreateFontIndirectA(lplf);
	return rv;
}
HOOKFUNC HFONT WINAPI MyCreateFontIndirectW(CONST LOGFONTW *lplf)
{
	debuglog(LCF_GDI, __FUNCTION__ " called (quality=%d).\n", lplf->lfQuality);
	if(lplf->lfQuality != NONANTIALIASED_QUALITY)
		((LOGFONTW*)lplf)->lfQuality = ANTIALIASED_QUALITY; // disable ClearType so it doesn't get into AVIs
	HFONT rv = CreateFontIndirectW(lplf);
	return rv;
}



HOOKFUNC LONG WINAPI MyChangeDisplaySettingsA(LPDEVMODEA lpDevMode, DWORD dwFlags)
{
	if(lpDevMode)
		debugprintf(__FUNCTION__ "(width=%d, height=%d, flags=0x%X) called.\n", lpDevMode->dmPelsWidth, lpDevMode->dmPelsHeight, dwFlags);
	else
		debugprintf(__FUNCTION__ "(flags=0x%X) called.\n", dwFlags);
	if(tasflags.forceWindowed && lpDevMode /*&& (dwFlags & CDS_FULLSCREEN)*/)
	{
		fakeDisplayWidth = lpDevMode->dmPelsWidth;
		fakeDisplayHeight = lpDevMode->dmPelsHeight;
		fakePixelFormatBPP = lpDevMode->dmBitsPerPel;
		fakeDisplayValid = TRUE;
		FakeBroadcastDisplayChange(fakeDisplayWidth,fakeDisplayHeight,fakePixelFormatBPP);
		if(gamehwnd)
			MakeWindowWindowed(gamehwnd, fakeDisplayWidth, fakeDisplayHeight);

		return DISP_CHANGE_SUCCESSFUL;
	}
	LONG rv = ChangeDisplaySettingsA(lpDevMode, dwFlags);
	if(lpDevMode) FakeBroadcastDisplayChange(lpDevMode->dmPelsWidth,lpDevMode->dmPelsHeight,lpDevMode->dmBitsPerPel);
	return rv;
}
HOOKFUNC LONG WINAPI MyChangeDisplaySettingsW(LPDEVMODEW lpDevMode, DWORD dwFlags)
{
	if(lpDevMode)
		debugprintf(__FUNCTION__ "(width=%d, height=%d, flags=0x%X) called.\n", lpDevMode->dmPelsWidth, lpDevMode->dmPelsHeight, dwFlags);
	else
		debugprintf(__FUNCTION__ "(flags=0x%X) called.\n", dwFlags);
	if(tasflags.forceWindowed && lpDevMode /*&& (dwFlags & CDS_FULLSCREEN)*/)
	{
		fakeDisplayWidth = lpDevMode->dmPelsWidth;
		fakeDisplayHeight = lpDevMode->dmPelsHeight;
		fakePixelFormatBPP = lpDevMode->dmBitsPerPel;
		fakeDisplayValid = TRUE;
		FakeBroadcastDisplayChange(fakeDisplayWidth,fakeDisplayHeight,fakePixelFormatBPP);
		if(gamehwnd)
			MakeWindowWindowed(gamehwnd, fakeDisplayWidth, fakeDisplayHeight);

		return DISP_CHANGE_SUCCESSFUL;
	}
	LONG rv = MyChangeDisplaySettingsW(lpDevMode, dwFlags);
	if(lpDevMode) FakeBroadcastDisplayChange(lpDevMode->dmPelsWidth,lpDevMode->dmPelsHeight,lpDevMode->dmBitsPerPel);
	return rv;
}


void ApplyGDIIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, GDI32, StretchBlt),
		MAKE_INTERCEPT(1, GDI32, BitBlt),
		MAKE_INTERCEPT(1, GDI32, SetDIBitsToDevice),
		MAKE_INTERCEPT(1, GDI32, StretchDIBits),
		MAKE_INTERCEPT(1, GDI32, GetPixel),
		MAKE_INTERCEPT(1, GDI32, GetDeviceCaps),
		MAKE_INTERCEPT(1, GDI32, ChoosePixelFormat),
		MAKE_INTERCEPT(/*1*/1, GDI32, SetPixelFormat), // todo? but overriding it breaks tumiki fighters sometimes?
		MAKE_INTERCEPT(1, GDI32, SwapBuffers),

		MAKE_INTERCEPT(1, GDI32, CreateFontIndirectA),
		MAKE_INTERCEPT(1, GDI32, CreateFontIndirectW),

		MAKE_INTERCEPT(1, USER32, ChangeDisplaySettingsA),
		MAKE_INTERCEPT(1, USER32, ChangeDisplaySettingsW),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
