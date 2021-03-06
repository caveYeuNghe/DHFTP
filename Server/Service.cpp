#include "Service.h"
#include <WinSock2.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <codecvt>
#include "EnvVar.h"
#include "IoObj.h"
#include "FileObj.h"

using namespace std;

void handleLOGIN(LPSESSION session, const char *username,const char *password, char *reply) {
	string userName = username;
	string passWord = password;

	//Check if logged in
	if (strlen(session->username) != 0) {
		initParam(reply, ALREADY_LOGIN, "Login failed. Already log in");
		return;
	}

	//Check if password or username field is empty
	if (userName.size() == 0 || passWord.size() == 0) {
		initParam(reply, WRONG_SYNTAX, "Login failed. Empty field");
		return;
	}

	SQLCHAR sqlUsername[50];
	SQLCHAR sqlPassword[50];
	SQLCHAR sqlStatus[50];

	EnterCriticalSection(&gCriticalSection);
	//Query select: to get data from username
	string query = "SELECT * FROM Account where username='" + userName + "'";

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server" << endl;
		cout << query;
	}
	else {
		if (SQLFetch(gSqlStmtHandle) == SQL_SUCCESS) {
			SQLGetData(gSqlStmtHandle, 1, SQL_CHAR, sqlUsername, sizeof(sqlUsername), NULL);
			SQLGetData(gSqlStmtHandle, 2, SQL_CHAR, sqlPassword, sizeof(sqlPassword), NULL);
			SQLGetData(gSqlStmtHandle, 3, SQL_CHAR, sqlStatus, sizeof(sqlStatus), NULL);

			string strSqlPassword = reinterpret_cast<char*>(sqlPassword);
			string strSqlStatus = reinterpret_cast<char*>(sqlStatus);

			SQLCloseCursor(gSqlStmtHandle);

			//Check passwords
			if (passWord == strSqlPassword) {
				if (strSqlStatus == "0") {
					query = "UPDATE Account SET status = 1 WHERE username='" + userName + "';";
					if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
						cout << "Error querying SQL Server";
						cout << "\n";
					}
					else {
						session->setUsername(username);
						session->setWorkingDir(username);
						initParam(reply, LOGIN_SUCCESS, "Login success");
					}
				}
				else {
					initParam(reply, ALREADY_LOGIN, "Login failed. ACcount already logged in another session");
				}
			}
			else {
				initParam(reply, WRONG_PASSWORD, "Login failed. Wrong password");
			}
		}
		else {
			initParam(reply, USER_NOT_EXIST, "Login failed. Username doesn't exist");
			SQLCloseCursor(gSqlStmtHandle);
		}
	}

	LeaveCriticalSection(&gCriticalSection);
}

void handleChangePass(LPSESSION session, const char *oldpass, const char *newpass, char * reply) {
	string userName = session->username;

	//Check if logged in
	if (userName.length() == 0) {
		initParam(reply, NOT_LOGIN, "ChangePass failed. Didn't log in");
		return;
	}
	string oldPass = oldpass, newPass = newpass;

	//Check if passwords entered are empty
	if (oldPass.length() == 0 || newPass.length() == 0) {
		initParam(reply, WRONG_SYNTAX, "ChangePass failed. Empty password");
		return;
	}

	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	EnterCriticalSection(&gCriticalSection);
	//Query select to get data 
	string query = "SELECT * FROM Account where username='" + userName + "'";

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server" << endl;
		LeaveCriticalSection(&gCriticalSection);
		return;
	}

	SQLWCHAR sqlPassword[50];

	if (SQLFetch(gSqlStmtHandle) == SQL_SUCCESS) {
		SQLGetData(gSqlStmtHandle, 2, SQL_CHAR, sqlPassword, sizeof(sqlPassword), NULL);
		string strSqlPassword = reinterpret_cast<char*>(sqlPassword);
		SQLCloseCursor(gSqlStmtHandle);

		if (oldPass == strSqlPassword) {
			//Query update: update password
			query = "UPDATE Account SET password = " + newPass + " WHERE username='" + userName + "';";
			if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
				cout << "Error querying SQL Server";
				cout << "\n";
			}
			else {
				initParam(reply, CHANGEPASS_SUCCESS, "Password changed successfully");
			}
		}
		else {
			initParam(reply, WRONG_PASSWORD, "Wrong old password");
		}
	}
	else {
		SQLCloseCursor(gSqlStmtHandle);
	}

	LeaveCriticalSection(&gCriticalSection);
}

