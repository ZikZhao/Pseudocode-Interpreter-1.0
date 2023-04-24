#include <Windowsx.h>
#include <Windows.h>
#include <cmath>
#include <ctime>
#include <list>
#include "Definitions.h"
#include "Debug.h"
#include "SyntaxCheck.h"
#include "Execution.h"

using namespace std;
HANDLE standard_input;
HANDLE standard_output;
HANDLE standard_error;
HANDLE signal_in = 0; // used for debugging mode
HANDLE signal_out = 0; // used for debugging mode
size_t& CII = current_instruction_index;

// these are used for debugging mode
bool debug = false;
list<BREAKPOINT> breakpoints;
HANDLE breakpoint_handled = CreateEvent(0, true, false, 0);
DWORD step_type;
HANDLE step_handled = CreateEvent(0, true, false, 0);

// format codes in physical files
wchar_t** FormatCode(wchar_t* code, _Out_ size_t* count_out) {
	// first stage formatting
	size_t count = 0;
	size_t total_length = 0;
	size_t* lengths = new size_t[1000] {};
	size_t* indentations = new size_t[1000] {};
	size_t* comment = new size_t[1000] {};
	size_t last_return = 0;
	bool before_text = true;
	long long after_text = -1;
	size_t offset = 0; // skip first few empty lines or comments
	for (size_t index = 0;; index++) {
		if (code[index] == 13 and code[index + 1] == 10) {
			lengths[count] = index - last_return;
			total_length += lengths[count] + 2;
			if (after_text == -1) {
				comment[count] = 0;
			}
			else {
				comment[count] = index - after_text;
			}
			last_return = index + 2;
			index++;
			count++;
			before_text = true;
			after_text = -1;
			if (count % 1000 == 0) {
				size_t* tag = new size_t[count + 1000]{};
				memcpy(tag, lengths, count * sizeof(size_t));
				delete[] lengths;
				lengths = tag;
				size_t* tag2 = new size_t[count + 1000]{};
				memcpy(tag2, indentations, count * sizeof(size_t));
				delete[] indentations;
				indentations = tag2;
				size_t* tag3 = new size_t[count + 1000]{};
				memcpy(tag3, comment, count * sizeof(size_t));
				delete[] comment;
				comment = tag3;
			}
		}
		else if ((code[index] == 9 or code[index] == 32) and before_text) {
			indentations[count]++;
		}
		else if (code[index] == 47 and code[index + 1] == 47 and after_text == -1){
			after_text = index;
			if (!before_text) {
				for (unsigned long long index2 = index - 1; index2 != -1; index2--) {
					if (code[index2] == 32 or code[index2] == 9) {
						after_text--;
					}
					else {
						break;
					}
				}
			}
			before_text = false;
		}
		else if (code[index] == 0) { // EOF
			lengths[count] = index - last_return;
			total_length += lengths[count];
			if (after_text == -1) {
				comment[count] = 0;
			}
			else {
				comment[count] = index - after_text;
			}
			count++;
			break;
		}
		else {
			before_text = false;
		}
	}
	// split into lines
	wchar_t** lines = new wchar_t* [count];
	for (size_t index = 0; index != count; index++) {
		wchar_t* line = new wchar_t[lengths[index] - indentations[index] - comment[index] + 1];
		memcpy(line, code + offset + indentations[index],
			(size_t)(lengths[index] - indentations[index] - comment[index]) * 2);
		line[lengths[index] - indentations[index] - comment[index]] = 0;
		strip(line);
		lines[index] = line;
		offset += lengths[index] + 2;
	}
	*count_out = count;
	return lines;
}

