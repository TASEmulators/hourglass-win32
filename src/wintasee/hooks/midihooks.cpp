// WIP CODE

#if !defined(MIDIHOOKS_INCL) && !defined(UNITY_BUILD)
#define MIDIHOOKS_INCL

#include <Windows.h>
#include <MMSystem.h>
#include <MMReg.h>

#include <stdio.h>

#include "../global.h"
#include "../../shared/ipc.h"
#include "../wintasee.h"
#include "../msgqueue.h"

#define mididebugprint debugprintf
//#define USETHEIRS

// these both match the return results from the "Microsoft GS Wavetable Synth"

const MIDIOUTCAPSA devcapsa =
{
    MM_MICROSOFT,                  /* manufacturer ID */
    0x1b,                  /* product ID */
    0x100,      /* version of the driver */
    "Microsoft GS Wavetable Synth",  /* product name (NULL terminated string) */
    MOD_SWSYNTH,           /* type of device */
    32,               /* # of voices (internal synth only) */
    32,                /* max # of notes (internal synth only) */
    0xffff,          /* channels used (internal synth only) */
    MIDICAPS_VOLUME,             /* functionality supported by driver */
};
const MIDIOUTCAPSW devcapsw =
{
    MM_MICROSOFT,                  /* manufacturer ID */
    0x1b,                  /* product ID */
    0x100,      /* version of the driver */
    L"Microsoft GS Wavetable Synth",  /* product name (NULL terminated string) */
    MOD_SWSYNTH,           /* type of device */
    32,               /* # of voices (internal synth only) */
    32,                /* max # of notes (internal synth only) */
    0xffff,          /* channels used (internal synth only) */
    MIDICAPS_VOLUME,             /* functionality supported by driver */
};


#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