void handleLOGOUT(LPSESSION session, char *reply) {
	string username = session->username;

	//Check if logged in
	if (username.length() == 0) {
		initParam(reply, NOT_LOGIN, "Logout failed. Didn't log in");
		return;
	}

	EnterCriticalSection(&gCriticalSection);
	//Query select to get data
	string query = "SELECT * FROM Account where username='" + username + "'";

	if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
		cout << "Error querying SQL Server";
		cout << "\n";
	}

	SQLCHAR sqlStatus[50];

	if (SQLFetch(gSqlStmtHandle) == SQL_SUCCESS) {
		SQLGetData(gSqlStmtHandle, 3, SQL_CHAR, sqlStatus, sizeof(sqlStatus), NULL);

		string strSqlStatus = reinterpret_cast<char*>(sqlStatus);

		SQLCloseCursor(gSqlStmtHandle);

		if (strSqlStatus == "1") {
			//Query update: to update online status to false (Not online)
			query = "UPDATE Account SET status = 0 WHERE username='" + username + "';";
			if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
				cout << "Error querying SQL Server";
				cout << "\n";
			}
			else {
				//Reset session
				session->setUsername("");
				session->setWorkingDir("");

				initParam(reply, LOGOUT_SUCCESS, "Logout successful");
			}
		}
		else {
			initParam(reply, NOT_LOGIN, "Logout failed. Didn't log in");
		}
	}
	else {
		SQLCloseCursor(gSqlStmtHandle);
	}

	LeaveCriticalSection(&gCriticalSection);
}

void handleREGISTER(const char *username, const char *password, char* reply) {
	string userName = username;
	string passWord = password;

	if (userName.size() == 0 || passWord.size() == 0) {
		initParam(reply, WRONG_SYNTAX, "Register failed. Empty field");
		return;
	}

	EnterCriticalSection(&gCriticalSection);

	if (!CreateDirectoryA(username, NULL)) {
		int error = GetLastError();
		printf("CreateDirectoryA() failed with error %d\n", error);
		if (error == ERROR_ALREADY_EXISTS)
			initParam(reply, USER_ALREADY_EXIST, "Register failed. Username already exists");
		else if (error == ERROR_INVALID_NAME) 
			initParam(reply, WRONG_SYNTAX, "Register failed. Invalid username");
		else
			initParam(reply, SERVER_FAIL, "CreateDirectoryA() failed");
	}
	else {
		string query;
		query = "INSERT INTO Account VALUES ('" + userName + "','" + passWord + "',0)";

		//Query insert: new account
		if (SQL_SUCCESS != SQLExecDirect(gSqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS)) {
			initParam(reply, USER_ALREADY_EXIST, "Register failed. Username already exists");
			RemoveDirectoryA(username);
		}
		else
			initParam(reply, REGISTER_SUCCESS, "Register success");
	}

	LeaveCriticalSection(&gCriticalSection);
}

