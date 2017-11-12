#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>

#include "OpenDialog.h"

using namespace std;

void replaceAllCharXWithCharY(wchar_t* path, const char X, const char Y);
const string scanFileForKeywords(const wchar_t* const pathToFile);
void writeToFile(string content, const wchar_t* const pathToFile);

int main() {
	// To display the path correctly on cmd.
	// _setmode(_fileno(stdout), _O_U8TEXT);

	// Create an array of COMDLG_FILTERSPEC structs with file types (type: wchar_t).
	COMDLG_FILTERSPEC styFilter[MAX_FILE_TYPES] = {
		{ L"Style files", L"*.sty" },
		{ L"All files", L"*.*" }
	};
	COMDLG_FILTERSPEC cwlFilter[MAX_FILE_TYPES] = {
		{ L"Completition word list files", L"*.cwl" },
		{ L"All files", L"*.*" }
	};
	OpenDialog styDialog(styFilter);
	OpenDialog cwlDialog(cwlFilter);

	// Specify the .sty and .cwl files.
	wchar_t *pathToSty  = styDialog.openFileDialog(L"Locale the .sty file to read from");
	wchar_t *pathToCwl = cwlDialog.openFileDialog(L"Locale the .cwl file to populate");
	if (pathToSty == L"" || pathToCwl == L"") {
		cout << "No .sty or .cwl file were specified. Exitting...\n";
		return 0;
	}
	// Replace backslashes with forward slashes to fix paths.
	replaceAllCharXWithCharY(pathToSty, '\\', '/');
	replaceAllCharXWithCharY(pathToCwl, '\\', '/');
	// Now to read the packages and commands from the .sty file.
	const string listOfKeywords = scanFileForKeywords(pathToSty);
	// If everything went well, then we will write the content to the .cwl file
	if (listOfKeywords != "") {
		writeToFile(listOfKeywords, pathToCwl);
	}
	return 0;
}

void replaceAllCharXWithCharY(wchar_t* path, const char X, const char Y) {
	wchar_t cPath = L' ';
	size_t size = 0;
	// Termination character of wchar are L'\0' (i.e. a 0 per convention)
	while (cPath != L'\0') {
		cPath = *path;
		if (cPath == X) {
			*path = Y;
		}
		size++;
		// wcout << "Current char: " << cPath << "\n" << L"\n"; // ALWAYS end wide character streams with L"\n".
		path++; // increment pointer to point at next character.
	}
	path -= size; // Reset pointer position to point at the drive letter again.
}

const string scanFileForKeywords(const wchar_t* const pathToFile) {
	string listOfKeywords = "";
	ifstream infile(pathToFile); // Create input file stream.
	if (infile) {
		cout << "Success loading file. Commencing keyword scan...\n";
		string line = "";

		while (getline(infile, line)) {
			smatch arrayOfMatches; // instantiation of std::match_results.
			regex pattern;
			string prefix = "";
			size_t fromIndex = 1;
			size_t toIndex = 2;
			if (0 == line.find("\\RequirePackage")) {
				pattern = "[\\]|e]\\{.+?\\}";
				prefix = "#include:";
				fromIndex = 2;
				toIndex = 3;
			}
			/* TODO Does not consider parameters [0-9]* and manual label for each.
			else if (0 == line.find("\\newcommand")) {
				pattern = "\\{.+?\\}";
			}
			*/
			// Regex-match the line for any keyword, Take the first match and add it.
			if (regex_search(line, arrayOfMatches, pattern)) {
				string matchedStr = arrayOfMatches.str(0);
				listOfKeywords += prefix + matchedStr.substr(fromIndex, matchedStr.length() - toIndex) + "\n";
			}
		}
		infile.close();
	}
	else {
		cout << "The file cannot be loaded!\n";
	}
	return listOfKeywords;
}

void writeToFile(string content, const wchar_t* const pathToFile) {
	/*
	ofstream outfile(pathToFile, ofstream::app); // Create output file stream
	if (outfile) {
		cout << "Success loading file. Commencing writing operation...\n";
		outfile << content;
		outfile.close();
	}
	else {
		cout << "The file cannot be loaded!\n";
	}
	*/
	fstream inoutfile(pathToFile); // Create output file stream
	if (inoutfile) {
		string inoutFileLine = "";
		cout << "Success loading file. Commencing writing operation...\n";

		while (getline(inoutfile, inoutFileLine)) {
			// string::find() returns string::npos when there is no match
			// string::npos is defined as -1.
			size_t pos = content.find(inoutFileLine);
			if (pos != string::npos) {
				// Match found! Delete line in content by getting
				// the pos of the next newline and the word length
				size_t wordLength = inoutFileLine.length();
				size_t secondNL = content.find('\n', pos);
				content.erase(secondNL - wordLength, wordLength+1); // +1 for deleting '\n'
			}
		}
		// "content" postcondition:
		// Any packages/commands left are to be added to the .cwl file.
		if (!content.compare("")) {
			istringstream contentStream(content);
			string contentLine = "";
			while (getline(contentStream, contentLine)) {
				if (0 == contentLine.find('\\')) {
					// Print out the command with args to the user.
					cout << "New command " << contentLine <<
						" found in .sty which does not exist in .cwl! \n" <<
						"As you can see, each arg has a temporary name. " <<
						"You can name each arg by typing a name for each of them " <<
						"separated with a space. \n" <<
						"If you want the temp. name for some arg, then type " <<
						"'-' for that arg.\n";
					string input = "";
					// Read input from user
					getline(cin, input);
					//TODO  Do something with the input. Replace arg names in content with user input
					cout << "TEST: You inputted " << input;
				}
			}
			// Write the changes to the .cwl file and close the stream.
			inoutfile << content;
		}
		inoutfile.close();
	}
	else {
		cout << "The file cannot be loaded!\n";
	}
}