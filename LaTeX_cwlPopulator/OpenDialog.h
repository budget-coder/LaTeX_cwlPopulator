#pragma once
#include <ShObjIdl.h>
const int MAX_FILE_TYPES = 2;

class OpenDialog {
	public:
		OpenDialog(COMDLG_FILTERSPEC filter[]);
		wchar_t *openFileDialog(LPCWSTR dialogTitle);

	private:
		COMDLG_FILTERSPEC fileTypeFilter[MAX_FILE_TYPES];
};