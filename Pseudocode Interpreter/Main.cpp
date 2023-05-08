#include <Windows.h>
#include <Windowsx.h>
#include <cmath>
#include <ctime>
#include <list>
#include <strsafe.h>
#include "Debug.h"
#include "Parser.h"
#include "Execution.h"
#define SEGMENT_SIZE 1024 // bytes read from file each time

using namespace std;
HANDLE standard_input;
HANDLE standard_output;
HANDLE standard_error;
HANDLE signal_in = 0; // used for debugging mode
HANDLE signal_out = 0; // used for debugging mode
IndexedList<wchar_t*> lines;
size_t& CII = current_instruction_index;

unsigned settings = default_settings;
// these are used for debugging mode
bool debug = false;
list<BREAKPOINT> breakpoints;
HANDLE breakpoint_handled = CreateEvent(0, true, false, 0);
DWORD step_type;
USHORT step_depth = 0; // in which call stepping is made (-1 means step in)
HANDLE step_handled = CreateEvent(0, true, false, 0);

// load physical files and store in a wide char buffer
wchar_t* LoadFile(wchar_t* filename) {
	HANDLE hfile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hfile == INVALID_HANDLE_VALUE) {
		WRITE_CONSOLE(standard_error, L"伪代码文件未能打开", 9);
		abort();
	}
	LARGE_INTEGER filesize{};
	GetFileSizeEx(hfile, &filesize);
	if (filesize.QuadPart >= 1ul << (sizeof(int) * 8 - 1)) {
		WRITE_CONSOLE(standard_error, L"文件过大", 4);
		abort();
	}
	char* buffer = new char[filesize.QuadPart];
	static DWORD read;
	if (not ReadFile(hfile, buffer, (DWORD)filesize.QuadPart, &read, NULL)) {
		WRITE_CONSOLE(standard_error, L"读取失败", 4);
		abort();
	}
	if (read != filesize.QuadPart) {
		WRITE_CONSOLE(standard_error, L"读取异常", 4);
		abort();
	}
	int size = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, buffer, read, nullptr, 0);
	wchar_t* wchar_buffer = new wchar_t[size + 1];
	MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, buffer, read, wchar_buffer, size);
	wchar_buffer[size] = 0;
	delete[] buffer;
	return wchar_buffer;
}

void FormatCode(wchar_t* data) {
	auto LineStrip = [](wchar_t* line, size_t size) {
		ULONG index;
		for (index = 0; line[index] != 0; index++) {
			if (line[index] != L' ' and line[index] != L'\t') {
				break;
			}
		}
		memcpy(line, line + index, (size - index + 1) * 2);
		for (index = 0; line[index] != 0; index++) {
			if (line[index] == L'\\' and line[index + 1] == L'\\') {
				break;
			}
		}
		line[index] = 0;
	};
	lines.set_construction(true);
	ULONG64 last_return = 0;
	for (ULONG64 index = 0;; index++) {
		if (data[index] == L'\r' or data[index] == 0) {
			wchar_t* line_buffer = new wchar_t[index - last_return + 1];
			memcpy(line_buffer, data + last_return, ((size_t)index - last_return) * 2);
			line_buffer[index - last_return] = 0;
			LineStrip(line_buffer, index - last_return);
			lines.append(line_buffer);
			if (data[index] == 0) {
				break;
			}
			else {
				last_return = index + 2;
			}
		}
	}
	lines.set_construction(false);
}

