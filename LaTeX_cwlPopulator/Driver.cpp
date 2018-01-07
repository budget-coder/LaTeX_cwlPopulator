/*
#if __cplusplus <= 199711L
	#error This library needs at least a C++11 compliant compiler
#endif
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
//#include <boost/regex.hpp>
#include <unordered_map>

#include "OpenDialog.h"

using namespace std;
//using namespace boost;

void replaceAllCharXWithCharY(wchar_t* path, const char X, const char Y);
const string scanFileForKeywords(const wchar_t* const pathToFile, unordered_map<string, int> * const cmdArgMap);
void writeToFile(string content, const wchar_t* const pathToFile, unordered_map<string, int> * const cmdArgMap);
const string enterOptionalArgument();

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

	unordered_map<string, int> cmdArgMap;
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
	const string listOfKeywords = scanFileForKeywords(pathToSty, &cmdArgMap);
	// If everything went well, then we will write the content to the .cwl file
	if (listOfKeywords != "") {
		writeToFile(listOfKeywords, pathToCwl, &cmdArgMap);
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

const string scanFileForKeywords(const wchar_t* const pathToFile, unordered_map<string, int> * const cmdArgMap) {
	string listOfKeywords = "";
	ifstream infile(pathToFile); // Create input file stream.
	if (infile) {
		cout << "Success loading file. Commencing keyword scan...\n";
		string line = "";

		while (getline(infile, line)) {
			smatch arrayOfMatches; // instantiation of std::match_results.
			regex pattern;
			string prefix = "";
			size_t fromIndex = 2; // ignore "e{" or "d" in front.
			size_t toIndex = fromIndex + 1; // ignore "e{" or "d{" in front and '}' and at the end.
			if (0 == line.find("\\RequirePackage")) {
				pattern = "[\\]|e]\\{.+?\\}";
				prefix = "#include:";
			}
			else if (0 == line.find("\\newcommand")) {
				// two patterns seperated by '|'. EITHER we match the first pattern (w. args)
				// in two capturing groups or the second (w/o args) in none.
				pattern = "(d\\{.+?\\})(\\[\\d*\\])|d\\{.+?\\}";
			}
			else {
				continue;
			}
			// Regex-match the line for any keyword, Take the first match and add it.
			if (regex_search(line, arrayOfMatches, pattern)) {
				string matchedStr;
				
				if (arrayOfMatches.size() == 1 || arrayOfMatches[2] == "") { // Only 1 match
					matchedStr = arrayOfMatches[0];
				}
				else {
					matchedStr = arrayOfMatches[1];
					//cout << "Matched CMD: " << matchedStr << endl;
					const string matchedArg = arrayOfMatches[2];
					//cout << "No. of matches: " << arrayOfMatches.size() << ". Multiple matches. Group 1: " << matchedStr << ". Group 2: " << matchedArg << endl;
					//cout << "No. of args: " << matchedArg.length() << endl;
					//cout << "Matched command: " << arrayOfMatches.str(1) << "." << endl;
					const int noOfArgs = stoi(matchedArg.substr(1, matchedArg.length() - 1)); // remove "[]" from arg
					//	cout << noOfArgs << " arguments were found for " << matchedStr.substr(fromIndex, matchedStr.length() - oIndex) << endl;
					(*cmdArgMap)[matchedStr.substr(fromIndex, matchedStr.length() - toIndex)] = noOfArgs;
					//	for (size_t i = 0; i < noOfArgs; i++) {
					//		enterOptionalArgument();
					//	}
				}
				matchedStr = matchedStr.substr(fromIndex, matchedStr.length() - toIndex);
				//cout << "Saving " << prefix + matchedStr << " to list." << endl;
				listOfKeywords += prefix + matchedStr + "\n";
			}
		}
		infile.close();
	}
	else {
		cout << "The file cannot be loaded!\n";
	}
	return listOfKeywords;
}

void writeToFile(string content, const wchar_t* const pathToFile, unordered_map<string, int> * const cmdArgMap) {
	fstream inoutfile(pathToFile); // Create in/out file stream
	if (inoutfile) {
		string inoutFileLine = "";
		cout << "Success loading file. Commencing writing operation...\n";
		//streamsize noOfCharsProcessed = 0;
		while (getline(inoutfile, inoutFileLine)) {
			//noOfCharsProcessed += inoutFileLine.length() + 2; // 2 for LF CR
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
		if (!content.empty()) {
			istringstream contentStream(content);
			// Find commands with arguments and let the user fill them out.
			inoutFileLine = "";
			//streamsize noOfCharsProcessed = 0; // streamsize is an integral type (i.e. "some integer type").
			while (getline(contentStream, inoutFileLine)) {
				//noOfCharsProcessed += contentLine.length() + 2; // 2 for \n
				if (0 == inoutFileLine.find('\\')) {
					//cout << "Command found. Can I find the command " << inoutFileLine << endl;
					unordered_map<string, int>::const_iterator valueToKey = (*cmdArgMap).find(inoutFileLine);
					if (valueToKey != (*cmdArgMap).end()) {
						// Print out the command with args to the user.
						const size_t noOfArgs = valueToKey->second;
						string args = "";
						for (size_t i = 0; i < noOfArgs; i++) {
							args += "{expr}";
						}
						cout << "New command " << inoutFileLine + args <<
							" found in .sty which does not exist in .cwl! \n" <<
							"As you can see, each arg has a temporary name. \n" <<
							"You can name each arg by typing a name for each of them " <<
							"separated with a space. \n" <<
							"If you want the temp. name for some arg, then type " <<
							"'-' for that arg.\n";
						const size_t pos = content.find(inoutFileLine); // Start pos. of the current CMD (incl. line-feed)
						args = "";
						for (size_t i = 0; i < noOfArgs; i++) {
							args += "{"  + enterOptionalArgument() + "}";
						}
						// Replace the command line with the args.
						
						//cout << "TEST. BEFORE SAVING, THIS IS THE CURRENT LOCATION OF COMMAND: " << noOfCharsProcessed << ". TRUE?" << endl;
						cout << "TEST. BEFORE SAVING, THIS IS THE CURRENT LOCATION OF COMMAND: " << pos << ". TRUE?" << endl;
						const size_t wordLength = inoutFileLine.length();
						content.insert(pos + wordLength, args); // insert args.
					}
				}
			} // All the file's content has been read; the stream's state is "eof"; 
			inoutfile.clear(); // Reset stream to beginning (state "good") so we can do operations.
			inoutfile << content; // Write the changes to the .cwl file. Per C++ standard, stream is closed automatically.
		}
		//inoutfile.close();
	}
	else {
		cout << "The file cannot be loaded!\n";
	}
}

const string enterOptionalArgument() {
	string input = "";
	// Read input from user
	getline(cin, input);
	//cout << "TEST: You inputted " << input << endl;
	return input;
}