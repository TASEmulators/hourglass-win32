// WIP CODE

#ifndef MIDITRAMPS_H_INCL
#define MIDITRAMPS_H_INCL

#define midiOutClose TrampmidiOutClose
TRAMPFUNC MMRESULT WINAPI midiOutClose(  HMIDIOUT hmo) TRAMPOLINE_DEF
#define midiOutGetDevCapsA TrampmidiOutGetDevCapsA
TRAMPFUNC MMRESULT WINAPI midiOutGetDevCapsA(  UINT_PTR uDeviceID,  LPMIDIOUTCAPSA pmoc,  UINT cbmoc) TRAMPOLINE_DEF
#define midiOutGetDevCapsW TrampmidiOutGetDevCapsW
TRAMPFUNC MMRESULT WINAPI midiOutGetDevCapsW(  UINT_PTR uDeviceID,  LPMIDIOUTCAPSW pmoc,  UINT cbmoc) TRAMPOLINE_DEF
#define midiOutGetErrorTextA TrampmidiOutGetErrorTextA
TRAMPFUNC MMRESULT WINAPI midiOutGetErrorTextA(  MMRESULT mmrError,  LPSTR pszText,  UINT cchText) TRAMPOLINE_DEF
#define midiOutGetErrorTextW TrampmidiOutGetErrorTextW
TRAMPFUNC MMRESULT WINAPI midiOutGetErrorTextW(  MMRESULT mmrError,  LPWSTR pszText,  UINT cchText) TRAMPOLINE_DEF
#define midiOutGetNumDevs TrampmidiOutGetNumDevs
TRAMPFUNC UINT WINAPI midiOutGetNumDevs(void) TRAMPOLINE_DEF
#define midiOutLongMsg TrampmidiOutLongMsg
TRAMPFUNC MMRESULT WINAPI midiOutLongMsg( HMIDIOUT hmo,  LPMIDIHDR pmh,  UINT cbmh) TRAMPOLINE_DEF
#define midiOutOpen TrampmidiOutOpen
TRAMPFUNC MMRESULT WINAPI midiOutOpen(  LPHMIDIOUT phmo,  UINT uDeviceID,  DWORD_PTR dwCallback,  DWORD_PTR dwInstance,  DWORD fdwOpen) TRAMPOLINE_DEF
#define midiOutPrepareHeader TrampmidiOutPrepareHeader
TRAMPFUNC MMRESULT WINAPI midiOutPrepareHeader(  HMIDIOUT hmo,  LPMIDIHDR pmh,  UINT cbmh) TRAMPOLINE_DEF
#define midiOutShortMsg TrampmidiOutShortMsg
TRAMPFUNC MMRESULT WINAPI midiOutShortMsg(  HMIDIOUT hmo,  DWORD dwMsg) TRAMPOLINE_DEF
#define midiOutUnprepareHeader TrampmidiOutUnprepareHeader
TRAMPFUNC MMRESULT WINAPI midiOutUnprepareHeader( HMIDIOUT hmo,  LPMIDIHDR pmh,  UINT cbmh) TRAMPOLINE_DEF

#define midiOutReset TrampmidiOutReset
TRAMPFUNC MMRESULT WINAPI midiOutReset(  HMIDIOUT hmo) TRAMPOLINE_DEF
#define midiOutCachePatches TrampmidiOutCachePatches
TRAMPFUNC MMRESULT WINAPI midiOutCachePatches(  HMIDIOUT hmo,  UINT uBank,  LPWORD pwpa,  UINT fuCache) TRAMPOLINE_DEF
#define midiOutCacheDrumPatches TrampmidiOutCacheDrumPatches
TRAMPFUNC MMRESULT WINAPI midiOutCacheDrumPatches(  HMIDIOUT hmo,  UINT uPatch,  LPWORD pwkya,  UINT fuCache) TRAMPOLINE_DEF
#define midiOutGetID TrampmidiOutGetID
TRAMPFUNC MMRESULT WINAPI midiOutGetID(  HMIDIOUT hmo,  LPUINT puDeviceID) TRAMPOLINE_DEF
#define midiConnect TrampmidiConnect
TRAMPFUNC MMRESULT WINAPI midiConnect(  HMIDI hmi,  HMIDIOUT hmo,  LPVOID pReserved) TRAMPOLINE_DEF
#define midiDisconnect TrampmidiDisconnect
TRAMPFUNC MMRESULT WINAPI midiDisconnect(  HMIDI hmi,  HMIDIOUT hmo,  LPVOID pReserved) TRAMPOLINE_DEF
#define midiOutGetVolume TrampmidiOutGetVolume
TRAMPFUNC MMRESULT WINAPI midiOutGetVolume(  HMIDIOUT hmo,  LPDWORD pdwVolume) TRAMPOLINE_DEF
#define midiOutSetVolume TrampmidiOutSetVolume
TRAMPFUNC MMRESULT WINAPI midiOutSetVolume(  HMIDIOUT hmo,  DWORD dwVolume) TRAMPOLINE_DEF

#define midiOutMessage TrampmidiOutMessage
TRAMPFUNC MMRESULT WINAPI midiOutMessage(  HMIDIOUT hmo,  UINT uMsg,  DWORD_PTR dw1,  DWORD_PTR dw2) TRAMPOLINE_DEF
#define midiStreamPause TrampmidiStreamPause
TRAMPFUNC MMRESULT WINAPI midiStreamPause(  HMIDISTRM hms) TRAMPOLINE_DEF


#define midiStreamClose TrampmidiStreamClose
TRAMPFUNC MMRESULT WINAPI midiStreamClose(  HMIDISTRM hms) TRAMPOLINE_DEF
#define midiStreamOpen TrampmidiStreamOpen
TRAMPFUNC MMRESULT WINAPI midiStreamOpen(  LPHMIDISTRM phms,  LPUINT puDeviceID,  DWORD cMidi,  DWORD_PTR dwCallback,  DWORD_PTR dwInstance,  DWORD fdwOpen) TRAMPOLINE_DEF
#define midiStreamOut TrampmidiStreamOut
TRAMPFUNC MMRESULT WINAPI midiStreamOut(  HMIDISTRM hms,  LPMIDIHDR pmh,  UINT cbmh) TRAMPOLINE_DEF
#define midiStreamProperty TrampmidiStreamProperty
TRAMPFUNC MMRESULT WINAPI midiStreamProperty(  HMIDISTRM hms,  LPBYTE lppropdata,  DWORD dwProperty) TRAMPOLINE_DEF
#define midiStreamPosition TrampmidiStreamPosition
TRAMPFUNC MMRESULT WINAPI midiStreamPosition(  HMIDISTRM hms,  LPMMTIME lpmmt,  UINT cbmmt) TRAMPOLINE_DEF
#define midiStreamRestart TrampmidiStreamRestart
TRAMPFUNC MMRESULT WINAPI midiStreamRestart(  HMIDISTRM hms) TRAMPOLINE_DEF
#define midiStreamStop TrampmidiStreamStop
TRAMPFUNC MMRESULT WINAPI midiStreamStop(  HMIDISTRM hms) TRAMPOLINE_DEF




#endif
