#include "FileObj.h"
#include <stdio.h>

LPFILEOBJ GetFileObj(HANDLE hfile, LONG64 size, FILEOBJ::OP op) {
	LPFILEOBJ newobj = NULL;

	if ((newobj = (LPFILEOBJ)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FILEOBJ))) == NULL)
		printf("HeapAlloc() failed with error %d\n", GetLastError());

	if (newobj) {
		newobj->file = hfile;
		newobj->size = size;
		newobj->operation = op;
	}

	return newobj;
}

void FreeFileObj(LPFILEOBJ fileobj) {
	//close file connection
	if (fileobj->fileSock != 0) {
		printf("Closing file socket %d\n", fileobj->fileSock);
		if (closesocket(fileobj->fileSock) == SOCKET_ERROR) {
			printf("closesocket failed with error %d\n", WSAGetLastError());
		}
	}
	//close file handle
	printf("Closing file handle\n");
	if (!CloseHandle(fileobj->file)) {
		printf("CloseHandle failed with error %d\n", GetLastError());
	}

	HeapFree(GetProcessHeap(), NULL, fileobj);
}