// format error message when an error is detected
void FormatErrorMessage(Error error, wchar_t** lines) {
	const wchar_t* error_type[] = {
		L"",
		L"语法错误(Syntax Error)",
		L"变量错误(Variable Error)",
		L"求值错误(Evaluation Error)",
		L"数据类型错误(Type Error)",
		L"参数错误(Argument Error)",
		L"值错误(Value Error)"
	};
	const wchar_t* tag_header = L"异常退出：";
	wchar_t* header = new wchar_t[wcslen(error_type[error.error_type]) + 6];
	memcpy(header, tag_header, wcslen(tag_header) * 2);
	memcpy(header + 5, error_type[error.error_type], wcslen(error_type[error.error_type]) * 2);
	header[wcslen(error_type[error.error_type]) + 5] = 0;
	int size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, header, (int)wcslen(header), nullptr, 0, nullptr, 0);
	char* buffer = new char[size + 1];
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, header, (int)wcslen(header), buffer, size, nullptr, 0);
	buffer[size] = '\n';
	WriteFile(standard_error, buffer, size + 1, nullptr, 0);
	static const wchar_t* tag_indicator[] = { L"位于第 ", L" 行，在 "};
	static const char* first_indentation = "  ";
	static const char* second_indentation = "    ";
	// base call
	size_t current_line_number = calling_ptr ? call[0].line_number : CII;
	wchar_t* line_number_string = unsigned_to_string(current_line_number + 1);
	const wchar_t* main_string = L"<主程序>";
	wchar_t* indicator = new wchar_t[wcslen(main_string) + wcslen(line_number_string) + 12];
	memcpy(indicator, tag_indicator[0], 8);
	memcpy(indicator + 4, line_number_string, wcslen(line_number_string) * 2);
	memcpy(indicator + 4 + wcslen(line_number_string), tag_indicator[1], 10);
	memcpy(indicator + 9 + wcslen(line_number_string), main_string, wcslen(main_string) * 2);
	indicator[wcslen(main_string) + wcslen(line_number_string) + 9] = L' ';
	indicator[wcslen(main_string) + wcslen(line_number_string) + 10] = L'中';
	indicator[wcslen(main_string) + wcslen(line_number_string) + 11] = 0;
	size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, indicator, (int)wcslen(indicator), nullptr, 0, nullptr, 0);
	buffer = new char[size + 3];
	memcpy(buffer, first_indentation, 2);
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, indicator, (int)wcslen(indicator), buffer + 2, size, nullptr, 0);
	buffer[size + 2] = '\n';
	WriteFile(standard_error, buffer, size + 3, nullptr, 0);
	delete[] buffer;
	size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lines[current_line_number], (int)wcslen(lines[current_line_number]), nullptr, 0, nullptr, 0);
	buffer = new char[size + 5];
	memcpy(buffer, second_indentation, 4);
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lines[current_line_number], (int)wcslen(lines[current_line_number]), buffer + 4, size, nullptr, 0);
	buffer[size + 4] = '\n';
	WriteFile(standard_error, buffer, size + 5, nullptr, 0);
	delete[] buffer;
	// call in stack
	for (unsigned short index = 0; index != calling_ptr; index++) {
		current_line_number = index == calling_ptr - 1 ? CII : call[index + 1].line_number;
		wchar_t* call_info = call[index].name;
		line_number_string = unsigned_to_string(current_line_number + 1);
		indicator = new wchar_t[wcslen(call_info) + wcslen(line_number_string) + 12];
		memcpy(indicator, tag_indicator[0], 8);
		memcpy(indicator + 4, line_number_string, wcslen(line_number_string) * 2);
		memcpy(indicator + 4 + wcslen(line_number_string), tag_indicator[1], 10);
		memcpy(indicator + 9 + wcslen(line_number_string), call_info, wcslen(call_info) * 2);
		indicator[wcslen(call_info) + wcslen(line_number_string) + 9] = 32;
		indicator[wcslen(call_info) + wcslen(line_number_string) + 10] = L'中';
		indicator[wcslen(call_info) + wcslen(line_number_string) + 11] = 0;
		size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, indicator, (int)wcslen(indicator), nullptr, 0, nullptr, 0);
		buffer = new char[size + 3];
		memcpy(buffer, first_indentation, 2);
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, indicator, (int)wcslen(indicator), buffer + 2, size, nullptr, 0);
		buffer[size + 2] = '\n';
		WriteFile(standard_error, buffer, size + 3, nullptr, 0);
		delete[] buffer;
		size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lines[current_line_number], (int)wcslen(lines[current_line_number]), nullptr, 0, nullptr, 0);
		buffer = new char[size + 5];
		memcpy(buffer, second_indentation, 4);
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lines[current_line_number], (int)wcslen(lines[current_line_number]), buffer + 4, size, nullptr, 0);
		buffer[size + 4] = '\n';
		WriteFile(standard_error, buffer, size + 5, nullptr, 0);
		delete[] buffer;
		break;
	}
	size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, error.error_message, (int)wcslen(error.error_message), nullptr, 0, nullptr, 0);
	buffer = new char[size + 1];
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, error.error_message, (int)wcslen(error.error_message), buffer, size, nullptr, 0);
	buffer[size] = '\n';
	WriteFile(standard_error, buffer, size + 1, nullptr, 0);
	delete[] buffer;
}

