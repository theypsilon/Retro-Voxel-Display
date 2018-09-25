#include <iostream>
#include <fstream>
#include <cassert>
#include <windows.h>
#include <shellapi.h>
#include <theypsilon/error.h>
#include <thread>

LPSTR* CommandLineToArgvA(LPSTR lpCmdLine, INT *pNumArgs)
{
	int retval;
	retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, NULL, 0);
	if (!SUCCEEDED(retval))
		return NULL;

	LPWSTR lpWideCharStr = (LPWSTR)malloc(retval * sizeof(WCHAR));
	if (lpWideCharStr == NULL)
		return NULL;

	retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, lpWideCharStr, retval);
	if (!SUCCEEDED(retval))
	{
		free(lpWideCharStr);
		return NULL;
	}

	int numArgs;
	LPWSTR* args;
	args = CommandLineToArgvW(lpWideCharStr, &numArgs);
	free(lpWideCharStr);
	if (args == NULL)
		return NULL;

	int storage = numArgs * sizeof(LPSTR);
	for (int i = 0; i < numArgs; ++i)
	{
		BOOL lpUsedDefaultChar = FALSE;
		retval = WideCharToMultiByte(CP_ACP, 0, args[i], -1, NULL, 0, NULL, &lpUsedDefaultChar);
		if (!SUCCEEDED(retval))
		{
			LocalFree(args);
			return NULL;
		}

		storage += retval;
	}

	LPSTR* result = (LPSTR*)LocalAlloc(LMEM_FIXED, storage);
	if (result == NULL)
	{
		LocalFree(args);
		return NULL;
	}

	int bufLen = storage - numArgs * sizeof(LPSTR);
	LPSTR buffer = ((LPSTR)result) + numArgs * sizeof(LPSTR);
	for (int i = 0; i < numArgs; ++i)
	{
		assert(bufLen > 0);
		BOOL lpUsedDefaultChar = FALSE;
		retval = WideCharToMultiByte(CP_ACP, 0, args[i], -1, buffer, bufLen, NULL, &lpUsedDefaultChar);
		if (!SUCCEEDED(retval))
		{
			LocalFree(result);
			LocalFree(args);
			return NULL;
		}

		result[i] = buffer;
		buffer += retval;
		bufLen -= retval;
	}

	LocalFree(args);

	*pNumArgs = numArgs;
	return result;
}

ty::error program(int argc, char* argv[]);

int CALLBACK WinMain(
	HINSTANCE   hInstance,
	HINSTANCE   hPrevInstance,
	LPSTR       lpCmdLine,
	int         nCmdShow
) {
	int argc;
	LPSTR * argv = CommandLineToArgvA(GetCommandLineA(), &argc);
	std::ofstream out;
	try {
		std::string logfile = std::string(PROJECT_BINARY_NAME) + "-" + std::string(PROJECT_VERSION) + ".log";
		out.open(logfile.c_str(), std::ios::out | std::ios::trunc);
		std::cout.rdbuf(out.rdbuf());
		std::cerr.rdbuf(out.rdbuf());
	}
	catch (...) {}
	std::string selected_picture = "";
	if (argc < 2) {
		int msgboxID = MessageBox(
			NULL,
			"Do you want to load the default animation?\n\nClick 'No' for selecting a custom picture from your system.",
			PROJECT_OFFICIAL_NAME,
			MB_ICONQUESTION | MB_YESNO
		);

		char szFile[256];
		if (msgboxID == IDNO)
		{
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFile = szFile;
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Pictures\0*.png;*.jpg;*.jpeg;*.tga;*.bmp;*.psd;*.gif;*.hdr;*.pic\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&ofn)) {
				selected_picture = szFile;
			}
		}
	}
	auto err = ty::error::none();
	if (selected_picture.empty()) {
		err = program(argc, argv);
	} else {
		char* fake_argv[2];
		fake_argv[0] = argv[0];
		fake_argv[1] = const_cast<char*>(selected_picture.c_str());
		err = program(2, fake_argv);
	}
	LocalFree(argv);
	if (err) {
		std::cerr << "[ERROR] " << err.message();
		MessageBoxA(
			NULL, //err.msg.c_str(),
			"Some unexpected error happened.\nMake sure you have installed the graphic drivers correctly.\nIf the problem persists, contact the author at:\ntheypsilon@gmail.com",
			"Message", 
			MB_OK | MB_ICONERROR
		);
		return -1;
	}
	return 0;
}