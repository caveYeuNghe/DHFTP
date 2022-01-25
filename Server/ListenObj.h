#pragma once
#include <WinSock2.h>
#include <mswsock.h>

typedef struct {
	SOCKET sock;

	// Pointers to Microsoft specific extensions.
	LPFN_ACCEPTEX lpfnAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs;
} LISTEN_OBJ, *LPLISTEN_OBJ;

_Ret_maybenull_ LPLISTEN_OBJ getListenObj(_In_ WORD port);