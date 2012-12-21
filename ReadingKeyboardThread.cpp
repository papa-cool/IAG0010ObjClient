#include "stdafx.h"
#include "ReadingKeyboardThread.h"

ReadingKeyboardThread::ReadingKeyboardThread(CEvent* ptrStopEvent): ptrStopEvent(ptrStopEvent)
{
}


ReadingKeyboardThread::~ReadingKeyboardThread(void)
{
}

int ReadingKeyboardThread::Run(void)
{
	_tstring command;
	_tcout << "To stop, type the \"exit\" command." << endl;
	_tcin >> command;
	while (command != _T("exit")) 
	{
		_tcout << "The entered command " << command << " hasn't been recongnized. Type the \"exit\" command to quit." << endl;
		_tcin >> command;
	}
	ptrStopEvent->SetEvent();
	return 1;
}