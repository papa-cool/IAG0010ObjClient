#include "stdafx.h"
#include "SendingThread.h"


SendingThread::SendingThread(ClientSocket* ptrClientSocket, CEvent* ptrDataRecvEvent, CEvent* ptrDataSentEvent, CEvent* ptrStopEvent, BOOL* ptrDownloadingCompleted):
	ptrClientSocket(ptrClientSocket), ptrDataRecvEvent(ptrDataRecvEvent), ptrDataSentEvent(ptrDataSentEvent), ptrStopEvent(ptrStopEvent), ptrDownloadingCompleted(ptrDownloadingCompleted)
{
	ptrDataRecvEvents[0] = ptrStopEvent;
	ptrDataRecvEvents[1] = ptrDataRecvEvent;
	ptrDataRecvLock = new CMultiLock(ptrDataRecvEvents,2);
	firstSending = true;
}

SendingThread::~SendingThread(void)
{
	delete ptrDataRecvLock;
}

int SendingThread::Run(void)
{
	DWORD lockResult;

	ptrClientSocket->setSendMessage(_T("Hello IAG0010Server"),20*sizeof(_TCHAR));
	while(!*ptrDownloadingCompleted){

		if ((lockResult = ptrDataRecvLock->Lock(INFINITE, false)) == -1) {
			_tcout << "WSAWaitForMultipleEvents() failed for packetReceivedEvents in sendingDataThread" << endl;
			return 1;
		}
		if (lockResult == WAIT_OBJECT_0)
			return 0; // stopEvent raised

		if (ptrClientSocket->send() == 1){
			_tcout << "ptrClientSocket->send() failed in SendingThread" << endl;
			return 1;
		}

		// Executed only the first time.
		if (firstSending) {
			ptrClientSocket->setSendMessage(_T("Ready"), 6*sizeof(_TCHAR));
			_tcout << "The downloading is gonna start." << endl;
			_tcout << "You can follow the evolution of the downloading." << endl;
			_tcout << "The downloaded file will be automatically launch in the default application." << endl;
			firstSending = false;
		}

		// Events management and synchronization
		ptrDataRecvEvent->ResetEvent();
		ptrDataSentEvent->SetEvent();
}

return 0;}
