#include "stdafx.h"
#include "ClientSocket.h"


ClientSocket::ClientSocket(CEvent* ptrStopEvent, BOOL* ptrDownloadingCompleted): ptrStopEvent(ptrStopEvent), ptrDownloadingCompleted(ptrDownloadingCompleted) {
	ptrRecvOverlapped = new WSAOVERLAPPED();
	ptrSendOverlapped = new WSAOVERLAPPED();
	ptrWSARecvCompletedEvents[0] = ptrStopEvent;
	ptrWSASendCompletedEvents[0] = ptrStopEvent;
	ptrWSARecvCompletedEvents[1] = ptrWSARecvCompletedEvent = new CEvent(false, true, NULL, NULL);
	ptrWSASendCompletedEvents[1] = ptrWSASendCompletedEvent = new CEvent(false, true, NULL, NULL);
	memset(ptrRecvOverlapped, 0, sizeof *ptrRecvOverlapped);
	memset(ptrSendOverlapped, 0, sizeof *ptrSendOverlapped);
	ptrRecvOverlapped->hEvent = ptrWSARecvCompletedEvents[1]->m_hObject = WSACreateEvent(); // manual and nonsignaled
	ptrSendOverlapped->hEvent = ptrWSASendCompletedEvents[1]->m_hObject = WSACreateEvent(); // manual and nonsignaled
	ptrWSARecvLock = new CMultiLock(ptrWSARecvCompletedEvents,2);
	ptrWSASendLock = new CMultiLock(ptrWSASendCompletedEvents,2);
	nReceivedBytes = 0;
	receiveFlags = 0;
	nSentBytes = 0;												  
	sendFlags = 0;
	ptrRecvDataBuffer = new WSABUF(); // WSABUF structure for WSARecv
	ptrRecvDataBuffer->buf = recvMessage;
	ptrRecvDataBuffer->len = 2048;
	ptrSendDataBuffer = new WSABUF(); // WSABUF structure for WSASend
	ptrSendDataBuffer->buf = sendMessage;
}

ClientSocket::~ClientSocket(void) {
	delete ptrWSARecvLock;
	delete ptrWSASendLock;
	delete ptrWSARecvCompletedEvents[1];
	delete ptrWSASendCompletedEvents[1];
	delete ptrRecvOverlapped;
	delete ptrSendOverlapped;
}

int ClientSocket::openConnection(void) {

	// Variables
	SOCKADDR_IN serverAddress;	// Structure of the server address

	// Creation of the client socket
	if ((clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET){
		_tprintf(_T("socket() failed, error %d\n"), WSAGetLastError());
		return 1;
	}
	// Description of server socket
	serverAddress.sin_family       = AF_INET;
	serverAddress.sin_addr.s_addr  = inet_addr("127.0.0.1");
	serverAddress.sin_port         = htons(1234);
	memset(&serverAddress.sin_zero, '\0', sizeof(serverAddress.sin_zero));

	// Connection of client to server
	if(connect(clientSocket, (SOCKADDR *)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR){
		_tprintf(_T("Unable to connect client to server, error %d\n"), WSAGetLastError());
		return 1;
	}

	return 0;
}

int ClientSocket::recv(void){
	DWORD lockResult, error;

	if (WSARecv(clientSocket, ptrRecvDataBuffer, 1, &nReceivedBytes, &receiveFlags, ptrRecvOverlapped, NULL) == SOCKET_ERROR)
	{
		if ((error = WSAGetLastError()) != WSA_IO_PENDING) {
			_tcout << "WSARecv() failed, error %d\n" << error << endl;
			return 1;
		}
		// We wait 5 seconde that receiving message complete.
		// If WSARecv don't complete into 5 secondes, it means that there is not data to receive.

		if ((lockResult = ptrWSARecvLock->Lock(5000, false)) == -1)
		{
			_tcout << "ptrWSARecvLock->Lock() failed for ptrWSArecvCompletedEvents" << endl;
			return 1;
		}
		if (lockResult == WAIT_TIMEOUT) {// No more data.
			_tcout << "No more data. All packets have been received." << endl;
			*ptrDownloadingCompleted = true;
			return 0; // Waiting stop because the time was out.
		}
		if (lockResult == WAIT_OBJECT_0 ) // stopEvent has become signaled
			return 0;
		if (!WSAGetOverlappedResult(clientSocket, ptrRecvOverlapped, &nReceivedBytes, FALSE, &receiveFlags)) {
			_tcout << "WSAGetOverlappedResult(&recvOverlapped) failed, error " << GetLastError() << endl;
			return 1;
		}
	}
	else
	{
		if (!nReceivedBytes){
			_tcout << "Server has closed the connection" << endl;
			return 1;
		}
	}
	if(ptrWSARecvCompletedEvent->ResetEvent() == 0) {
		_tcout << "ResetEvent() failed for ptrWSARecvCompletedEvents[1]" << endl;
		return 1;
	}
	return 0;
}

char* ClientSocket::getRecvMessage(void) {
	return recvMessage;
}

void ClientSocket::setSendMessage(_TCHAR* message, int lenght) {
	memset(&sendMessage[0],lenght,1);
	memset(&sendMessage[1],0,3);
	memcpy(&sendMessage[4],message,lenght);
	ptrSendDataBuffer->len = lenght+4;
}

int ClientSocket::send(void) {
	DWORD lockResult, error;

	if (WSASend(clientSocket, ptrSendDataBuffer, 1, &nSentBytes, sendFlags, ptrSendOverlapped, NULL)== SOCKET_ERROR)
	{
		if ((error = WSAGetLastError()) != WSA_IO_PENDING) {
			_tcout << "WSASend() failed, error " << error << endl;
			return 1;
		}
		lockResult = ptrWSASendLock->Lock(INFINITE, false);
		if (lockResult == -1) {
			_tcout << "ptrWSASendLock->Lock() failed for ptrWSASendCompletedEvents." << endl;
			return 1;
		}
		if (lockResult == WAIT_OBJECT_0) // stopEvent raised.
			return 0;
		if (!WSAGetOverlappedResult(clientSocket, ptrSendOverlapped, &nSentBytes, false, &sendFlags)) {
			_tcout << "WSAGetOverlappedResult(&sendOverlapped) failed, error " << GetLastError() << endl;
			return 1;
		}
	}
	else
	{
		if (!nSentBytes) {
			_tcout << "Server has closed the connection\n" << endl;
			return 1;
		}
	}
	if(ptrWSASendCompletedEvent->ResetEvent() == 0) {
		_tcout << "ResetEvent() failed for ptrWSASendCompletedEvents[1]." << endl;
		return 1;
	}
	_tcout << nReceivedBytes << " bytes received" << endl;
	return 0;
}

void ClientSocket::closeConnection(void) {

	DWORD error;

	if (clientSocket != INVALID_SOCKET)	{
		if (shutdown(clientSocket, SD_RECEIVE) == SOCKET_ERROR)		{
			if ((error = WSAGetLastError()) != WSAENOTCONN) // WSAENOTCONN means that the connection was not established
				_tcout << "shutdown() failed, error " << WSAGetLastError() << endl;
		}
		closesocket(clientSocket);
	}
}