// perform sequence check (e.g. there must be only declaration statements inside record definition)
void SequenceCheck(size_t length) {
	bool define_type = false;
	bool define_subroutine = false;
	for (size_t index = 0; index != length; index++) {
		unsigned short& syntax_index = parsed_code[index].syntax_index;
		if (Construct::constructs[syntax_index] == Construct::type_ender) { define_type = false; continue; }
		if (Construct::constructs[syntax_index] == Construct::procedure_ender or
			Construct::constructs[syntax_index] == Construct::function_ender) {
			define_subroutine = false;
			continue;
		}
		if (define_type) {
			if (Construct::constructs[syntax_index] != Construct::declaration) {
				CII = index;
				throw Error(SyntaxError, L"结构体定义中仅能存在声明语句");
			}
		}
		if (define_subroutine) {
			if (Construct::constructs[syntax_index] == Construct::procedure_header or
				Construct::constructs[syntax_index] == Construct::function_header) {
				CII = index;
				throw Error(SyntaxError, L"在子程序中定义子程序是非法的");
			}
			if (Construct::constructs[syntax_index] == Construct::type_header or
				Construct::constructs[syntax_index] == Construct::pointer_type_header or
				Construct::constructs[syntax_index] == Construct::enumerated_type_header) {
				CII = index;
				throw Error(SyntaxError, L"在子程序中定义类型是非法的");
			}
		}
		if (Construct::constructs[syntax_index] == Construct::procedure_header or
			Construct::constructs[syntax_index] == Construct::function_header) { define_subroutine = true; }
		if (Construct::constructs[syntax_index] == Construct::type_header) { define_type = true; }
	}
}

// perform syntax check for each line and extract key information
bool SyntaxCheck(size_t length, wchar_t** lines) {
	calling_ptr = 0;
	parsed_code = new MatchedSyntax[length];
	for (CII = 0; CII != length; CII++) {
		bool matched = false;
		for (unsigned short index = 0; index != Construct::number_of_constructs; index++) {
			Result* result = (*(Result*(*)(wchar_t*))(Construct::constructs[index]))(lines[CII]);
			if (result->matched) {
				matched = true;
				if (result->error_message) {
					Error error(SyntaxError, result->error_message);
					FormatErrorMessage(error, lines);
					delete[] parsed_code;
					return false;
				}
				else {
					parsed_code[CII] = MatchedSyntax(index, result);
				}
				break;
			}
			else {
				delete result;
			}
		}
		if (not matched) {
			Error error(SyntaxError, L"无法识别的语法");
			FormatErrorMessage(error, lines);
			delete[] parsed_code;
			return false;
		}
	}
	if (nesting_ptr) {
		Error error(SyntaxError, L"出乎意料的EOF：嵌套结构未结束");
		CII = length - 1;
		FormatErrorMessage(error, lines);
		delete[] parsed_code;
		return false;
	}
	try { SequenceCheck(length); }
	catch(Error& error){
		FormatErrorMessage(error, lines);
		delete[] parsed_code;
		return false;
	}
	return true;
}

// process signal
INT_PTR SignalProc(UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case SIGNAL_CONNECTION:
	{
		if (wParam == CONNECTION_PIPE) {
			signal_out = (HANDLE)lParam;
		}
		else {
			SetEvent(breakpoint_handled);
		}
		break;
	}
	case SIGNAL_BREAKPOINT:
	{
		switch (LOWORD(wParam)) {
		case BREAKPOINT_ADD:
			WORD type = HIWORD(wParam);
			switch (type) {
			case BREAKPOINT_TYPE_NORMAL:
				breakpoints.push_back(BREAKPOINT{ (size_t)lParam, type });
				break;
			}
			break;
		}
	}
	case SIGNAL_EXECUTION:
	{
		step_type = wParam;
		SetEvent(breakpoint_handled);
		SetEvent(step_handled);
		break;
	}
	}
	return 0;
}

