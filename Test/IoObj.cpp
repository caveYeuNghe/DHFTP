#include "IoObj.h"
#include <stdio.h>
#include <MSWSock.h>
#include "Envar.h"

LPIO_OBJ getIoObject(IO_OBJ::OP operation, char * buffer, DWORD length) {
	LPIO_OBJ newobj = NULL;

	if ((newobj = (LPIO_OBJ)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IO_OBJ) + sizeof(char) * length)) == NULL)
		printf("HeapAlloc() failed with error %d\n", GetLastError());

	if (newobj) {
		newobj->operation = operation;

		newobj->buffer = (char *)(((char *)newobj) + sizeof(IO_OBJ));

		newobj->dataBuff.len = length;
		newobj->dataBuff.buf = newobj->buffer;

		if (buffer != NULL)
			memcpy_s(newobj->buffer, length, buffer, length);
	}

	return newobj;
}

void freeIoObject(LPIO_OBJ ioobj) {
	HeapFree(GetProcessHeap(), NULL, ioobj);
}

void IO_OBJ::setBufferSend(char *i_buffer) {
	strcpy_s(this->buffer, BUFFSIZE, i_buffer);
	this->dataBuff.buf = this->buffer;
	this->dataBuff.len = strlen(this->buffer);
}

void IO_OBJ::setBufferRecv(char *i_buffer) {
	strcpy_s(this->buffer, BUFFSIZE, i_buffer);
	int length = strlen(buffer);
	this->dataBuff.buf = this->buffer + length;
	this->dataBuff.len = BUFFSIZE - length;
}

void IO_OBJ::setFileOffset(LONG64 fileOffset) {
	this->overlapped.Offset = fileOffset & 0xFFFF'FFFF;
	this->overlapped.OffsetHigh = (fileOffset >> 32) & 0xFFFF'FFFF;
}

bool PostSend(SOCKET sock, LPIO_OBJ sendObj) {
	if ((WSASend(sock, &(sendObj->dataBuff), 1, NULL, 0, &(sendObj->overlapped), NULL)) == SOCKET_ERROR) {
		DWORD error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			printf("WSASend failed with error %d\n", error);
			return FALSE;
		}
	}

	return TRUE;
}

bool PostRecv(SOCKET sock, LPIO_OBJ recvObj) {
	DWORD flags = 0;
	if ((WSARecv(sock, &(recvObj->dataBuff), 1, NULL, &flags, &(recvObj->overlapped), NULL)) == SOCKET_ERROR) {
		DWORD error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			printf("WSARecv failed with error %d\n", error);
			return FALSE;
		}
	}

	return TRUE;
}

bool PostWrite(HANDLE hfile, LPIO_OBJ writeObj) {
	if (!WriteFile(hfile, writeObj->buffer, writeObj->dataBuff.len, NULL, &(writeObj->overlapped))) {
		DWORD error = WSAGetLastError();
		if (error != ERROR_IO_PENDING) {
			printf("WriteFile failed with error %d\n", error);
			return FALSE;
		}
	}

	return TRUE;
}

bool PostSendFile(SOCKET sock, HANDLE hfile, LPIO_OBJ sendFObj) {
	if (!TransmitFile(sock, hfile, sendFObj->dataBuff.len, 0, &(sendFObj->overlapped), NULL, 0)) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			printf("TransmitFile failed with error %d\n", error);
			return FALSE;
		}
	}
	return TRUE;
}



