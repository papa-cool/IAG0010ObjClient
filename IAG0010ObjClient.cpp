// IAG0010ObjClient.cpp : définit le point d'entrée pour l'application console.
//

#include "stdafx.h"
#include "IAG0010ObjClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Seul et unique objet application

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// Initialise MFC et affiche un message d'erreur en cas d'échec
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: modifiez le code d'erreur selon les besoins
			_tprintf(_T("Erreur irrécupérable : l'initialisation MFC a échoué\n"));
			nRetCode = 1;
		}
		else
		{
			// TODO: codez le comportement de l'application à cet emplacement.

			// Windsock initialization.
			WSADATA WSAData;
			WSAStartup(MAKEWORD(2,2), &WSAData);


			// Events initialization.
			CEvent* ptrDataRecvEvent = new CEvent(false, true , NULL, NULL); // A packet has been received
			CEvent* ptrDataSentEvent = new CEvent(true, true , NULL, NULL); // A packet has been sent
			CEvent* ptrStopEvent = new CEvent(false, true , NULL, NULL); // Client order to stop application

			// This variable indicate if the downloading has completed.
			BOOL downloadingCompleted = false;

			// ClientSocket creation and connection.
			ClientSocket* ptrClientSocket = new ClientSocket(ptrStopEvent, &downloadingCompleted); // Socket to communicate with the server
			if (ptrClientSocket->openConnection() == 1){
				getchar();
				return 1;
			}

			// File creation
			HANDLE file; // File received by downloading
			file = CreateFile(_T("IAG0010ObjClient_pohikiri.doc"),GENERIC_READ | GENERIC_WRITE, 0, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file == INVALID_HANDLE_VALUE) {
				_tcout << "Unable to create file, error " << GetLastError() << endl;
				getchar();
				return 1;
			}

			// Thread creation and starting.
			ReadingKeyboardThread* ptrReadingKeyboardThread = new ReadingKeyboardThread(ptrStopEvent); // Thread to received the "exit" command throw the terminal.
			ptrReadingKeyboardThread->CreateThread(); // Call Run.
			ReceivingThread* ptrReceivingThread = new ReceivingThread(ptrClientSocket, ptrDataRecvEvent, ptrDataSentEvent, ptrStopEvent, &downloadingCompleted, &file); // Thread to receive data from the server.
			ptrReceivingThread->CreateThread(); // Call Run.
			SendingThread* ptrSendingThread = new SendingThread(ptrClientSocket, ptrDataRecvEvent, ptrDataSentEvent, ptrStopEvent, &downloadingCompleted); // Thread to send data to the server.
			ptrSendingThread->CreateThread(); // Call Run.

			CSingleLock* ptrStopEventLock = new CSingleLock(ptrStopEvent); // Wait stopEvent.

			// Application closing

			ptrStopEventLock->Lock(INFINITE);
			Sleep(5000);
			ptrClientSocket->closeConnection();
			CloseHandle(file);
			WSACleanup();
			if(downloadingCompleted)
				system("IAG0010ObjClient_pohikiri.doc");


			// TODO: Until there.
		}
	}
	else
	{
		// TODO: modifiez le code d'erreur selon les besoins
		_tprintf(_T("Erreur irrécupérable : échec de GetModuleHandle\n"));
		nRetCode = 1;
	}

	return nRetCode;
}