void handleRETRIVE(LPSESSION session, const char *filename, char *reply) {
	LPFILEOBJ fileobj;
	HANDLE hFile;
	LARGE_INTEGER fileSize;
	char fullPath[MAX_PATH];

	//havent login
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Retrive failed. Didn't log in");
		return;
		session->setUsername("test");
		session->setWorkingDir("test");
	}

	//Check access and get full path
	if (!checkAccess(session, filename, fullPath)) {
		initParam(reply, NO_ACCESS, "Retrive failed. Dont have access to this file");
		return;
	}

	//previous file wasnt closed
	EnterCriticalSection(&session->cs);

	if (session->fileobj != NULL) {
		session->closeFile(TRUE);
		initParam(reply, SERVER_FAIL, "Store failed. Transmitting one file at a time");
		LeaveCriticalSection(&session->cs);
		return;
	}
	

	//Open existing file
	hFile = CreateFileA(fullPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", error);
		if (error == ERROR_FILE_NOT_FOUND)
			initParam(reply, NOT_EXIST, "Retrive failed. File doesnt exist");
		else if (error == ERROR_PATH_NOT_FOUND)
			initParam(reply, NOT_EXIST, "Retrive failed. Path doesnt exist");
		else if (error == ERROR_SHARING_VIOLATION)
			initParam(reply, FILE_BUSY, "Retrive failed. File is being used by another process");
		else
			initParam(reply, SERVER_FAIL, "Retrive failed. CreateFile failed");
		
		LeaveCriticalSection(&session->cs);
		return;
	}
	
	LeaveCriticalSection(&session->cs);

	GetFileSizeEx(hFile, &fileSize);
	fileobj = GetFileObj(hFile, fileSize.QuadPart, FILEOBJ::RETR);
	if (fileobj == NULL) {
		initParam(reply, SERVER_FAIL, "Retrive failed. Out of memory");
		return;
	}

	session->fileobj = fileobj;

	//Set accept event of file listen socket
	EnterCriticalSection(&gCriticalSection);

	gFileListen->count++;
	WSASetEvent(gFileListen->acceptEvent);

	LeaveCriticalSection(&gCriticalSection);

	initParam(reply, RETRIEVE_SUCCESS, (ULONG_PTR)session, fileobj->size);
}

void handleSTORE(LPSESSION session, const char * filename, const char *fileSize, char *reply) {
	LPFILEOBJ fileobj;
	LONG64 size;
	char fullPath[MAX_PATH];

	//havent login
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Store failed. Didn't log in");
		return;
		session->setUsername("test");
		session->setWorkingDir("test");
	}

	//Check param
	size = _atoi64(fileSize);
	if (size == 0) {
		initParam(reply, WRONG_SYNTAX, "Store failed. Invalid file size");
		return;
	}

	//Check access and get full path
	if (!checkAccess(session, filename, fullPath)) {
		initParam(reply, NO_ACCESS, "Store failed. Dont have access to this file");
		return;
	}

	//previous file wasnt closed
	EnterCriticalSection(&session->cs);
	if (session->fileobj != NULL) {
		session->closeFile(TRUE);
		initParam(reply, SERVER_FAIL, "Store failed. Transmitting one file at a time");
		LeaveCriticalSection(&session->cs);
		return;
	}
	

	//creat new file
	HANDLE hFile = CreateFileA(fullPath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, FILE_FLAG_OVERLAPPED, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", error);
		if (error == ERROR_FILE_EXISTS)
			initParam(reply, ALREADY_EXIST, "Store failed. File already exsit");
		else if (error == ERROR_PATH_NOT_FOUND)
			initParam(reply, NOT_EXIST, "Store failed. Path doesnt exist");
		else if (error == ERROR_SHARING_VIOLATION)
			initParam(reply, FILE_BUSY, "Store failed. File is being used by another process");
		else if (error == ERROR_INVALID_NAME)
			initParam(reply, WRONG_SYNTAX, "Store failed. Invalid file name");
		else
			initParam(reply, SERVER_FAIL, "Store failed. CreateFile failed");

		LeaveCriticalSection(&session->cs);
		return;
	}
	
	LeaveCriticalSection(&session->cs);
	
	//Associates the file hanlde for writing
	if (CreateIoCompletionPort(hFile, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		initParam(reply, SERVER_FAIL, "Store failed. CreateIoCompletionPort() failed");
		return;
	}

	fileobj = GetFileObj(hFile, size, FILEOBJ::STOR);
	if (fileobj == NULL) {
		initParam(reply, SERVER_FAIL," Store failed. Out of memory");
		return;
	}

	session->fileobj = fileobj;

	//Set accept event of file listen socket
	EnterCriticalSection(&gCriticalSection);

	gFileListen->count++;
	WSASetEvent(gFileListen->acceptEvent);

	LeaveCriticalSection(&gCriticalSection);


	initParam(reply, STORE_SUCCESS, (ULONG_PTR)session);
}