// thread used to monitor signal pipe
DWORD WINAPI SignalThread(LPVOID lpParameter) {
	static char buffer[20];
	DWORD content = 0;
	while (true) {
		PeekNamedPipe(signal_in, nullptr, 0, nullptr, &content, nullptr);
		if (content) {
			ReadFile(signal_in, buffer, 20, 0, 0);
			if (SignalProc(*(UINT*)buffer, *(WPARAM*)(buffer + 4), *(LPARAM*)(buffer + 12))) {
				return 0;
			}
		}
	}
}

// send signal to the server
void SendSignal(UINT message, WPARAM wParam, LPARAM lParam) {
	static char buffer[20];
	memcpy(buffer, &message, 4);
	memcpy(buffer + 4, &wParam, 8);
	memcpy(buffer + 12, &lParam, 8);
	WriteFile(signal_out, buffer, 1024, 0, nullptr);
}

// check whether the current line of code contains a breakpoint
inline bool CheckBreakpoint(ULONG64 line_index) {
	for (list<BREAKPOINT>::iterator iter = breakpoints.begin(); iter != breakpoints.end(); iter++) {
		if (iter->line_index == line_index) {
			return true;
		}
	}
	return false;
}

// interprete and execute the code without debugging the code
int ExecuteNormal(size_t length, wchar_t** lines) {
	calling_ptr = 0;
	if (not SyntaxCheck(length, lines)) {
		return -1;
	}
	for (CII = 0; CII != length; CII++) {
		try {
			((FUNCTION_PTR)Execution::executions[parsed_code[CII].syntax_index])(parsed_code[CII].result);
		}
		catch (Error& error) {
			if (globals->error_handling_ptr) {
				globals->error_handling_ptr--;
				current_instruction_index = globals->error_handling_indexes[globals->error_handling_ptr];
			}
			else {
				FormatErrorMessage(error, lines);
				return -1;
			}
		}
	}
	return 0;
}

// interprete and execute the code in debugging mode
int ExecuteDubug(size_t length, wchar_t** lines) {
	calling_ptr = 0;
	if (not SyntaxCheck(length, lines)) {
		return - 1;
	}
	for (CII = 0; CII != length; CII++) {
		if (CheckBreakpoint(current_instruction_index)) {
			ResetEvent(breakpoint_handled);
			SendSignal(SIGNAL_BREAKPOINT, BREAKPOINT_HIT, CII);
			WaitForSingleObject(breakpoint_handled, INFINITE);
		}
		else if (step_type == EXECUTION_STEPIN or step_type == EXECUTION_STEPOVER) {
			ResetEvent(step_handled);
			SendSignal(SIGNAL_EXECUTION, EXECUTION_STEPPED, CII);
			WaitForSingleObject(step_handled, INFINITE);
		}
		try {
			((FUNCTION_PTR)Execution::executions[parsed_code[CII].syntax_index])(parsed_code[CII].result);
		}
		catch (Error& error) {
			if (globals->error_handling_ptr) {
				globals->error_handling_ptr--;
				current_instruction_index = globals->error_handling_indexes[globals->error_handling_ptr];
			}
			else {
				FormatErrorMessage(error, lines);
				SendSignal(SIGNAL_CONNECTION, CONNECTION_EXIT, 0);
				return -1;
			}
		}
	}
	SendSignal(SIGNAL_CONNECTION, CONNECTION_EXIT, 0);
	return 0;
}

