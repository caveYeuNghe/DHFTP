#pragma once

#define SERVER_ADDR "127.0.0.1"
#define CMD_PORT 5500
#define FILE_PORT 6600
#define BUFFSIZE 4096
#define TRANSMITFILE_MAX ((2<<30) - 1)

#define ENDING_DELIMITER "\r\n"
#define HEADER_DELIMITER "\r"
#define PARA_DELIMITER " "

#define LOGIN "LOGI"
#define LOGOUT "LOGO"
#define REGISTER "REG"
#define RETRIEVE "RETR"
#define STORE "STOR"
#define RENAME "RN"
#define DELETEFILE "DEL"
#define MAKEDIR "MKD"
#define REMOVEDIR "RMD"
#define CHANGEWDIR "CWD"
#define PRINTWDIR "PWD"
#define LISTDIR "LIST"
#define RESPONE "RES"
#define RECEIVE "RECV"
#define CONNECT "CNCT"

extern sockaddr_in gCmdAddr;
extern sockaddr_in gFileAddr;

enum REPLY_CODE {
	LOGIN_SUCCESS = 110,
	LOGOUT_SUCCESS = 111,
	REGISTER_SUCCESS = 112,

	NOT_LOGIN = 310,
	ALREADY_LOGIN = 311,
	USER_NOT_EXIST = 312,
	USER_ALREADY_EXIST = 313,
	WRONG_PASSWORD = 314,
	EMPTY_FIELD = 315,

	RETRIEVE_SUCCESS = 220,
	STORE_SUCCESS = 221,
	FINISH_SEND = 120,
	RENAME_SUCCESS = 121,
	DELETE_SUCCESS = 122,
	MAKEDIR_SUCCESS = 123,
	REMOVEDIR_SUCCESS = 124,
	CHANGEWDIR_SUCCESS = 125,
	PRINTWDIR_SUCCESS = 126,
	LIST_SUCCESS = 127,

	NO_ACCESS = 320,
	FILE_NOT_EXIST = 321,
	FILE_ALREADY_EXIST = 322,
	NAME_WRONG_FORMAT = 323,
	TRANSMIT_FAIL = 324,
	DIR_NOT_EMPTY = 325,

	WRONG_SYNTAX = 330,
	SERVER_FAIL = 331
};