void handleMOVE(LPSESSION session, const char *oldpath, const char *newpath, char *reply) {
	char fullOldPath[MAX_PATH];
	char fullNewPath[MAX_PATH];

	//Check if logged in
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Move failed. Didn't log in");
		return;
	}

	//Check validity of directory entered
	if (!checkAccess(session, oldpath, fullOldPath) || !checkAccess(session, newpath, fullNewPath)) {
		initParam(reply, NO_ACCESS, "Move failed. Dont have access to this directory");
		return;
	}

	EnterCriticalSection(&session->cs);
	//Move File
	if (MoveFileA(fullOldPath, fullNewPath)) {
		initParam(reply, RENAME_SUCCESS, "Move successful");
	}
	else {
		int error = GetLastError();
		printf("MoveFileA failed with error %d\n", error);
		if (error == ERROR_ALREADY_EXISTS)
			initParam(reply,ALREADY_EXIST, "Move failed. Name already exists");
		else if (error == ERROR_FILE_NOT_FOUND)
			initParam(reply, NOT_EXIST, "Move failed. File not found");
		else if (error == ERROR_PATH_NOT_FOUND)
			initParam(reply, NOT_EXIST, "Move failed. Path not found");
		else if (error == ERROR_SHARING_VIOLATION)
			initParam(reply, FILE_BUSY, "Move failed. File is being used by another process");
		else if (error == ERROR_INVALID_NAME)
			initParam(reply, WRONG_SYNTAX, "Move failed. Invalid file name");
		else
			initParam(reply, SERVER_FAIL, "Move failed. MoveFile failed");
	}

	LeaveCriticalSection(&session->cs);
}

void handleDELETE(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];
	char rootPath[MAX_PATH];

	//Check if logged in
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Delete file failed. Didn't log in");
		return;
	}

	//Check validity of directory entered
	if (!checkAccess(session, pathname, fullPath) || !checkAccess(session, HOME, rootPath)) {
		initParam(reply, NO_ACCESS, "Delete file failed. Dont have access to this directory");
		return;
	}
	
	EnterCriticalSection(&session->cs);
	//Delete file
	if (DeleteFileA(fullPath)) {
		initParam(reply, DELETE_SUCCESS, "Delete file successfull");
	}
	else {
		int error = GetLastError();
		printf("DeleteFile failed with error %d\n", error);
		if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
			initParam(reply, NOT_EXIST, "Delete file failed. File doesn't exist");
		else if (error == ERROR_SHARING_VIOLATION)
			initParam(reply, FILE_BUSY, "Delete file failed. File is being used by another process");
		else if (error == ERROR_ACCESS_DENIED)
			initParam(reply, WRONG_FORMAT, "Delete file failed. Not file");
		else 
			initParam(reply, SERVER_FAIL, "Delete file failed.");
	}

	LeaveCriticalSection(&session->cs);
}

