// Specifies a thread for wich a hook routine was set by calling SetWindowsHookEx()
typedef struct _WND_THREAD
{
#if _DEBUG
	ULONG		Magic;
#endif
	LIST_ENTRY	Entry;		// Global thread list's entry
	HHOOK		hHook;		// Hook routine handle
	ULONG		ThreadId;	// Thread ID
} WND_THREAD, *PWND_THREAD;

#define WND_THREAD_MAGIC		'rhTK'
#define	ASSERT_WND_THREAD(x)	ASSERT(x->Magic == WND_THREAD_MAGIC)

WINERROR WndHookStartup(void);
void WndHookCleanup(void);
WINERROR WndHookOnThreadAttach(ULONG ThreadId);
WINERROR WndHookOnThreadDetach(ULONG ThreadId);

