// rococo.dll.test.cpp : This file contains the 'main' function. Program execution begins and ends there.


#include <Windows.h>

int main()
{
	auto hNPP = LoadLibraryA("C:\\Program Files\\Notepad++\\plugins\\sexystudio.4.notepad++.debug\\sexystudio.4.notepad++.debug.dll");
	if (hNPP == NULL)
	{
		return GetLastError();
	}
	else
	{
		return 0;
	}
}


