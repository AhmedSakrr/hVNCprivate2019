#ifndef __BMP_H_
#define __BMP_H_

#define BLOCK_SIZE 64

typedef struct tagRGBTRIPLE  RGBTRIPLE, *PRGBTRIPLE, NEAR *NPRGBTRIPLE, FAR *LPRGBTRIPLE;

extern HBRUSH hFrameBrush,hBlackBrush;

HBITMAP 
	BmpCreateDibSection(
		HDC hDC,
		PBITMAP_INFO BmpInfo,
		void **lpBkgBits
		);
VOID BmpCopyScreenBuffer(PVNC_SESSION pSession,RECT *lpRect,BOOL bClient);
VOID BmpCopyRectFromBuffer( PVNC_SESSION pSession, PUCHAR lpScreen, PUCHAR lpTo, LPRECT Rect );
HRGN FastDetectChanges ( PVNC_SESSION pSession, LPRECT ClipRect, BOOL bDeepScan );
HBRUSH BmpGetBlackBrush( VOID );
WINERROR BmpGetPixelFormat( PBITMAP_INFO BitmapInfo, PPIXEL_FORMAT PixelFormat, int height, int width);
BOOL BmpSetPalette(HDC hDC, HBITMAP hBitmap, PBITMAP_INFO BitmapInfo);
BOOL BmpDetectBlackSceen ( PVNC_SESSION pSession );

VOID BmpInitiPainting( VOID );

#endif //__BMP_H_