void handleMAKEDIR(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];

	//Check if logged in
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Make dir failed. Didn't log in");
		return;
	}

	//Check validity of directory entered
	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "Make dir failed. Dont have access to this directory");
		return;
	}

	if (CreateDirectoryA(fullPath, NULL)) {
		initParam(reply, MAKEDIR_SUCCESS, "Directory created successfully");
	}
	else {
		int error = GetLastError();
		printf("CreateDirectory failed with error %d\n", error);
		if (error == ERROR_PATH_NOT_FOUND)
			initParam(reply, NOT_EXIST, "Makde dir failed. Path not found");
		else if (error == ERROR_ALREADY_EXISTS)
			initParam(reply, ALREADY_EXIST, "Makde dir failed. Path already exists");
		else if (error == ERROR_INVALID_NAME)
			initParam(reply, WRONG_SYNTAX, "Makde dir failed. Invalid directory name");
		else
			initParam(reply, SERVER_FAIL, "CreateDirectory failed");
	}
}

void handleREMOVEDIR(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];
	char rootPath[MAX_PATH];

	//Check if logged in
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Remove directory failed. Didn't log in");
		return;
	}

	//Check validity of directory entered
	if (!checkAccess(session, pathname, fullPath) || !checkAccess(session, HOME, rootPath)) {
		initParam(reply, NO_ACCESS, "Remove directory failed. Dont have access to this directory");
		return;
	}

	if (!strcmp(rootPath, fullPath)) {
		initParam(reply, WRONG_SYNTAX, "Remove directory failed. Cant delete root directory");
		return;
	}

	//Remove directory
	EnterCriticalSection(&session->cs);

	if (RemoveDirectoryA(fullPath)) {
		initParam(reply, REMOVEDIR_SUCCESS, "Directory removed successfully");
	}
	else {
		int error = GetLastError();
		printf("RemoveDirectory failed with error %d\n", error);
		if (error == ERROR_DIR_NOT_EMPTY) {
			initParam(reply, DIR_NOT_EMPTY, "Remove directory failed. Not empty");
		}
		else if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
			initParam(reply, NOT_EXIST, "Remove directory failed. Path not found");
		}
		else if (error == ERROR_DIRECTORY) {
			initParam(reply, WRONG_FORMAT, "Remove directory failed. Not directory");
		}
		else
			initParam(reply, SERVER_FAIL, "RemoveDirectory failed");
	}

	LeaveCriticalSection(&session->cs);
}

void handleCHANGEWDIR(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];
	char fileData[BUFFSIZE];
	HANDLE find;

	//Check if logged in
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Change working directory failed. Didn't log in");
		return;
	}

	//Check length of path
	if (strlen(pathname) == 0 || strlen(pathname) > MAX_PATH) {
		initParam(reply, WRONG_SYNTAX, "Change working directory failed. Invalid path");
		return;
	}

	//Check validity of directory entered
	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "Change working directory failed. Dont have access to this directory");
		return;
	}

	//Change working directory
	find = FindFirstFileExA(
		fullPath,
		FindExInfoBasic,
		fileData,
		FindExSearchLimitToDirectories,
		NULL,
		FIND_FIRST_EX_CASE_SENSITIVE);

	if (find == INVALID_HANDLE_VALUE)
		initParam(reply, NOT_EXIST, "Change working directory failed. Path not found");
	else {
		session->setWorkingDir(fullPath);
		initParam(reply, CHANGEWDIR_SUCCESS, "Directory changed successfully");
	}

	FindClose(find);
}

void handlePRINTWDIR(LPSESSION session, char *reply) {
	char workingDir[MAX_PATH] = "";
	char rootPath[MAX_PATH] = "", workingPath[MAX_PATH] = "";
	DWORD rootlength;

	//Check if logged in
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "Print working directory failed. Didn't log in");
		return;
	}

	//Get full root
	rootlength = GetFullPathNameA(session->username, MAX_PATH, rootPath, NULL);
	//Get full working dir
	checkAccess(session, ".", workingPath);

	//Get short form of working dir
	sprintf_s(workingDir, MAX_PATH, "%s%s", HOME, strstr(workingPath, rootPath) + strlen(rootPath));

	initParam(reply, PRINTWDIR_SUCCESS, workingDir);
}

