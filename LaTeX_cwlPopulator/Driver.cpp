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

#include "OpenDialog.hpp"

using namespace std;
//using namespace boost;

void replaceAllCharXWithCharY(wchar_t* path, const char X, const char Y);
const string scanFileForKeywords(const wchar_t* const pathToFile, unordered_map<string, int> * const cmdArgMap);
void writeToFile(string content, const wchar_t* const pathToFile, unordered_map<string, int> * const cmdArgMap);

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
	// If everything went well, then we will write the content to the .cwl file.
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
			//size_t fromIndex = 2; // ignore "e{" or "d{" in front.
			const size_t fromIndex = 1; // ignore "{" or "d" in front.
			const size_t toIndex = fromIndex + 1; // ignore '{' or 'd' in front and '}' and at the end.
			if (0 == line.find("\\RequirePackage")) {
				pattern = "[\\]|e](\\{.+?\\})";
				prefix = "#include:";
			}
			else if (line.find("\\newcommandx") == 0) {
				// two patterns seperated by '|'. EITHER we match the first pattern (w. args)
				// in two capturing groups or the second (w/o args) in none.
				pattern = "x(\\{.+?\\})(\\[\\d*\\])|d(\\{.+?\\})";
			}
			else if (line.find("\\newcommand") == 0) {
				pattern = "d(\\{.+?\\})(\\[\\d*\\])|d(\\{.+?\\})";
			}
			else {
				continue;
			}
			// Regex-match the line for any keyword, Take the first match and add it.
			if (regex_search(line, arrayOfMatches, pattern)) {
				string matchedStr = "";
				if (arrayOfMatches.size() == 1) { // TODO Useless if? Maybe combine all ifs (with smarter pattern)?
					matchedStr = arrayOfMatches[0];
				}
				else if (arrayOfMatches.size() == 2) { // package
					matchedStr = arrayOfMatches[1];
				}
				else if (arrayOfMatches.size() == 4 && arrayOfMatches[2] == "") { // Only 1 match
					matchedStr = arrayOfMatches[3];
				}
				else {
					matchedStr = arrayOfMatches[1];
					const string matchedArg = arrayOfMatches[2];
					const int noOfArgs = stoi(matchedArg.substr(1, matchedArg.length() - 1)); // remove "[]" from arg
					(*cmdArgMap)[matchedStr.substr(fromIndex, matchedStr.length() - toIndex)] = noOfArgs;
				}
				matchedStr = matchedStr.substr(fromIndex, matchedStr.length() - toIndex);
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
		while (getline(inoutfile, inoutFileLine)) {
			if (inoutFileLine == "") { // Empty '\n'. Skip
				continue;
			}
			// If inoutFileLine is a command w. args, we have to remove them.
			// The commands from the .sty file do not have any args after all.
			size_t pos = inoutFileLine.find('{');
			// string::find() returns string::npos when there is no match
			// string::npos is defined as -1.
			if (pos != string::npos) {
				inoutFileLine = inoutFileLine.substr(0, pos); // Remove args if any.
			}
			pos = content.find(inoutFileLine); // Now to search for real.
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
			inoutFileLine = "";
			// Find commands with arguments and let the user fill them out.
			while (getline(contentStream, inoutFileLine)) {
				if (0 == inoutFileLine.find('\\')) {
					unordered_map<string, int>::const_iterator valueToKey = (*cmdArgMap).find(inoutFileLine);
					if (valueToKey != (*cmdArgMap).end()) {
						const size_t noOfArgs = valueToKey->second;
						string args = "";
						for (size_t i = 0; i < noOfArgs; i++) {
							args += "{expr}";
						}
						cout << "New command \033[1;33m" << inoutFileLine + args <<
							"\033[0m found in .sty which does not exist in .cwl! \n" <<
							"As you can see, each arg has a temporary name. \n" <<
							"You can name each arg by typing a name for each of them " <<
							"one at a time. \n" <<
							"If you want the temp. name for some arg, then type " <<
							"'-' for that arg.\n";
						args = "";
						for (size_t i = 1; i <= noOfArgs; i++) {
							cout << "Please input arg " << i << "'s name: ";
							string input = "";
							getline(cin, input);
							if (input == "-") { // Fill the rest with default.
								for (size_t j = 0; j <= noOfArgs - i; j++) {
									args += "{expr}";
								}
								break;
							}
							args += "{"  + input + "}";
						}
						// Replace the command line with the args.
						const size_t pos = content.find(inoutFileLine); // Start pos. of the current CMD (incl. line-feed)
						const size_t wordLength = inoutFileLine.length();
						content.insert(pos + wordLength, args); // insert args after the CMD.
					}
				}
			} // All the file's content has been read; the stream's state is "eof"; 
			inoutfile.clear(); // Reset stream to beginning (state "good") so we can do operations.
			inoutfile << content; // Write the changes to the .cwl file. Per C++ standard, stream is closed automatically.
		}
	}
	else {
		cout << "The file cannot be loaded!\n";
	}
}