HOOKFUNC MMRESULT WINAPI MymidiOutGetDevCapsA(  UINT_PTR uDeviceID,  LPMIDIOUTCAPSA pmoc,  UINT cbmoc)
{
	mididebugprint("MIDIDEBUG: 'midiOutGetDevCapsA' (%08x,%08x,%08x,)\n", (int) (void *) uDeviceID, (int) (void *) pmoc, (int) (void *) cbmoc);
#if 0 // def USETHEIRS
	int ret = midiOutGetDevCapsA(uDeviceID, pmoc, cbmoc);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	// assume the correct device is being queried
	if (pmoc)
		memcpy (pmoc, &devcapsa, MIN (cbmoc, sizeof (devcapsa)));
	return MMSYSERR_NOERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiOutGetDevCapsW(  UINT_PTR uDeviceID,  LPMIDIOUTCAPSW pmoc,  UINT cbmoc)
{
	mididebugprint("MIDIDEBUG: 'midiOutGetDevCapsW' (%08x,%08x,%08x,)\n", (int) (void *) uDeviceID, (int) (void *) pmoc, (int) (void *) cbmoc);
#if 0 // def USETHEIRS
	int ret = midiOutGetDevCapsW(uDeviceID, pmoc, cbmoc);
	mididebugprint("\t#%i#\n", ret);
	char temp[1024];
	sprintf (temp, "\t%08x manu\n\t%08x prod\n\t%08xver\n\t%ls\n\t%08x type\n\t%08x voice\n\t%08x note\n\t%08x chan\n\t%08x func\n",
		 pmoc->wMid, pmoc->wPid, pmoc->vDriverVersion, pmoc->szPname, pmoc->wTechnology, pmoc->wVoices, pmoc->wNotes, pmoc->wChannelMask, pmoc->dwSupport);
	mididebugprint ("%s", temp);

	return ret;

#endif

	// assume the correct device is being queried
	if (pmoc)
		memcpy (pmoc, &devcapsw, MIN (cbmoc, sizeof (devcapsw)));
	return MMSYSERR_NOERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiOutGetErrorTextA(  MMRESULT mmrError,  LPSTR pszText,  UINT cchText)
{
	mididebugprint("MIDIDEBUG: 'midiOutGetErrorTextA' (%08x,%08x,%08x,)\n", (int) (void *) mmrError, (int) (void *) pszText, (int) (void *) cchText);
#ifdef USETHEIRS
	int ret = midiOutGetErrorTextA(mmrError, pszText, cchText);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	// no reason we can't use the official functions here
	return midiOutGetErrorTextA (mmrError, pszText, cchText);
}

HOOKFUNC MMRESULT WINAPI MymidiOutGetErrorTextW(  MMRESULT mmrError,  LPWSTR pszText,  UINT cchText)
{
	mididebugprint("MIDIDEBUG: 'midiOutGetErrorTextW' (%08x,%08x,%08x,)\n", (int) (void *) mmrError, (int) (void *) pszText, (int) (void *) cchText);
#ifdef USETHEIRS
	int ret = midiOutGetErrorTextW(mmrError, pszText, cchText);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	return midiOutGetErrorTextW (mmrError, pszText, cchText);
}

HOOKFUNC UINT WINAPI MymidiOutGetNumDevs(void)
{
	mididebugprint("MIDIDEBUG: 'midiOutGetNumDevs' (,)\n");
#ifdef USETHEIRS
	//int ret = midiOutGetNumDevs();
	int ret = 0;
	mididebugprint("\t#%i#\n", ret);
	return 1;
	return ret;

#endif
	return 1;
	return 1;
}


struct MidiHandler
{
	FILE *output;
	void (CALLBACK *fcnproc) (HMIDIOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
	DWORD_PTR userdata;
	unsigned deadbeef;
};

#define CHECK_DEADBEEF if ((unsigned) hmo == 0xffffffff || (unsigned) hmo < 0x100)\
	{\
		debugprintf ("caught a midi ID?\n");\
		return MMSYSERR_ERROR;\
	}\
	if (h->deadbeef != 0xdeadbeef)\
	{\
	debugprintf ("caught someone else's HMIDIOUT?\n");\
	return MMSYSERR_ERROR;\
	}


HOOKFUNC MMRESULT WINAPI MymidiOutOpen(  LPHMIDIOUT phmo,  UINT uDeviceID,  DWORD_PTR dwCallback,  DWORD_PTR dwInstance,  DWORD fdwOpen)
{
	mididebugprint("MIDIDEBUG: 'midiOutOpen' (%08x,%08x,%08x,%08x,%08x,)\n", (int) (void *) phmo, (int) (void *) uDeviceID, (int) (void *) dwCallback, (int) (void *) dwInstance, (int) (void *) fdwOpen);

	int ismapper = 0;
	if (uDeviceID == MIDI_MAPPER)
	{
		ismapper = 1;
	}

#ifdef USETHEIRS
	int ret = midiOutOpen(phmo, uDeviceID, dwCallback, dwInstance, fdwOpen);
	mididebugprint("\t#%i#\n", ret);
	mididebugprint("\tOPEN:ID:%08x H:%08x %s\n", (int) (void *) uDeviceID, (int) (void *) (*phmo), ismapper ? "MAPPER" : "NOMAP");
	return ret;

#endif

	static int numopen = 0;

	if (!phmo)
		return MMSYSERR_ERROR;
	// hi word only, i think
	DWORD callbacktype = fdwOpen & 0xffff0000;

	/*                  0x00000000                        0x00030000      */
	if (callbacktype == CALLBACK_NULL || callbacktype == CALLBACK_FUNCTION)
	{

		debugprintf ("midiOutOpen, starting output!\n");
		char name[32];
		sprintf (name, "%03imidilog.txt", numopen++);
		MidiHandler *h = new MidiHandler();
		h->fcnproc = callbacktype == CALLBACK_NULL ? NULL : (void (CALLBACK *) (HMIDIOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)) dwCallback;
		h->userdata = dwInstance;
		h->output = fopen (name, "w");
		h->deadbeef = 0xdeadbeef;
		fprintf (h->output, "%d,Starting output\n", detTimer.GetTicks());
		if (h->fcnproc)
		{
			debugprintf ("Calling Function %08x\n", h->fcnproc);
			h->fcnproc ((HMIDIOUT) (void *) h, MOM_OPEN, h->userdata, 0, 0);
		}
		*phmo = (HMIDIOUT) (void *) h;
		mididebugprint("midi: open instance to %08x\n", (int) (void *) h);
		return MMSYSERR_NOERROR;
	}

	switch (callbacktype)
	{
		case CALLBACK_EVENT:  // 0x00050000
			debugprintf ("midiOutOpen, but we can't support the callback yet! (evt)\n");
			return MMSYSERR_ERROR;
		case CALLBACK_THREAD: // 0x00020000
			debugprintf ("midiOutOpen, but we can't support the callback yet! (thd)\n");
			return MMSYSERR_ERROR;
		case CALLBACK_WINDOW: // 0x00010000
			debugprintf ("midiOutOpen, but we can't support the callback yet! (wnd)\n");
			return MMSYSERR_ERROR;
		default:
			debugprintf ("midiOutOpen, but we can't support the callback yet! (? ?)\n");
			return MMSYSERR_ERROR;
	}
}

HOOKFUNC MMRESULT WINAPI MymidiOutClose(  HMIDIOUT hmo)
{
	mididebugprint("MIDIDEBUG: 'midiOutClose' (%08x,)\n", (int) (void *) hmo);
#ifdef USETHEIRS
	int ret = midiOutClose(hmo);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	MidiHandler *h = (MidiHandler *) hmo;
	CHECK_DEADBEEF;
	fprintf (h->output, "%d,finalizing...\n", detTimer.GetTicks());
	fclose (h->output);
	if (h->fcnproc)
	{
		debugprintf ("Calling Function %08x\n", h->fcnproc);
		h->fcnproc (hmo, MOM_CLOSE, h->userdata, 0, 0);
	}
	delete h;
	return MMSYSERR_NOERROR;
}


// sysex: pretend to support this
HOOKFUNC MMRESULT WINAPI MymidiOutLongMsg( HMIDIOUT hmo,  LPMIDIHDR pmh,  UINT cbmh)
{
	mididebugprint("MIDIDEBUG: 'midiOutLongMsg' (%08x,%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) pmh, (int) (void *) cbmh);
	char scrap[8192];
	int buffpos = 0;
	buffpos += sprintf (scrap + buffpos, "\t[%u]:", pmh->dwBufferLength);
	for (unsigned i = 0; i < pmh->dwBufferLength; i++)
	{
		buffpos += sprintf (scrap + buffpos, "%02x,", (unsigned) (unsigned char) pmh->lpData[i]);
	}
	buffpos += sprintf (scrap + buffpos, "\n");
	mididebugprint (scrap);
#ifdef USETHEIRS
	int ret = midiOutLongMsg(hmo, pmh, cbmh);
	mididebugprint("\t#%i#\n", ret);

	return ret;

#endif

	MidiHandler *h = (MidiHandler *) hmo;
	CHECK_DEADBEEF;
	fprintf (h->output, "%d,sysex,", detTimer.GetTicks());
	for (unsigned i = 0; i < pmh->dwBufferLength; i++)
	{
		fprintf (h->output, "%02x,", (unsigned) (unsigned char) pmh->lpData[i]);
	}
	fprintf (h->output, "\n");

	// invoke function callback
	if (h->fcnproc)
	{
		mididebugprint("Calling Callback on finish longmsg\n");
		h->fcnproc(hmo, MOM_DONE, h->userdata, (int) (void *) pmh, 0);
	}
	return MMSYSERR_NOERROR;
}

// prep sysex buffer
HOOKFUNC MMRESULT WINAPI MymidiOutPrepareHeader(  HMIDIOUT hmo,  LPMIDIHDR pmh,  UINT cbmh)
{
	mididebugprint("MIDIDEBUG: 'midiOutPrepareHeader' (%08x,%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) pmh, (int) (void *) cbmh);
#ifdef USETHEIRS
	int ret = midiOutPrepareHeader(hmo, pmh, cbmh);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif
	if (!pmh)
	{
		mididebugprint("NULL PTR\n");
		return MMSYSERR_ERROR;
	}
	pmh->dwFlags |= MHDR_PREPARED;
	return MMSYSERR_NOERROR;
}

// unprep sysex buffer
HOOKFUNC MMRESULT WINAPI MymidiOutUnprepareHeader( HMIDIOUT hmo,  LPMIDIHDR pmh,  UINT cbmh)
{
	mididebugprint("MIDIDEBUG: 'midiOutUnprepareHeader' (%08x,%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) pmh, (int) (void *) cbmh);
#ifdef USETHEIRS
	int ret = midiOutUnprepareHeader(hmo, pmh, cbmh);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif
	if (!pmh)
	{
		mididebugprint("NULL PTR\n");
		return MMSYSERR_ERROR;
	}
	pmh->dwFlags &= ~MHDR_PREPARED;
	return MMSYSERR_NOERROR;
}


HOOKFUNC MMRESULT WINAPI MymidiOutShortMsg(  HMIDIOUT hmo,  DWORD dwMsg)
{
	mididebugprint("MIDIDEBUG: 'midiOutShortMsg' (%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) dwMsg);
#ifdef USETHEIRS
	int ret = midiOutShortMsg(hmo, dwMsg);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	MidiHandler *h = (MidiHandler *) hmo;
	CHECK_DEADBEEF;
	fprintf (h->output, "%d,%08x\n", dwMsg, detTimer.GetTicks());
	debugprintf ("%08x,%d\n", dwMsg, detTimer.GetTicks());
	return MMSYSERR_NOERROR;
}


HOOKFUNC MMRESULT WINAPI MymidiOutReset(  HMIDIOUT hmo)
{
#ifdef USETHEIRS
	int ret = midiOutReset(hmo);
	mididebugprint("MIDIDEBUG: 'midiOutReset' (%08x,)\n", (int) (void *) hmo);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	MidiHandler *h = (MidiHandler *) hmo;
	CHECK_DEADBEEF;
	fprintf (h->output, "%d,##RESET##\n", detTimer.GetTicks());
	return MMSYSERR_NOERROR;
}

// driver specific messages: eat them
HOOKFUNC MMRESULT WINAPI MymidiOutMessage(  HMIDIOUT hmo,  UINT uMsg,  DWORD_PTR dw1,  DWORD_PTR dw2)
{
#ifdef USETHEIRS
	int ret = midiOutMessage(hmo, uMsg, dw1, dw2);
	mididebugprint("MIDIDEBUG: 'midiOutMessage' (%08x,%08x,%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) uMsg, (int) (void *) dw1, (int) (void *) dw2);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	// hmo can be a device ID?, so be careful here
	debugprintf ("midiOutMessage: %d %u %d %d\n", hmo, uMsg, dw1, dw2);
	return MMSYSERR_NOERROR;
}





/* misc junk functions */

HOOKFUNC MMRESULT WINAPI MymidiOutCachePatches(  HMIDIOUT hmo,  UINT uBank,  LPWORD pwpa,  UINT fuCache)
{
	mididebugprint("MIDIDEBUG: 'midiOutCachePatches' (%08x,%08x,%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) uBank, (int) (void *) pwpa, (int) (void *) fuCache);
#ifdef USETHEIRS
	//int ret = midiOutCachePatches(hmo, uBank, pwpa, fuCache);
	//mididebugprint("\t#%i#\n", ret);
	//return ret;

#endif

	return MMSYSERR_NOERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiOutCacheDrumPatches(  HMIDIOUT hmo,  UINT uPatch,  LPWORD pwkya,  UINT fuCache)
{
	mididebugprint("MIDIDEBUG: 'midiOutCacheDrumPatches' (%08x,%08x,%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) uPatch, (int) (void *) pwkya, (int) (void *) fuCache);
#ifdef USETHEIRS
	//int ret = midiOutCacheDrumPatches(hmo, uPatch, pwkya, fuCache);
	//mididebugprint("\t#%i#\n", ret);
	//return ret;

#endif

	return MMSYSERR_NOERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiOutGetID(  HMIDIOUT hmo,  LPUINT puDeviceID)
{
	mididebugprint("MIDIDEBUG: 'midiOutGetID' (%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) puDeviceID);
#ifdef USETHEIRS
	int ret = midiOutGetID(hmo, puDeviceID);
	mididebugprint("\t#%i#\n", ret);
	mididebugprint("\tOPEN:%08x\n", *puDeviceID);
	return ret;

#endif

	if (puDeviceID)
		*puDeviceID = 0;
	return MMSYSERR_NOERROR;
}

// connect an input to an output
HOOKFUNC MMRESULT WINAPI MymidiConnect(  HMIDI hmi,  HMIDIOUT hmo,  LPVOID pReserved)
{
	mididebugprint("MIDIDEBUG: 'midiConnect' (%08x,%08x,%08x,)\n", (int) (void *) hmi, (int) (void *) hmo, (int) (void *) pReserved);
#ifdef USETHEIRS
	int ret = midiConnect(hmi, hmo, pReserved);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	return MMSYSERR_NOERROR;
}
// reverse the above
HOOKFUNC MMRESULT WINAPI MymidiDisconnect(  HMIDI hmi,  HMIDIOUT hmo,  LPVOID pReserved)
{
	mididebugprint("MIDIDEBUG: 'midiDisconnect' (%08x,%08x,%08x,)\n", (int) (void *) hmi, (int) (void *) hmo, (int) (void *) pReserved);
#ifdef USETHEIRS
	int ret = midiDisconnect(hmi, hmo, pReserved);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	return MMSYSERR_NOERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiOutGetVolume(  HMIDIOUT hmo,  LPDWORD pdwVolume)
{
	mididebugprint("MIDIDEBUG: 'midiOutGetVolume' (%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) pdwVolume);
#ifdef USETHEIRS
	int ret = midiOutGetVolume(hmo, pdwVolume);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	MidiHandler *h = (MidiHandler *) hmo;
	CHECK_DEADBEEF;
	fprintf (h->output, "%d,##VOLUME##\n", detTimer.GetTicks());
	return MMSYSERR_NOERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiOutSetVolume(  HMIDIOUT hmo,  DWORD dwVolume)
{
	mididebugprint("MIDIDEBUG: 'midiOutSetVolume' (%08x,%08x,)\n", (int) (void *) hmo, (int) (void *) dwVolume);
#ifdef USETHEIRS
	int ret = midiOutSetVolume(hmo, dwVolume);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	MidiHandler *h = (MidiHandler *) hmo;
	CHECK_DEADBEEF;
	fprintf (h->output, "%d,##VOLUME##\n", detTimer.GetTicks());
	return MMSYSERR_NOERROR;
}









/* midi stream functions */

HOOKFUNC MMRESULT WINAPI MymidiStreamPause(  HMIDISTRM hms)
{
#ifdef USETHEIRS_STREAM
	mididebugprint("MIDIDEBUG: 'midiStreamPause' (%08x,)\n", (int) (void *) hms);
	int ret = midiStreamPause(hms);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	debugprintf("x attempted! (NYI)\n");
	return MMSYSERR_ERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiStreamClose(  HMIDISTRM hms)
{
#ifdef USETHEIRS_STREAM
	mididebugprint("MIDIDEBUG: 'midiStreamClose' (%08x,)\n", (int) (void *) hms);
	int ret = midiStreamClose(hms);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	debugprintf("x attempted! (NYI)\n");
	return MMSYSERR_ERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiStreamOpen(  LPHMIDISTRM phms,  LPUINT puDeviceID,  DWORD cMidi,  DWORD_PTR dwCallback,  DWORD_PTR dwInstance,  DWORD fdwOpen)
{
#ifdef USETHEIRS_STREAM
	mididebugprint("MIDIDEBUG: 'midiStreamOpen' (%08x,%08x,%08x,%08x,%08x,%08x,)\n", (int) (void *) phms, (int) (void *) puDeviceID, (int) (void *) cMidi, (int) (void *) dwCallback, (int) (void *) dwInstance, (int) (void *) fdwOpen);
	int ret = midiStreamOpen(phms, puDeviceID, cMidi, dwCallback, dwInstance, fdwOpen);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	debugprintf("midiStreamOpen attempted! (NYI)\n");
	return MMSYSERR_ERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiStreamOut(  HMIDISTRM hms,  LPMIDIHDR pmh,  UINT cbmh)
{
#ifdef USETHEIRS_STREAM
	mididebugprint("MIDIDEBUG: 'midiStreamOut' (%08x,%08x,%08x,)\n", (int) (void *) hms, (int) (void *) pmh, (int) (void *) cbmh);
	int ret = midiStreamOut(hms, pmh, cbmh);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	debugprintf("x attempted! (NYI)\n");
	return MMSYSERR_ERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiStreamProperty(  HMIDISTRM hms,  LPBYTE lppropdata,  DWORD dwProperty)
{
#ifdef USETHEIRS_STREAM
	mididebugprint("MIDIDEBUG: 'midiStreamProperty' (%08x,%08x,%08x,)\n", (int) (void *) hms, (int) (void *) lppropdata, (int) (void *) dwProperty);
	int ret = midiStreamProperty(hms, lppropdata, dwProperty);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	debugprintf("x attempted! (NYI)\n");
	return MMSYSERR_ERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiStreamPosition(  HMIDISTRM hms,  LPMMTIME lpmmt,  UINT cbmmt)
{
#ifdef USETHEIRS_STREAM
	mididebugprint("MIDIDEBUG: 'midiStreamPosition' (%08x,%08x,%08x,)\n", (int) (void *) hms, (int) (void *) lpmmt, (int) (void *) cbmmt);
	int ret = midiStreamPosition(hms, lpmmt, cbmmt);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	debugprintf("x attempted! (NYI)\n");
	return MMSYSERR_ERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiStreamRestart(  HMIDISTRM hms)
{
	mididebugprint("MIDIDEBUG: 'midiStreamRestart' (%08x,)\n", (int) (void *) hms);
#ifdef USETHEIRS_STREAM
	int ret = midiStreamRestart(hms);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	debugprintf("x attempted! (NYI)\n");
	return MMSYSERR_ERROR;
}

HOOKFUNC MMRESULT WINAPI MymidiStreamStop(  HMIDISTRM hms)
{
	mididebugprint("MIDIDEBUG: 'midiStreamStop' (%08x,)\n", (int) (void *) hms);
#ifdef USETHEIRS_STREAM
	int ret = midiStreamStop(hms);
	mididebugprint("\t#%i#\n", ret);
	return ret;

#endif

	debugprintf("x attempted! (NYI)\n");
	return MMSYSERR_ERROR;
}

void ApplyMidiIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, WINMM, midiOutClose),
		MAKE_INTERCEPT(1, WINMM, midiOutGetDevCapsA),
		MAKE_INTERCEPT(1, WINMM, midiOutGetDevCapsW),
		MAKE_INTERCEPT(1, WINMM, midiOutGetErrorTextA),
		MAKE_INTERCEPT(1, WINMM, midiOutGetErrorTextW),
		MAKE_INTERCEPT(1, WINMM, midiOutGetNumDevs),
		MAKE_INTERCEPT(1, WINMM, midiOutLongMsg),
		MAKE_INTERCEPT(1, WINMM, midiOutOpen),
		MAKE_INTERCEPT(1, WINMM, midiOutPrepareHeader),
		MAKE_INTERCEPT(1, WINMM, midiOutShortMsg),
		MAKE_INTERCEPT(1, WINMM, midiOutUnprepareHeader),
		MAKE_INTERCEPT(1, WINMM, midiOutReset),
		MAKE_INTERCEPT(1, WINMM, midiOutCachePatches),
		MAKE_INTERCEPT(1, WINMM, midiOutCacheDrumPatches),
		MAKE_INTERCEPT(1, WINMM, midiOutGetID),
		MAKE_INTERCEPT(1, WINMM, midiConnect),
		MAKE_INTERCEPT(1, WINMM, midiDisconnect),
		MAKE_INTERCEPT(1, WINMM, midiOutGetVolume),
		MAKE_INTERCEPT(1, WINMM, midiOutSetVolume),
		MAKE_INTERCEPT(1, WINMM, midiOutMessage),
		MAKE_INTERCEPT(1, WINMM, midiStreamPause),
		MAKE_INTERCEPT(1, WINMM, midiStreamClose),
		MAKE_INTERCEPT(1, WINMM, midiStreamOpen),
		MAKE_INTERCEPT(1, WINMM, midiStreamOut),
		MAKE_INTERCEPT(1, WINMM, midiStreamProperty),
		MAKE_INTERCEPT(1, WINMM, midiStreamPosition),
		MAKE_INTERCEPT(1, WINMM, midiStreamRestart),
		MAKE_INTERCEPT(1, WINMM, midiStreamStop),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