void handleLISTDIR(LPSESSION session, const char *pathname, char *reply) {
	char fullPath[MAX_PATH];

	//Check if logged in
	if (strlen(session->username) == 0) {
		initParam(reply, NOT_LOGIN, "List directory failed. Didn't log in");
		return;
	}

	//Check length of path
	if (strlen(pathname) == 0 || strlen(pathname) > MAX_PATH) {
		initParam(reply, WRONG_SYNTAX, "List directory failed. Invalid path");
		return;
	}

	//Check validity of directory entered
	if (!checkAccess(session, pathname, fullPath)) {
		initParam(reply, NO_ACCESS, "List directory failed. Dont have access to this directory");
		return;
	}

	string pathName = fullPath;
	pathName += "\\*";
	string names;

	WIN32_FIND_DATAA data;
	HANDLE hFind = FindFirstFileA(pathName.c_str(), &data);     

	//Get file and folder names in directory 
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			string name = data.cFileName;
			if (name != "." && name != "..") {
				name += "\n";
				names += name;
			}
		} while (FindNextFileA(hFind, &data));
		
		initParam(reply, LIST_SUCCESS, names.c_str());
	}
	else {
		initParam(reply, NOT_EXIST, "List directory failed. Invalid path");
	}

	FindClose(hFind);
}

void HandleMess(LPSESSION session, char *mess, char *reply) {
	std::vector<std::string> para;
	char cmd[BUFFSIZE] = "", res[BUFFSIZE] = "";
	const char *p1, *p2;

	//Parse message
	newParseMess(mess, cmd, para);

	if (!strcmp(cmd, LOGIN)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleLOGIN(session, p1, p2, res);
		}
	}
	else if (!strcmp(cmd, LOGOUT)) {
		if (para.size() != 0) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			handleLOGOUT(session, res);
		}
	}
	else if (!strcmp(cmd, REGISTER)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleREGISTER(p1, p2, res);
		}
	}
	else if (!strcmp(cmd, CHANGEPASS)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleChangePass(session, p1, p2, res);
		}
	}
	else if (!strcmp(cmd, STORE)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleSTORE(session, p1, p2, res);
		}
	}
	else if (!strcmp(cmd, RETRIEVE)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleRETRIVE(session, p1, res);
		}
	}
	else if (!strcmp(cmd, MOVE)) {
		if (para.size() != 2) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			p2 = para[1].c_str();
			handleMOVE(session, p1, p2, res);
		}
	}
	else if (!strcmp(cmd, DELETEFILE)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleDELETE(session, p1, res);
		}
	}
	else if (!strcmp(cmd, MAKEDIR)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleMAKEDIR(session, p1, res);
		}
	}
	else if (!strcmp(cmd, REMOVEDIR)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleREMOVEDIR(session, p1, res);
		}
	}
	else if (!strcmp(cmd, CHANGEWDIR)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleCHANGEWDIR(session, p1, res);
		}
	}
	else if (!strcmp(cmd, PRINTWDIR)) {
		if (para.size() != 0) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			handlePRINTWDIR(session, res);
		}
	}
	else if (!strcmp(cmd, LISTDIR)) {
		if (para.size() != 1) {
			initParam(res, WRONG_SYNTAX, "Wrong number of parameters");
		}
		else {
			p1 = para[0].c_str();
			handleLISTDIR(session, p1, res);
		}
	}
	else
		initParam(res, WRONG_SYNTAX, "Wrong header");

	initMessage(reply, RESPONE, res);
}