// format error message when an error is detected
void FormatErrorMessage(Error error) {
	static const wchar_t* error_type[] = {
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
	static const wchar_t* tag_indicator[] = { L"位于第 ", L" 行，在 " };
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
	size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, *lines[current_line_number], (int)wcslen(*lines[current_line_number]), nullptr, 0, nullptr, 0);
	buffer = new char[size + 5];
	memcpy(buffer, second_indentation, 4);
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, *lines[current_line_number], (int)wcslen(*lines[current_line_number]), buffer + 4, size, nullptr, 0);
	buffer[size + 4] = '\n';
	WriteFile(standard_error, buffer, size + 5, nullptr, 0);
	delete[] buffer;
	// call in stack
	for (USHORT index = 0; index != calling_ptr; index++) {
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
		size = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, *lines[current_line_number], (int)wcslen(*lines[current_line_number]), nullptr, 0, nullptr, 0);
		buffer = new char[size + 5];
		memcpy(buffer, second_indentation, 4);
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, *lines[current_line_number], (int)wcslen(*lines[current_line_number]), buffer + 4, size, nullptr, 0);
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
void SequenceCheck() {
	bool define_type = false;
	Nesting* nesting[128];
	USHORT nesting_ptr = 0;
	for (CII = 0; CII != lines.size(); CII++) {
		USHORT& syntax_index = parsed_code[CII]->syntax_index;
		if (define_type) {
			if (syntax_index != Construct::_declaration and syntax_index != Construct::_type_ender) {
				throw Error(SyntaxError, L"结构体内仅能出现声明语句");
			}
		}
		switch (syntax_index) {
			// IF
		case Construct::_if_header_1:
			if (nesting_ptr == 128) { throw Error(SyntaxError, L"嵌套深度到达上限"); }
			nesting[nesting_ptr] = new Nesting(Nesting::IF, CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_if_header_2:
			if (nesting_ptr == 128) { throw Error(SyntaxError, L"嵌套深度到达上限"); }
			nesting[nesting_ptr] = new Nesting(Nesting::IF, CII);
			nesting[nesting_ptr]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_then_tag:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与THEN标签配对的IF头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::IF) { throw Error(SyntaxError, L"未找到与THEN标签配对的IF头"); }
			if (nesting[nesting_ptr - 1]->tag_number != 1) { throw Error(SyntaxError, L"重定义THEN标签是非法的"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			break;
		case Construct::_else_tag:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与ELSE标签配对的IF头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::IF) { throw Error(SyntaxError, L"未找到与ELSE标签配对的IF头"); }
			if (nesting[nesting_ptr - 1]->tag_number != 2) { throw Error(SyntaxError, L"重定义ELSE标签是非法的"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			break;
		case Construct::_if_ender:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与ELSE标签配对的IF头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::IF) { throw Error(SyntaxError, L"未找到与ELSE标签配对的IF头"); }
			if (nesting[nesting_ptr - 1]->tag_number == 1) { throw Error(SyntaxError, L"未找到THEN标签"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			nesting_ptr--;
			break;
			// CASE OF
		case Construct::_case_of_header:
			if (nesting_ptr == 128) { throw Error(SyntaxError, L"嵌套深度到达上限"); }
			nesting[nesting_ptr] = new Nesting(Nesting::CASE, CII);
			nesting[nesting_ptr]->nest_info->case_of_info.number_of_values = 0;
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_case_tag:
		{
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到CASE OF头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::CASE) { throw Error(SyntaxError, L"未找到CASE OF头"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			RPN_EXP** new_rpn_list = new RPN_EXP * [nesting[nesting_ptr - 1]->tag_number - 1];
			memcpy(new_rpn_list, nesting[nesting_ptr - 1]->nest_info->case_of_info.values, sizeof(RPN_EXP*) * (size_t)(nesting[nesting_ptr - 1]->tag_number - 2));
			new_rpn_list[nesting[nesting_ptr - 1]->tag_number - 2] = (RPN_EXP*)parsed_code[CII]->result.args[1];
			nesting[nesting_ptr - 1]->nest_info->case_of_info.values = new_rpn_list;
			nesting[nesting_ptr - 1]->nest_info->case_of_info.number_of_values++;
			break;
		}
		case Construct::_otherwise_tag:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到CASE OF头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::CASE) { throw Error(SyntaxError, L"未找到CASE OF头"); }
			if (nesting[nesting_ptr - 1]->tag_number == 1) { throw Error(SyntaxError, L"在定义OTHERWISE标签前需定义至少一个分支"); }
			if (nesting[nesting_ptr - 1]->nest_info->case_of_info.otherwise_defined) { throw Error(SyntaxError, L"重定义OTHERWISE标签是非法的"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			nesting[nesting_ptr - 1]->nest_info->case_of_info.otherwise_defined = true;
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			break;
		case Construct::_case_of_ender:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与ENDCASE标签配对的CASE OF头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::CASE) { throw Error(SyntaxError, L"未找到与ENDCASE标签配对的CASE OF头"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			nesting_ptr--;
			break;
			// FOR
		case Construct::_for_header_1:
			if (nesting_ptr == 128) { throw Error(SyntaxError, L"嵌套深度到达上限"); }
			nesting[nesting_ptr] = new Nesting(Nesting::FOR, CII);
			nesting[nesting_ptr]->nest_info->for_info.init = false;
			nesting[nesting_ptr]->nest_info->for_info.step = nullptr;
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_for_header_2:
			if (nesting_ptr == 128) { throw Error(SyntaxError, L"嵌套深度到达上限"); }
			nesting[nesting_ptr] = new Nesting(Nesting::FOR, CII);
			nesting[nesting_ptr]->nest_info->for_info.init = false;
			nesting[nesting_ptr]->nest_info->for_info.step = nullptr;
			nesting[nesting_ptr]->nest_info->for_info.step = (RPN_EXP*)parsed_code[CII]->result.args[4];
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_for_ender:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与NEXT标签配对的FOR头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::FOR) { throw Error(SyntaxError, L"未找到与NEXT标签配对的FOR头"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			nesting_ptr--;
			break;
			// WHILE
		case Construct::_while_header:
			if (nesting_ptr == 128) { throw Error(SyntaxError, L"嵌套深度到达上限"); }
			nesting[nesting_ptr] = new Nesting(Nesting::WHILE, CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_while_ender:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与ENDWHILE标签配对的WHILE头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::WHILE) { throw Error(SyntaxError, L"未找到与ENDWHILE标签配对的WHILE头"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			nesting_ptr--;
			break;
			// REPEAT
		case Construct::_repeat_header:
			if (nesting_ptr == 128) { throw Error(SyntaxError, L"嵌套深度到达上限"); }
			nesting[nesting_ptr] = new Nesting(Nesting::REPEAT, CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_repeat_ender:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与UNTIL标签配对的REPEAT头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::REPEAT) { throw Error(SyntaxError, L"未找到与UNTIL标签配对的REPEAT头"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			nesting_ptr--;
			break;
			// TRY
		case Construct::_try_header:
			if (nesting_ptr == 128) { throw Error(SyntaxError, L"嵌套深度到达上限"); }
			nesting[nesting_ptr] = new Nesting(Nesting::TRY, CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_except_tag:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与EXCEPT标签配对的TRY头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::TRY) { throw Error(SyntaxError, L"未找到与EXCEPT标签配对的TRY头"); }
			if (nesting[nesting_ptr - 1]->tag_number != 1) { throw Error(SyntaxError, L"重定义EXCEPT标签是非法的"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			break;
		case Construct::_try_ender:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与ENDTRY标签配对的TRY头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::TRY) { throw Error(SyntaxError, L"未找到与ENDTRY标签配对的TRY头"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			nesting_ptr--;
			break;
			// PROCEDURE
		case Construct::_procedure_header:
			if (nesting_ptr) { throw Error(SyntaxError, L"子过程的定义必须位于全局且不被条件约束"); }
			nesting[nesting_ptr] = new Nesting(Nesting::PROCEDURE, CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_procedure_ender:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与ENDPROCEDURE标签配对的PROCEDURE头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::PROCEDURE) { throw Error(SyntaxError, L"未找到与ENDPROCEDURE标签配对的PROCEDURE头"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			nesting_ptr--;
			break;
			// FUNCTION
		case Construct::_function_header:
			if (nesting_ptr) { throw Error(SyntaxError, L"子过程的定义必须位于全局且不被条件约束"); }
			nesting[nesting_ptr] = new Nesting(Nesting::FUNCTION, CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr];
			nesting_ptr++;
			break;
		case Construct::_function_ender:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与ENDFUNCTION标签配对的FUNCTION头"); }
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::FUNCTION) { throw Error(SyntaxError, L"未找到与ENDFUNCTION标签配对的FUNCTION头"); }
			nesting[nesting_ptr - 1]->add_tag(CII);
			parsed_code[CII]->result.args[0] = nesting[nesting_ptr - 1];
			nesting_ptr--;
			break;
			// CONTINUE
		case Construct::_continue_tag:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与CONTINUE标签配对的FOR, WHILE或REPEAT头"); }
			for (USHORT depth = nesting_ptr - 1;; depth--) {
				if (nesting[depth]->nest_type == Nesting::FOR or
					nesting[depth]->nest_type == Nesting::WHILE or
					nesting[depth]->nest_type == Nesting::REPEAT) {
					parsed_code[CII]->result.args[0] = nesting[depth];
					break;
				}
				if (depth == 0) { throw Error(SyntaxError, L"未找到与CONTINUE标签配对的FOR, WHILE或REPEAT头"); }
			}
			break;
			// BREAK
		case Construct::_break_tag:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与BREAK标签配对的FOR, WHILE或REPEAT头"); }
			for (USHORT depth = nesting_ptr - 1;; depth--) {
				if (nesting[depth]->nest_type == Nesting::FOR or
					nesting[depth]->nest_type == Nesting::WHILE or
					nesting[depth]->nest_type == Nesting::REPEAT) {
					parsed_code[CII]->result.args[0] = nesting[depth];
					break;
				}
				if (depth == 0) { throw Error(SyntaxError, L"未找到与BREAK标签配对的FOR, WHILE或REPEAT头"); }
			}
			break;
			// RETURN
		case Construct::_return_statement:
			if (not nesting_ptr) { throw Error(SyntaxError, L"未找到与RETURN标签配对的PROCEDURE或FUNCTION头"); }
			if (nesting[0]->nest_type == Nesting::PROCEDURE or
				nesting[0]->nest_type == Nesting::FUNCTION) {
				parsed_code[CII]->result.args[0] = nesting[0];
			}
			else {
				throw Error(SyntaxError, L"未找到与RETURN标签配对的PROCEDURE或FUNCTION头");
			}
			break;
		}
	}
	if (nesting_ptr) {
		CII = lines.size() - 1;
		throw Error(SyntaxError, L"出乎意料的EOF：嵌套结构未结束");
	}
}

// perform syntax check for each line and extract key information
bool SyntaxCheck() {
	calling_ptr = 0;
	parsed_code = new CONSTRUCT * [lines.size()];
	for (CII = 0; CII != lines.size(); CII++) {
		CONSTRUCT* construct = Construct::parse(*lines[CII]);
		parsed_code[CII] = construct;
		if (not construct->result.matched) {
			Error error(SyntaxError, L"无法识别的语法");
			FormatErrorMessage(error);
			delete[] parsed_code;
			return false;
		}
	}
	try { SequenceCheck(); }
	catch (Error& error) {
		FormatErrorMessage(error);
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
		switch (wParam) {
		case EXECUTION_STEPIN:
			step_depth = (USHORT)-1;
			break;
		case EXECUTION_STEPOVER:
			step_depth = calling_ptr;
			break;
		case EXECUTION_STEPOUT:
			step_depth = calling_ptr - 1;
			break;
		}
		SetEvent(breakpoint_handled);
		SetEvent(step_handled);
		step_type = wParam;
		break;
	}
	}
	return 0;
}

// thread used to monitor signal pipe
DWORD SignalThread(LPVOID lpParameter) {
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
int ExecuteNormal() {
	calling_ptr = 0;
	if (not SyntaxCheck()) {
		return -1;
	}
	for (CII = 0; CII != lines.size(); CII++) {
		try {
			((FUNCTION_PTR)Execution::executions[parsed_code[CII]->syntax_index])(parsed_code[CII]->result);
		}
		catch (Error& error) {
			if (globals->error_handling_ptr) {
				globals->error_handling_ptr--;
				current_instruction_index = globals->error_handling_indexes[globals->error_handling_ptr];
			}
			else {
				FormatErrorMessage(error);
				return -1;
			}
		}
	}
	for (ULONG index = 0; index != lines.size(); index++) {
		parsed_code[index]->release_tokens();
		delete parsed_code[index];
	}
	return 0;
}

// interpret and execute the code in debugging mode
int ExecuteDubug() {
	calling_ptr = 0;
	if (not SyntaxCheck()) {
		return -1;
	}
	for (CII = 0; CII != lines.size(); CII++) {
		if (CheckBreakpoint(current_instruction_index)) {
			ResetEvent(breakpoint_handled);
			SendSignal(SIGNAL_BREAKPOINT, BREAKPOINT_HIT, CII);
			WaitForSingleObject(breakpoint_handled, INFINITE);
		}
		else if ((step_type == EXECUTION_STEPIN or step_type == EXECUTION_STEPOVER) and step_depth >= 0) {
			ResetEvent(step_handled);
			SendSignal(SIGNAL_EXECUTION, EXECUTION_STEPPED, CII);
			WaitForSingleObject(step_handled, INFINITE);
		}
		try {
			((FUNCTION_PTR)Execution::executions[parsed_code[CII]->syntax_index])(parsed_code[CII]->result);
		}
		catch (Error& error) {
			if (globals->error_handling_ptr) {
				globals->error_handling_ptr--;
				current_instruction_index = globals->error_handling_indexes[globals->error_handling_ptr];
			}
			else {
				FormatErrorMessage(error);
				SendSignal(SIGNAL_CONNECTION, CONNECTION_EXIT, 0);
				return -1;
			}
		}
	}
	SendSignal(SIGNAL_CONNECTION, CONNECTION_EXIT, 0);
	for (ULONG index = 0; index != lines.size(); index++) {
		parsed_code[index]->release_tokens();
	}
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
		for (USHORT match_index = 0; match_index != sizeof(match_args) / sizeof(wchar_t*); match_index++) {
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
	wchar_t* data = LoadFile(args[1]);
	FormatCode(data);
	if (debug) {
		static char signal_handle[8];
		ReadFile(standard_input, signal_handle, 8, nullptr, nullptr);
		signal_in = *(HANDLE*)signal_handle;
		CreateThread(0, 0, SignalThread, 0, 0, 0);
		WaitForSingleObject(breakpoint_handled, INFINITE);
		return ExecuteDubug();
	}
	else {
		return ExecuteNormal();
	}
}
