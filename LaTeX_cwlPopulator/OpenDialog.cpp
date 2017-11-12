#include "OpenDialog.h"
using namespace std;

OpenDialog::OpenDialog(COMDLG_FILTERSPEC filter[]) {
	for (int i = 0; i < MAX_FILE_TYPES; i++) {
		fileTypeFilter[i] = filter[i];
	}
}

wchar_t* OpenDialog::openFileDialog(LPCWSTR dialogTitle) {
	wchar_t* pszFilePath = 0;

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog *pFileOpen;
		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
		if (SUCCEEDED(hr)) {
			// Set the file types and title.
			pFileOpen->SetFileTypes(_countof(fileTypeFilter), fileTypeFilter);
			pFileOpen->SetTitle(dialogTitle);
			// Show the Open dialog box.
			hr = pFileOpen->Show(NULL);
			if (SUCCEEDED(hr)) {
				IShellItem *pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr)) {
					// Get the file name from the dialog box.
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					// MessageBox(NULL, pszFilePath, L"File Path", MB_OK);
					// CoTaskMemFree(pszFilePath); // Frees a memory block previously allocated through CoTaskMemAlloc.
					pItem->Release();
				}
			}
			else if (FAILED(hr)) {
				// Handle the case where the user cancels.
				pszFilePath = L"";
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
	return pszFilePath;
}