bool connectSQL(const char *db, const char *username, const char *password) {
	SQLHANDLE sqlConnHandle;
	SQLHANDLE sqlEnvHandle;
	SQLCHAR retconstring[SQL_RETURN_CODE_LEN];

	sqlConnHandle = NULL;
	gSqlStmtHandle = NULL;

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle))
		cout << "Alloc failed" << endl;
	if (SQL_SUCCESS != SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))
		cout << "Set env failed" << endl;
	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle))
		cout << "Alloc failed" << endl;

	cout << "Attempting connection to SQL Server...";
	cout << "\n";

	std::ostringstream connStr;
	connStr << "DRIVER={SQL Server};"
		<< "SERVER=localhost,1433;"
		<< "DATABASE=" << db << ";"
		<< "UID=" << username << ";"
		<< "PWD=" << password << ";";

	switch (SQLDriverConnect(sqlConnHandle,
		NULL,
		(SQLCHAR*)connStr.str().c_str(),
		SQL_NTS,
		retconstring,
		SQL_RETURN_CODE_LEN,
		NULL,
		SQL_DRIVER_NOPROMPT))
	{
	case SQL_SUCCESS:
		cout << "Successfully connected to SQL Server";
		cout << "\n";
		break;
	case SQL_SUCCESS_WITH_INFO:
		cout << "Successfully connected to SQL Server";
		cout << "\n";
		break;
	case SQL_INVALID_HANDLE:
		cout << "Could not connect to SQL Server";
		cout << "\n";
	case SQL_ERROR:
		cout << "Could not connect to SQL Server";
		cout << "\n";
	default:
		break;
	}

	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &gSqlStmtHandle)) {
		cout << "Alloc handle failed";
		return FALSE;
	}

	return TRUE;
}

void newParseMess(char *mess, char *cmd, std::vector<std::string> &para) {
	string strMess = mess;
	string strCmd, strP;
	int crPos, spPos, lenStr;

	lenStr = strMess.length();
		
	crPos = strMess.find(HEADER_DELIMITER);

	if (crPos == -1) {
		strcpy_s(cmd, BUFFSIZE, strMess.c_str());
	}
	else {
		strCmd = strMess.substr(0, crPos);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());

		strP = strMess.substr(crPos + strlen(HEADER_DELIMITER), lenStr - crPos - strlen(HEADER_DELIMITER));
		spPos = strP.find(PARA_DELIMITER);

		while (1) {
			spPos = strP.find(PARA_DELIMITER);

			if (spPos == -1) {
				para.push_back(strP);
				break;
			}
			else {
				para.push_back(strP.substr(0, spPos));
				strP = strP.substr(spPos + strlen(PARA_DELIMITER), strP.length() - spPos - strlen(PARA_DELIMITER));
			}
		}
	}
}

bool checkAccess(LPSESSION session, _In_ const char *path, _Out_ char *fullPath) {
	char rootPath[MAX_PATH];
	char temp[MAX_PATH];

	if (strstr(path, HOME) == path)
		sprintf_s(temp, MAX_PATH, "%s%s", session->username, strlen(path) > 1 ? path + 1 : "");
	else
		sprintf_s(temp, MAX_PATH, "%s%s%s", session->workingDir, "\\", path);

	DWORD rootLength = GetFullPathNameA(session->username, MAX_PATH, rootPath, NULL);
	DWORD pathLength = GetFullPathNameA(temp, MAX_PATH, fullPath, NULL);

	if (rootLength != 0 && pathLength != 0 && strstr(fullPath, rootPath) != rootPath) {
		return TRUE;
	}
	
	//path invalid or dont start root path
	strcpy_s(fullPath, MAX_PATH, "");
	return FALSE;
}

void initParam(_Out_ char *param) {
	strcpy_s(param, BUFFSIZE, "");
}

template <typename P>
void initParam(_Out_ char *param, _In_ P p) {
	std::ostringstream sstr;

	//param + para
	sstr << p;
	strcat_s(param, BUFFSIZE, sstr.str().c_str());
}

template <typename P, typename... Args>
void initParam(_Out_ char *param, _In_ P p, _In_opt_ Args... paras) {
	std::ostringstream sstr;

	//param + para + para delimiter
	sstr << p << PARA_DELIMITER;
	strcat_s(param, BUFFSIZE, sstr.str().c_str());

	//recursion
	initParam(param, paras...);
}