// program entry point
int wmain(int argc, wchar_t** args)
{
	// retrieve pipes
	standard_input = GetStdHandle(STD_INPUT_HANDLE);
	standard_output = GetStdHandle(STD_OUTPUT_HANDLE);
	standard_error = GetStdHandle(STD_ERROR_HANDLE);
	// process arguments
	static const wchar_t help_message[] = L"Command: \"Pseudocode Interpreter.exe\" "
		"<filename> [/debug] [/help] "
		"[/new_line] [/output_object] [/no_crlf_in_read] [/new_line_in_write] [/flush_immediately]\n\n"
		"Explanation:\n"
		"<filename>             Path of pseudocode file to interprete\n"
		"/debug                 Interprete in debug mode\n"
		"/help                  Output help message\n"
		"The following arguments are switch option(only has states enabled or disabled:\n"
		"/new_file              Enabled is default; New line will be added to the output message automatically\n"
		"/output_object         Enabled is default; Output values as objects; It allows array, record and enumerated to output\n"
		"/no_crlf_in_read       Enabled is default; Discard CRLF(carriage return/line feed) when READLINE is executed\n"
		"/new_line_in_write     Enabled is default; Automatic append CRLF to the end of line when WRITELINE is executed\n"
		"/flush_immediately     Disabled is default; Flush the file buffers immediately after WRITELINE is executed\n\n"
		"Insert a '-' before argument name to disable a switch option. e.g. /-new_line";
	static const wchar_t* match_args[] = { L"/debug", L"/help", L"/new_file", L"/-new_file",
		L"/output_object", L"/-output_object", L"/no_crlf_in_read", L"/-no_crlf_in_read", L"/new_line_in_write",
		L"/-new_line_in_write", L"/flush_immediately", L"/-flush_immediately"
	};
	wchar_t* filename = nullptr;
	for (int arg_index = 1; arg_index != argc; arg_index++) {
		for (unsigned short match_index = 0; match_index != sizeof(match_args) / sizeof(wchar_t*); match_index++) {
			if (wcslen(match_args[match_index]) == wcslen(args[arg_index])) {
				if (memcmp(match_args[match_index], args[arg_index], wcslen(match_args[match_index])) == 0) {
					switch (match_index) {
					case 0:
						debug = true;
						break;
					case 1:
						WRITE_CONSOLE(standard_output, help_message, sizeof(help_message));
						return 0;
					case 2:
						settings |= AUTOMATIC_NEW_LINE_AFTER_OUTPUT;
						break;
					case 3:
						settings &= ~AUTOMATIC_NEW_LINE_AFTER_OUTPUT;
						break;
					case 4:
						settings |= OUTPUT_AS_OBJECT;
						break;
					case 5:
						settings &= ~OUTPUT_AS_OBJECT;
						break;
					case 6:
						settings |= DISCARD_CRLF_IN_READ;
						break;
					case 7:
						settings &= ~DISCARD_CRLF_IN_READ;
						break;
					case 8:
						settings |= AUTOMATIC_NEW_LINE_IN_WRITE;
						break;
					case 9:
						settings &= ~AUTOMATIC_NEW_LINE_IN_WRITE;
						break;
					case 10:
						settings |= FLUSH_FILE_BUFFERS_AFTER_WRITE;
						break;
					case 11:
						settings &= ~FLUSH_FILE_BUFFERS_AFTER_WRITE;
						break;
					default:
						if (args[arg_index][0] != L'/') {
							if (not filename) {
								filename = args[arg_index];
							}
							else {
								static const wchar_t* error_message = L"无法识别的参数列表";
								WRITE_CONSOLE(standard_error, error_message, 9);
								return 1;
							}
						}
						else {
							static const wchar_t* error_message = L"无法识别的参数列表";
							WRITE_CONSOLE(standard_error, error_message, 9);
							return 1;
						}
						break;
					}
				}
			}
		}
	}
	HANDLE hfile = CreateFileW(args[1], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hfile == INVALID_HANDLE_VALUE) {
		static const wchar_t* error_message = L"伪代码文件未能打开";
		WRITE_CONSOLE(standard_error, error_message, 9);
		return 1;
	}
	LARGE_INTEGER filesize{};
	GetFileSizeEx(hfile, &filesize);
	char* buffer = new char[filesize.QuadPart + 1];
	buffer[filesize.QuadPart] = 0;
	DWORD read = 0;
	if (not ReadFile(hfile, buffer, (DWORD)filesize.QuadPart, &read, 0)) {
		static const wchar_t* error_message = L"读取失败";
		WRITE_CONSOLE(standard_error, error_message, 4);
		return 1;
	}
	int size = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, buffer, -1, nullptr, 0);
	wchar_t* wchar_buffer = new wchar_t[size];
	MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, buffer, -1, wchar_buffer, size);
	delete[] buffer;
	size_t line_number = 0;
	wchar_t** lines = FormatCode(wchar_buffer, &line_number);
	if (debug) {
		static char signal_handle[8];
		ReadFile(standard_input, signal_handle, 8, nullptr, nullptr);
		signal_in = *(HANDLE*)signal_handle;
		CreateThread(0, 0, SignalThread, 0, 0, 0);
		WaitForSingleObject(breakpoint_handled, INFINITE);
		return ExecuteDubug(line_number, lines);
	}
	else {
		return ExecuteNormal(line_number, lines);
	}
}
