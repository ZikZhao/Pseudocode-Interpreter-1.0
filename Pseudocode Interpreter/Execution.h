#pragma once
#pragma comment(linker, "/STACK:20971520")
#ifdef EOF
#undef EOF
#endif
#define WRITE_CONSOLE(handle, lpStr, cchStr) {int size = WideCharToMultiByte(CP_ACP, NULL, lpStr, cchStr, nullptr, NULL, nullptr, nullptr); char* buffer = new char[size]; WideCharToMultiByte(CP_ACP, NULL, lpStr, cchStr, buffer, size, nullptr, nullptr); WriteFile(handle, buffer, size, nullptr, nullptr);}

extern HANDLE standard_input;
extern HANDLE standard_output;
extern HANDLE standard_error;
extern unsigned settings;
extern bool debug;
extern HANDLE breakpoint_handled;
extern DWORD step_type;
extern USHORT step_depth;
extern HANDLE step_handled;
void SendSignal(UINT message, WPARAM wParam, LPARAM lParam);
bool CheckBreakpoint(ULONG64 line_index);
typedef void (*FUNCTION_PTR)(RESULT);


File* files = new File[8]; // maximum 8 files opened at the same time
USHORT file_ptr = 0;

CONSTRUCT** parsed_code = nullptr;
BinaryTree* current_locals = nullptr;
BinaryTree* globals = new BinaryTree;
BinaryTree* enumerations = new BinaryTree; // a binary tree storing all the enumerated constants
bool define_record = false;
wchar_t* record_name;
BinaryTree record_fields; // data structure used to store declaration statements of record type
CALLSTACK callstack;
DATA* return_value = nullptr;

DATA* evaluate(RPN_EXP* expr); // forward declaration
DATA* evaluate_variable(wchar_t* path, bool* constant = nullptr); // forward declaration
DATA* evaluate_variable(DATA* current, wchar_t* expr); // forward declaration
DATA* function_calling(wchar_t* function_name, USHORT number_of_args, DATA** args); // forward declaration

inline BinaryTree::Node* find_variable(wchar_t* variable_name) {
	BinaryTree::Node* local_node = nullptr;
	if (current_locals) {
		local_node = current_locals->find(variable_name);
	}
	BinaryTree::Node* global_node = globals->find(variable_name);
	return local_node ? local_node : global_node;
}

DATA* evaluate_type(wchar_t* expr) {
	USHORT type = 0;
	if (Element::fundamental_type(expr, &type)) {
		DATA* type_data = new DATA{ type, nullptr };
		return type_data;
	}
	else if (Element::array_type(expr)) {
		DATA* type_data = new DATA{ 6, nullptr };
		return type_data;
	}
	else {
		BinaryTree::Node* node = find_variable(expr);
		if (not node) {
			throw Error(TypeError, L"类型不存在");
		}
		else if (node->value->type != 7 and node->value->type != 9 and node->value->type != 11) {
			throw Error(TypeError, L"类型不合法");
		}
		else {
			return new DATA{ node->value->type, node->value->value }; // second field cannot be deleted
		}
	}
}

namespace Builtins {
	USHORT number_of_args_list[] = { 3, 1, 2, 2, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 2, 1, 1 };
	const wchar_t* function_name_list[] = { L"MID", L"LENGTH", L"LEFT", L"RIGHT", L"ASC", L"CHR", L"MOD",
		L"DIV", L"LCASE", L"UCASE", L"TO_LOWER", L"TO_UPPER", L"INT", L"STRING_TO_NUM", L"RANDINT", L"EOF", L"HASH"};
	bool check_args(DATA** args, USHORT function_index, const USHORT* types, wchar_t** error_message_out = nullptr) {
		USHORT& number_of_args = number_of_args_list[function_index];
		static wchar_t* function_name = const_cast<wchar_t*>(function_name_list[function_index]);
		for (USHORT index = 0; index != number_of_args; index++) {
			if (args[index]->type != types[index]) {
				if (error_message_out) {
					static wchar_t const* message_template = L"函数：第参数与定义时的数据类型不符";
					static size_t length = wcslen(message_template);
					wchar_t* args_index = unsigned_to_string((size_t)index + 1);
					wchar_t* error_message = new wchar_t[wcslen(function_name) + length + wcslen(args_index) + 2];
					memcpy(error_message, function_name, wcslen(function_name) * 2);
					error_message[wcslen(function_name)] = 32;
					memcpy(error_message + wcslen(function_name) + 1, message_template, 8);
					memcpy(error_message + wcslen(function_name) + 5, args_index, wcslen(args_index) * 2);
					memcpy(error_message + wcslen(function_name) + 5 + wcslen(args_index), message_template + 4, length * 2);
					*error_message_out = error_message;
				}
				return false;
			}
		}
		return true;
	}
	DATA* MID(DATA** args) {
		DATA*& ThisString = args[0];
		DATA*& x = args[1];
		DATA*& y = args[2];
		static USHORT types[] = { 3, 0, 0 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 0, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t* string = ((DataType::String*)ThisString->value)->string;
		size_t start = (size_t)((DataType::Integer*)x->value)->value;
		size_t length = (size_t)((DataType::Integer*)y->value)->value;
		if (start >= ((DataType::String*)ThisString->value)->length) {
			throw Error(ArgumentError, L"MID函数：截取起点超出字符串下标范围");
		}
		DATA* result_data = new DATA{ 3, new DataType::String(length, string + start) };
		return result_data;
	}
	DATA* LENGTH(DATA** args) {
		DATA*& ThisString = args[0];
		static USHORT types[] = { 3 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 1, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t* string = ((DataType::String*)ThisString->value)->string;
		DATA* result_data = new DATA{ 0, new DataType::Integer(wcslen(string)) };
		return result_data;
	}
	DATA* LEFT(DATA** args) {
		DATA*& ThisString = args[0];
		DATA*& x = args[1];
		static USHORT types[] = { 3, 0 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 2, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t* string = ((DataType::String*)ThisString->value)->string;
		DATA* result_data = new DATA{ 3, new DataType::String(((DataType::Integer*)x->value)->value, string) };
		return result_data;
	}
	DATA* RIGHT(DATA** args) {
		DATA*& ThisString = args[0];
		DATA*& x = args[1];
		static USHORT types[] = { 3, 0 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 3, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t* string = ((DataType::String*)ThisString->value)->string;
		size_t offset = ((DataType::Integer*)x->value)->value > (long long)wcslen(string) ? 0 : wcslen(string) - ((DataType::Integer*)x->value)->value;
		DATA* result_data = new DATA{ 3, new DataType::String(((DataType::Integer*)x->value)->value, string + offset) };
		return result_data;
	}
	DATA* ASC(DATA** args) {
		DATA*& ThisString = args[0];
		static USHORT types[] = { 2 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 4, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t value = ((DataType::Char*)ThisString->value)->value;
		return new DATA{ 0, new DataType::Integer((long long)value) };
	}
	DATA* CHR(DATA** args) {
		DATA*& x = args[0];
		static USHORT types[] = { 0 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 5, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		long long ascurrent_instruction_index_value = ((DataType::Integer*)x->value)->value;
		if (ascurrent_instruction_index_value >> 16) {
			throw Error(ArgumentError, L"整数过大无法转换（应介于0到65535之间）");
		}
		return new DATA{ 2, new DataType::Char((wchar_t)ascurrent_instruction_index_value) };
	}
	DATA* MOD(DATA** args) {
		DATA*& ThisNum = args[0];
		DATA*& ThisDiv = args[1];
		static USHORT types[] = { 0, 0 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 6, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		long long dividend = ((DataType::Integer*)ThisNum->value)->value;
		long long divisor = ((DataType::Integer*)ThisDiv->value)->value;
		if (divisor == 0) {
			throw Error(ArgumentError, L"除数不能为0");
		}
		return new DATA{ 0, new DataType::Integer(dividend % divisor) };
	}
	DATA* DIV(DATA** args) {
		DATA*& ThisNum = args[0];
		DATA*& ThisDiv = args[1];
		static USHORT types[] = { 0, 0 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 7, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		long long dividend = ((DataType::Integer*)ThisNum->value)->value;
		long long divisor = ((DataType::Integer*)ThisDiv->value)->value;
		if (divisor == 0) {
			throw Error(ArgumentError, L"除数不能为0");
		}
		return new DATA{ 0, new DataType::Integer(dividend / divisor) };
	}
	DATA* LCASE(DATA** args) {
		DATA*& x = args[0];
		static USHORT types[] = { 2 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 8, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t value = ((DataType::Char*)x->value)->value;
		if (value >= 65 and value <= 90) {
			return new DATA{ 2, new DataType::Char((wchar_t)(value + 32)) };
		}
		else {
			return DataType::copy(x);
		}
	}
	DATA* UCASE(DATA** args) {
		DATA*& x = args[0];
		static USHORT types[] = { 2 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 9, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t value = ((DataType::Char*)x->value)->value;
		if (value >= 97 and value <= 122) {
			return new DATA{ 2, new DataType::Char((wchar_t)(value - 32)) };
		}
		else {
			return DataType::copy(x);
		}
	}
	DATA* TO_LOWER(DATA** args) {
		DATA*& x = args[0];
		static USHORT types[] = { 3 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 10, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t* original = ((DataType::String*)x->value)->string;
		DataType::String* result_obj = new DataType::String(wcslen(original), original);
		wchar_t*& string = result_obj->string;
		for (size_t index = 0; string[index] != 0; index++) {
			if (string[index] >= 65 and string[index] <= 90) {
				string[index] += 32;
			}
		}
		return new DATA{ 3, result_obj };
	}
	DATA* TO_UPPER(DATA** args) {
		DATA*& x = args[0];
		static USHORT types[] = { 3 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 11, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t* original = ((DataType::String*)x->value)->string;
		DataType::String* result_obj = new DataType::String(wcslen(original), original);
		wchar_t*& string = result_obj->string;
		for (size_t index = 0; string[index] != 0; index++) {
			if (string[index] >= 97 and string[index] <= 122) {
				string[index] -= 32;
			}
		}
		return new DATA{ 3, result_obj };
	}
	DATA* INT(DATA** args) {
		DATA*& x = args[0];
		static USHORT types[] = { 1 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 12, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		long double value = ((DataType::Real*)x->value)->value;
		return new DATA{ 0, new DataType::Integer((long long)value) };
	}
	DATA* STRING_TO_NUM(DATA** args) {
		DATA*& x = args[0];
		static USHORT types[] = { 3 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 13, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t* string = ((DataType::String*)x->value)->string;
		long double result = string_to_real(string);
		return new DATA{ 1, new DataType::Real(result) };
	}
	DATA* RANDINT(DATA** args) {
		DATA*& lower = args[0];
		DATA*& upper = args[1];
		static USHORT types[] = { 0, 0 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 14, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		long long& lower_bound = ((DataType::Integer*)lower->value)->value;
		long long& upper_bound = ((DataType::Integer*)upper->value)->value;
		srand((unsigned int)time(0));
		return new DATA{ 0, new DataType::Integer(rand() % (upper_bound - lower_bound + 1) + lower_bound) };
	}
	DATA* EOF(DATA** args) {
		// EOF is defined as a macro in stdio.h
		DATA*& filename = args[0];
		static USHORT types[] = { 3 };
		wchar_t* error_message_out = nullptr;
		if (not check_args(args, 15, types, &error_message_out)) {
			throw Error(ArgumentError, error_message_out);
		}
		wchar_t*& string = ((DataType::String*)filename->value)->string;
		for (USHORT file_index = 0; file_index != file_ptr; file_index++) {
			if (wcslen(string) == wcslen(files[file_index].filename)) {
				bool matched = true;
				for (size_t index = 0; string[index] != 0; index++) {
					if (string[index] != files[file_index].filename[index]) {
						matched = false;
						break;
					}
				}
				if (matched) {
					return new DATA{ 4, new DataType::Boolean(files[file_index].eof) };
				}
			}
		}
		throw Error(ValueError, L"文件未打开");
	}
	DATA* HASH(DATA** args) {
		DATA*& x = args[0];
		size_t hash_value = 0;
		switch (x->type) {
		case 0:
		{
			long long value = ((DataType::Integer*)x->value)->value;
			hash_value = std::hash<long long>()(value);
			break;
		}
		case 1:
		{
			long double value = ((DataType::Real*)x->value)->value;
			hash_value = std::hash<long double>()(value);
		}
		case 2:
		{
			wchar_t value = ((DataType::Char*)x->value)->value;
			hash_value = std::hash<wchar_t>()(value);
			break;
		}
		case 3:
		{
			wchar_t* string = ((DataType::String*)x->value)->string;
			long long value = 0;
			for (size_t index = 0; string[index] != 0; index++) {
				value <<= 16;
				value += string[index];
			}
			hash_value = std::hash<long long>()(value);
			break;
		}
		case 4:
		{
			bool value = ((DataType::Boolean*)x->value)->value;
			hash_value = std::hash<bool>()(value);
			break;
		}
		case 5:
		{
			DataType::Date* date = (DataType::Date*)x->value;
			size_t value = (size_t)date->second + ((size_t)date->minute << 6) + ((size_t)date->hour << 12) +
				((size_t)date->day << 17) + ((size_t)date->month << 22) + ((size_t)date->year << 26);
			hash_value = std::hash<size_t>()(value);
			break;
		}
		default:
			throw Error(ArgumentError, L"此类型数据不可哈希");
			break;
		}
		return new DATA{ 0, new DataType::Integer(hash_value) };
	}
	void* find(wchar_t* function_name, USHORT* args_number_out = nullptr) {
		static void* functions[] = {
			MID, LENGTH, LEFT, RIGHT, ASC, CHR, MOD, DIV, LCASE, UCASE, TO_LOWER, TO_UPPER,
			INT, STRING_TO_NUM, RANDINT, EOF, HASH
		};
		static USHORT number_of_functions = sizeof(functions) / sizeof(void*);
		for (USHORT index = 0; index != number_of_functions; index++) {
			if (function_name_list[index][0] == function_name[0] and wcslen(function_name) == wcslen(function_name_list[index])) {
				bool matched = true;
				for (USHORT index2 = 0; function_name[index2] != 0; index2++) {
					if (function_name_list[index][index2] != function_name[index2]) {
						matched = false;
						break;
					}
				}
				if (matched) {
					if (args_number_out) { *args_number_out = number_of_args_list[index]; }
					return functions[index];
				}
			}
		}
		return nullptr;
	}
}

namespace Execution {
	void empty_line(RESULT result) {
		UNREFERENCED_PARAMETER(result);
	}
	void declaration(RESULT result) {
		USHORT count = *(USHORT*)result.args[1];
		BinaryTree& scope = current_locals ? *current_locals : *globals;
		for (USHORT variable_index = 0; variable_index != count; variable_index++) {
			if (define_record) {
				if (record_fields.find(((wchar_t**)result.args[2])[variable_index])) {
					throw Error(TypeError, L"字段已在记录类型中声明");
				}
			}
			else {
				if (scope.find(((wchar_t**)result.args[2])[variable_index])) {
					throw Error(VariableError, L"变量已在当前域中声明");
				}
			}
			DATA* data = nullptr;
			if (((wchar_t*)result.args[0])[0] == L'V') { // variable type
				DATA* type_data = evaluate_type((wchar_t*)result.args[3]);
				data = DataType::new_empty_data(type_data);
				data->variable_data = true;
				delete type_data;
			}
			else { // array type
				DATA* type_data = evaluate_type((wchar_t*)result.args[3]);
				DataType::Array* object = new DataType::Array{ type_data, (USHORT*)result.args[4] };
				data = new DATA{ 6, object, true };
			}
			if (define_record) {
				record_fields.insert(((wchar_t**)result.args[2])[variable_index], data);
			}
			else {
				scope.insert(((wchar_t**)result.args[2])[variable_index], data);
			}
		}
	}
	void assignment(RESULT result) {
		bool is_constant = false;
		DATA* assignment_point = evaluate_variable((wchar_t*)result.args[0], &is_constant);
		if (is_constant) {
			throw Error(VariableError, L"赋值给常量是非法的");
		}
		else {
			DATA* eval_result = evaluate((RPN_EXP*)result.args[1]);
			if (not eval_result) {
				throw Error(EvaluationError, L"调用的子程序返回值为空，无法完成赋值操作");
			}
			DATA* type_data = DataType::get_type(assignment_point);
			DATA* adapted_result = DataType::type_adaptation(eval_result, type_data);
			DataType::release_data(eval_result);
			delete type_data;
			if (not adapted_result) {
				throw Error(VariableError, L"表达式结果与变量类型不符");
			}
			if (assignment_point->type == 10) { // pointer type
				BinaryTree::Node* target = (BinaryTree::Node*)((DataType::Address*)adapted_result->value)->value;
				DataType::Pointer* ptr = (DataType::Pointer*)assignment_point->value;
				((DataType::Pointer*)assignment_point->value)->reference(target);
				if (not ptr->init) {
					ptr->remove_link(assignment_point);
				}
				ptr->add_link(target, assignment_point);
			}
			else {
				memcpy(assignment_point, adapted_result, sizeof(DATA));
				assignment_point->variable_data = true;
				delete adapted_result;
			}
		}
	}
	void constant(RESULT result) {
		BinaryTree* scope = current_locals ? current_locals : globals;
		BinaryTree::Node* target_node = find_variable((wchar_t*)result.args[0]);
		if (target_node) {
			throw Error(VariableError, L"常量已存在");
		}
		DATA* eval_result = evaluate((RPN_EXP*)result.args[1]);
		eval_result->variable_data = true;
		scope->insert((wchar_t*)result.args[0], eval_result, true);
	}
	void type_header(RESULT result) {
		if (current_locals) {
			throw Error(SyntaxError, L"在局部域中定义结构体是非法的");
		}
		else if (define_record) {
			throw Error(SyntaxError, L"在结构体中定义结构体是非法的");
		}
		else {
			define_record = true;
			record_name = (wchar_t*)result.args[0];
		}
	}
	void type_ender(RESULT result) {
		UNREFERENCED_PARAMETER(result);
		if (define_record == false) {
			throw Error(SyntaxError, L"未找到ENDTYPE标签对应的TYPE头");
		}
		DATA* data = new DATA{ 7, new DataType::RecordType(&record_fields) };
		globals->insert(record_name, data);
		record_fields.clear();
		define_record = false;
		record_name = nullptr;
	}
	void pointer_type_header(RESULT result) {
		if (current_locals) {
			throw Error(SyntaxError, L"在局部域中定义指针类型是非法的");
		}
		else if (define_record) {
			throw Error(SyntaxError, L"在结构体中定义指针类型是非法的");
		}
		else if (globals->find((wchar_t*)result.args[0])) {
			throw Error(TypeError, L"同名类型已存在");
		}
		else if (Element::fundamental_type((wchar_t*)result.args[0])) {
			throw Error(TypeError, L"禁止使用内置类型作为新类型的名称");
		}
		else {
			wchar_t* type_expr = ((wchar_t*)result.args[1]) + 1; // skip pointer sign
			// fundamental data types
			static const wchar_t* fundamentals[] = { L"INTEGER", L"REAL", L"CHAR", L"STRING", L"boolEAN", L"DATE" };
			for (USHORT index = 0; index != 6; index++) {
				if (type_expr[0] == fundamentals[index][0] and wcslen(type_expr) == wcslen(fundamentals[index])) {
					bool matched = true;
					for (USHORT index2 = 1; type_expr[index2] != 0; index2++) {
						if (type_expr[index2] != fundamentals[index][index2]) {
							matched = false;
							break;
						}
					}
					if (matched) {
						DATA* type = new DATA{ index, nullptr };
						DATA* pointer_type = new DATA{ 9, new DataType::PointerType(type) };
						globals->insert((wchar_t*)result.args[0], pointer_type);
						return;
					}
				}
			}
			// other data types
			BinaryTree::Node* node = globals->find(type_expr);
			if (node) {
				DATA* pointer_type = new DATA{ 9, new DataType::PointerType(node->value) };
				globals->insert((wchar_t*)result.args[0], pointer_type);
			}
			else {
				throw Error(SyntaxError, L"未找到指定类型");
			}
		}
	}
	void enumerated_type_header(RESULT result){
		if (current_locals) {
			throw Error(SyntaxError, L"在局部域中定义枚举类型是非法的");
		}
		BinaryTree::Node* node = globals->find((wchar_t*)result.args[1]);
		if (node) {
			throw Error(SyntaxError, L"已有变量或类型与该枚举类型同名");
		}
		DataType::EnumeratedType* new_type = new DataType::EnumeratedType((wchar_t*)result.args[1]);
		DataType::EnumeratedType::add_constants(new_type, enumerations);
		DATA* data = new DATA{ 11, new_type };
		globals->insert((wchar_t*)result.args[0], data);
	}
	void output(RESULT result) {
		DATA* data = evaluate((RPN_EXP*)result.args[0]);
		if (not data) {
			throw Error(EvaluationError, L"调用的子程序返回值为空，无法完成输出操作");
		}
		DWORD count = 0;
		wchar_t* message = settings & OUTPUT_AS_OBJECT ? DataType::output_data_as_object(data, &count) : DataType::output_data(data, &count);
		WRITE_CONSOLE(standard_output, message, count);
		static const wchar_t new_line = L'\n';
		if (settings & AUTOMATIC_NEW_LINE_AFTER_OUTPUT) {
			WRITE_CONSOLE(standard_output, &new_line, 1);
		}
		delete[] message;
		DataType::release_data(data);
	}
	void input(RESULT result) {
		bool is_constant = false;
		DATA* variable_data = evaluate_variable((wchar_t*)result.args[0], &is_constant);
		if (is_constant) {
			throw Error(VariableError, L"无法输入给常量");
		}
		if (variable_data->type != 3) {
			throw Error(VariableError, L"接受输入的变量的数据类型必须为字符串");
		}
		DWORD size = 0;
		WaitForSingleObject(standard_input, INFINITE);
		while (size == 0) {
			PeekNamedPipe(standard_input, nullptr, NULL, nullptr, &size, nullptr);
		}
		char* buffer = new char[size];
		ReadFile(standard_input, buffer, size, nullptr, nullptr);
		int wchar_size = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, size, nullptr, NULL);
		wchar_t* wchar_buffer = new wchar_t[wchar_size + 1];
		MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, size, wchar_buffer, wchar_size);
		delete[] buffer;
		wchar_buffer[wchar_size] = 0;
		DATA* data = new DATA{ 3, new DataType::String(wchar_size, wchar_buffer) };
		memcpy(variable_data, data, sizeof(DATA));
		variable_data->variable_data = true;
		delete[] wchar_buffer;
	}
	void if_header_1(RESULT result) {
		DATA* data = evaluate((RPN_EXP*)result.args[1]);
		if (data->type != 4) {
			DataType::release_data(data);
			throw Error(EvaluationError, L"表达式的结果必须为布尔值");
		}
		if (((DataType::Boolean*)data->value)->value) { // THEN statements
			current_instruction_index = ((Nesting*)result.args[0])->line_numbers[1];
		}
		else { // ELSE statements (if not defined, then ENDIF tag)
			current_instruction_index = ((Nesting*)result.args[0])->line_numbers[2];
		}
		DataType::release_data(data);
	}
	void if_header_2(RESULT result) {
		DATA* data = evaluate((RPN_EXP*)result.args[1]);
		if (data->type != 4) {
			DataType::release_data(data);
			throw Error(EvaluationError, L"表达式的结果必须为布尔值");
		}
		if (not ((DataType::Boolean*)data->value)->value) { // ELSE statements (if not defined, then ENDIF tag)
			current_instruction_index = ((Nesting*)result.args[0])->line_numbers[2];
		}
		DataType::release_data(data);
	}
	void then_tag(RESULT result) {
		UNREFERENCED_PARAMETER(result);
		return; // nothing to do
	}
	void else_tag(RESULT result) {
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[3];
	}
	void if_ender(RESULT result) {
		return;
	}
	void case_of_header(RESULT result) {
		DATA* data = evaluate((RPN_EXP*)result.args[1]);
		for (USHORT tag_index = 0; tag_index != ((Nesting*)result.args[0])->nest_info->case_of_info.number_of_values; tag_index++) {
			DATA* match_data = evaluate((RPN_EXP*)((Nesting*)result.args[0])->nest_info->case_of_info.values[tag_index]);
			bool match_result = DataType::check_identical(data, match_data);
			DataType::release_data(match_data);
			if (match_result) {
				DataType::release_data(data);
				current_instruction_index = ((Nesting*)result.args[0])->line_numbers[tag_index + 1];
				return;
			}
		}
		DataType::release_data(data);
		if (((Nesting*)result.args[0])->nest_info->case_of_info.number_of_values + 3 == ((Nesting*)result.args[0])->tag_number) { // OTHERWISE
			current_instruction_index = ((Nesting*)result.args[0])->line_numbers[((Nesting*)result.args[0])->tag_number - 2];
		}
		else { // ENDCASE
			current_instruction_index = ((Nesting*)result.args[0])->line_numbers[((Nesting*)result.args[0])->tag_number - 1];
		}
	}
	void case_tag(RESULT result) {
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[((Nesting*)result.args[0])->tag_number - 1]; // ENDCASE
	}
	void otherwise_tag(RESULT result) {
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[((Nesting*)result.args[0])->tag_number - 1]; // ENDCASE
	}
	void case_of_ender(RESULT result) {
		return;
	}
	void for_header_1(RESULT result) {
		// only run in first loop as ender does not return to this statement
		BinaryTree::Node* node = find_variable((wchar_t*)result.args[1]);
		DATA* lower_bound = evaluate((RPN_EXP*)result.args[2]);
		if (lower_bound->type != 0) {
			throw Error(VariableError, L"循环变量的初始值必须为整数类型");
		}
		DATA* upper_bound = evaluate((RPN_EXP*)result.args[3]);
		if (upper_bound->type != 0) {
			throw Error(VariableError, L"循环变量的结束值必须为整数类型");
		}
		((Nesting*)result.args[0])->nest_info->for_info.upper_bound = upper_bound;
		if (not node) {
			if (Element::variable((wchar_t*)result.args[1])) {
				lower_bound->variable_data = true;
				if (current_locals) {
					current_locals->insert((wchar_t*)result.args[1], lower_bound);
				}
				else {
					globals->insert((wchar_t*)result.args[1], lower_bound);
				}
			}
			else {
				DataType::release_data(lower_bound);
				throw Error(VariableError, L"无法初始化变量路径");
			}
		}
		else {
			if (node->value->type != 0) {
				DataType::release_data(lower_bound);
				throw Error(VariableError, L"循环变量类型必须为整数");
			}
			else {
				memcpy(node->value, lower_bound, sizeof(DATA));
				node->value->variable_data = true;
			}
		}
		((Nesting*)result.args[0])->nest_info->for_info.init = true;
	}
	void for_header_2(RESULT result) {
		for_header_1(result);
	}
	void for_ender(RESULT result) {
		BinaryTree::Node* node = find_variable((wchar_t*)result.args[1]);
		if (not node) {
			throw Error(VariableError, L"变量不存在");
		}
		DATA*& variable_data = node->value;
		long long step = 1;
		if (((Nesting*)result.args[0])->nest_info->for_info.step) {
			DATA* data = evaluate(((Nesting*)result.args[0])->nest_info->for_info.step);
			if (data->type == 0) { step = ((DataType::Integer*)data->value)->value; }
			else if (data->type == 1) { step = (long long)((DataType::Real*)data->value)->value; }
			else { throw Error(SyntaxError, L"步长必须为整数"); }
		}
		if (node->value->type == 0) {
			((DataType::Integer*)variable_data->value)->value += step;
		}
		else if (node->value->type == 1) {
			((DataType::Real*)variable_data->value)->value += step;
		}
		else {
			throw Error(VariableError, L"非数值变量不能自增");
		}
		if (((DataType::Integer*)((Nesting*)result.args[0])->nest_info->for_info.upper_bound->value)->value >= ((DataType::Integer*)node->value->value)->value) { // next loop
			current_instruction_index = ((Nesting*)result.args[0])->line_numbers[0];
		}
	}
	void while_header(RESULT result) {
		DATA* data = evaluate((RPN_EXP*)result.args[1]);
		if (data->type != 4) {
			DataType::release_data(data);
			throw Error(EvaluationError, L"表达式的结果必须为布尔值");
		}
		if (not ((DataType::Boolean*)data->value)->value) { // ENDWHILE
			current_instruction_index = ((Nesting*)result.args[0])->line_numbers[1];
		}
		DataType::release_data(data);
	}
	void while_ender(RESULT result) {
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[0] - 1;
	}
	void repeat_header(RESULT result) {
		UNREFERENCED_PARAMETER(result);
		return;
	}
	void repeat_ender(RESULT result) {
		DATA* data = evaluate((RPN_EXP*)result.args[1]);
		if (data->type != 4) {
			DataType::release_data(data);
			throw Error(EvaluationError, L"表达式的结果必须为布尔值");
		}
		if (not ((DataType::Boolean*)data->value)->value) {
			current_instruction_index = ((Nesting*)result.args[0])->line_numbers[0] - 1;
		}
		DataType::release_data(data);
	}
	void procedure_header(RESULT result) {
		BinaryTree::Node* node = find_variable((wchar_t*)result.args[1]);
		if (node) {
			throw Error(VariableError, L"过程名已被占用");
		}
		USHORT& number_of_args = (*(USHORT*)result.args[2]);
		PARAMETER*& params = (PARAMETER*&)result.args[3];
		for (USHORT index = 0; index != number_of_args; index++) {
			params[index].type = evaluate_type(params[index].type_string);
		}
		DATA* data = new DATA{ 13, new DataType::Function(current_instruction_index, number_of_args, params) };
		globals->insert((wchar_t*)result.args[1], data);
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[1];
	}
	void procedure_ender(RESULT result) {
		UNREFERENCED_PARAMETER(result);
		return_value = new DATA{ 65535, nullptr };
	}
	void function_header(RESULT result) {
		BinaryTree::Node* node = find_variable((wchar_t*)result.args[1]);
		if (node) {
			throw Error(VariableError, L"函数名已被占用");
		}
		USHORT& number_of_args = (*(USHORT*)result.args[2]);
		PARAMETER*& params = (PARAMETER*&)result.args[3];
		DATA* return_type = evaluate_type((wchar_t*)result.args[4]);
		for (USHORT index = 0; index != number_of_args; index++) {
			USHORT bulitin_type = 0;
			if (Element::fundamental_type(params[index].type_string, &bulitin_type)) {
				params[index].type = new DATA{ bulitin_type, nullptr };
			}
			else {
				BinaryTree::Node* node2 = find_variable((wchar_t*)result.args[1]);
				if (not node2) {
					throw Error(TypeError, L"未找到类型定义");
				}
			}
		}
		DATA* data = new DATA{ 13, new DataType::Function(current_instruction_index, number_of_args, params, return_type) };
		globals->insert((wchar_t*)result.args[1], data);
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[1];
	}
	void function_ender(RESULT result) {
		procedure_ender(result);
	}
	void continue_tag(RESULT result) {
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[1] - 1;
	}
	void break_tag(RESULT result) {
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[1];
	}
	void return_statement(RESULT result) {
		if (*(bool*)result.args[1]) {
			DATA* data = evaluate((RPN_EXP*)result.args[2]);
			if (data->type == 7 or data->type == 9 or data->type == 11) {
				throw Error(ValueError, L"类型不能作为函数返回值");
			}
			if (data->variable_data) {
				return_value = DataType::copy(data);
			}
			else {
				return_value = data;
			}
		}
		else {
			return_value = new DATA{ 65535, nullptr };
		}
	}
	void try_header(RESULT result) {
		// if EXCEPT tag is defined then when error is detected current_instruction_index is changed to that line
		// otherwise ENDTRY
		BinaryTree*& scope = current_locals ? current_locals : globals;
		scope->error_handling_indexes[scope->error_handling_ptr] = ((Nesting*)result.args[0])->line_numbers[1];
		scope->error_handling_ptr++;
	}
	void except_tag(RESULT result) {
		BinaryTree*& scope = current_locals ? current_locals : globals;
		scope->error_handling_ptr--;
		current_instruction_index = ((Nesting*)result.args[0])->line_numbers[2];
	}
	void try_ender(RESULT result) {
		return;
	}
	void openfile_statement(RESULT result) {
		DATA* filename_data = evaluate((RPN_EXP*)result.args[0]);
		if (filename_data->type != 3) {
			DataType::release_data(filename_data);
			throw Error(ValueError, L"文件名必须为字符串");
		}
		else {
			if (file_ptr == 8) {
				DataType::release_data(filename_data);
				throw Error(ValueError, L"至多同时打开8个文件");
			}
			USHORT mode = *(USHORT*)result.args[1];
			wchar_t* filename = new wchar_t[wcslen(((DataType::String*)filename_data->value)->string) + 1];
			memcpy(filename, ((DataType::String*)filename_data->value)->string, (wcslen(((DataType::String*)filename_data->value)->string) + 1) * 2);
			DataType::release_data(filename_data);
			HANDLE file_handle = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (not file_handle) {
				delete[] filename;
				if (GetLastError() == ERROR_FILE_NOT_FOUND) {
					throw Error(ValueError, L"访问的文件不存在");
				}
				else {
					throw Error(ValueError, L"无法访问文件");
				}
			}
			if (mode == 2) {
				SetFilePointer(file_handle, 0, 0, FILE_END);
			}
			files[file_ptr].handle = file_handle;
			files[file_ptr].mode = mode;
			files[file_ptr].filename = filename;
			file_ptr++;
		}
	}
	void readfile_statement(RESULT result) {
		DATA* filename_data = evaluate((RPN_EXP*)result.args[0]);
		if (filename_data->type != 3) {
			DataType::release_data(filename_data);
			throw Error(ValueError, L"文件名必须为字符串");
		}
		else {
			wchar_t*& filename = ((DataType::String*)filename_data->value)->string;
			for (USHORT file_index = 0; file_index != file_ptr; file_index++) {
				if (wcslen(filename) == wcslen(files[file_index].filename)) {
					bool matched = true;
					for (size_t index = 0; filename[index] != 0; index++) {
						if (filename[index] != files[file_index].filename[index]) {
							matched = false;
							break;
						}
					}
					if (matched) {
						wchar_t* line = files[file_index].readline(not (settings & DISCARD_CRLF_IN_READ));
						if (not line) {
							throw Error(ValueError, L"无法读取");
						}
						bool is_constant = false;
						DATA* assignment_point = evaluate_variable((wchar_t*)result.args[1], &is_constant);
						if (is_constant) {
							delete[] line;
							throw Error(VariableError, L"常量无法用于接收字符串数据");
						}
						else {
							if (assignment_point->type != 3) {
								delete[] line;
								throw Error(VariableError, L"接收变量类型必须为字符串");
							}
							DATA data{ 3, new DataType::String(wcslen(line), line) };
							memcpy(assignment_point, &data, sizeof(DATA));
							assignment_point->variable_data = true;
						}
						DataType::release_data(filename_data);
						return;
					}
				}
			}
			DataType::release_data(filename_data);
			throw Error(ValueError, L"文件未打开");
		}
	}
	void writefile_statement(RESULT result) {
		DATA* filename_data = evaluate((RPN_EXP*)result.args[0]);
		DATA* message_data = evaluate((RPN_EXP*)result.args[1]);
		if (filename_data->type != 3) {
			throw Error(ValueError, L"文件名必须为字符串");
		}
		if (message_data->type != 3) {
			throw Error(ValueError, L"写入的数据必须为字符串");
		}
		wchar_t*& string = ((DataType::String*)filename_data->value)->string;
		for (USHORT file_index = 0; file_index != file_ptr; file_index++) {
			if (wcslen(string) == wcslen(files[file_index].filename)) {
				bool matched = true;
				for (size_t index = 0; string[index] != 0; index++) {
					if (string[index] != files[file_index].filename[index]) {
						matched = false;
						break;
					}
				}
				if (matched) {
					bool write_result = files[file_index].writeline(((DataType::String*)message_data->value)->string,
						settings & AUTOMATIC_NEW_LINE_IN_WRITE, settings & FLUSH_FILE_BUFFERS_AFTER_WRITE);
					DataType::release_data(filename_data);
					DataType::release_data(message_data);
					if (not write_result) {
						throw Error(ValueError, L"写入失败");
					}
					return;
				}
			}
		}
		DataType::release_data(filename_data);
		DataType::release_data(message_data);
		throw Error(ValueError, L"文件未打开");
	}
	void closefile_statement(RESULT result) {
		DATA* filename_data = evaluate((RPN_EXP*)result.args[0]);
		if (filename_data->type != 3) {
			DataType::release_data(filename_data);
			throw Error(ValueError, L"文件名必须为字符串");
		}
		else {
			wchar_t*& string = ((DataType::String*)filename_data->value)->string;
			for (USHORT file_index = 0; file_index != file_ptr; file_index++) {
				if (wcslen(string) == wcslen(files[file_index].filename)) {
					bool matched = true;
					for (size_t index = 0; string[index] != 0; index++) {
						if (string[index] != files[file_index].filename[index]) {
							matched = false;
							break;
						}
					}
					if (matched) {
						DataType::release_data(filename_data);
						files[file_index].closefile();
						memcpy(files + file_index, files + file_index + 1, sizeof(File) * (size_t)(7 - file_index));
						file_ptr--;
						return;
					}
				}
			}
		}
		DataType::release_data(filename_data);
		throw Error(ValueError, L"文件未打开");
	}
	void seek_statement(RESULT result) {
		DATA* filename_data = evaluate((RPN_EXP*)result.args[0]);
		DATA* address_data = evaluate((RPN_EXP*)result.args[1]);
		if (filename_data->type != 3) {
			throw Error(ValueError, L"文件名必须为字符串");
		}
		size_t address = 0;
		if (address_data->type == 0) {
			address = ((DataType::Integer*)address_data->value)->value;
		}
		else if (address_data->type == 1) {
			address = (size_t)((DataType::Real*)address_data->value)->value;
		}
		else {
			throw Error(ValueError, L"地址必须为数字类型");
		}
		wchar_t*& string = ((DataType::String*)filename_data->value)->string;
		for (USHORT file_index = 0; file_index != file_ptr; file_index++) {
			if (wcslen(string) == wcslen(files[file_index].filename)) {
				bool matched = true;
				for (size_t index = 0; string[index] != 0; index++) {
					if (string[index] != files[file_index].filename[index]) {
						matched = false;
						break;
					}
				}
				if (matched) {
					files[file_index].seek(address);
					DataType::release_data(filename_data);
					DataType::release_data(address_data);
					return;
				}
			}
		}
		DataType::release_data(filename_data);
		DataType::release_data(address_data);
		throw Error(ValueError, L"文件未打开");
	}
	void getrecord_statement(RESULT result) {
		DATA* filename_data = evaluate((RPN_EXP*)result.args[0]);
		if (filename_data->type != 3) {
			DataType::release_data(filename_data);
			throw Error(ValueError, L"文件名必须为字符串");
		}
		else {
			wchar_t*& filename = ((DataType::String*)filename_data->value)->string;
			for (USHORT file_index = 0; file_index != file_ptr; file_index++) {
				if (wcslen(filename) == wcslen(files[file_index].filename)) {
					bool matched = true;
					for (size_t index = 0; filename[index] != 0; index++) {
						if (filename[index] != files[file_index].filename[index]) {
							matched = false;
							break;
						}
					}
					if (matched) {
						char* sequence = files[file_index].getrecord();
						if (not sequence) {
							throw Error(ValueError, L"反序列化错误");
						}
						DATA* data = DataType::desequentialise(sequence);
						delete[] sequence;
						bool is_constant = false;
						DATA* assignment_point = evaluate_variable((wchar_t*)result.args[1], &is_constant);
						if (is_constant) {
							DataType::release_data(data);
							throw Error(VariableError, L"常量无法用于接收反序列化数据");
						}
						else {
							if (assignment_point->type != data->type) {
								DataType::release_data(data);
								throw Error(VariableError, L"接收变量类型与反序列化的类型不符");
							}
							memcpy(assignment_point, data, sizeof(DATA));
							assignment_point->variable_data = true;
						}
						DataType::release_data(filename_data);
						return;
					}
				}
			}
			DataType::release_data(filename_data);
			throw Error(ValueError, L"文件未打开");
		}
	}
	void putrecord_statement(RESULT result) {
		DATA* filename_data = evaluate((RPN_EXP*)result.args[0]);
		DATA* record_data = evaluate((RPN_EXP*)result.args[1]);
		if (filename_data->type != 3) {
			throw Error(ValueError, L"文件名必须为字符串");
		}
		wchar_t*& string = ((DataType::String*)filename_data->value)->string;
		for (USHORT file_index = 0; file_index != file_ptr; file_index++) {
			if (wcslen(string) == wcslen(files[file_index].filename)) {
				bool matched = true;
				for (size_t index = 0; string[index] != 0; index++) {
					if (string[index] != files[file_index].filename[index]) {
						matched = false;
						break;
					}
				}
				if (matched) {
					char* digest = DataType::sequentialise(record_data);
					DataType::release_data(filename_data);
					DataType::release_data(record_data);
					if (not digest) {
						throw Error(ValueError, L"该数据不可被序列化");
					}
					else {
						files[file_index].putrecord(digest);
					}
					return;
				}
			}
		}
		DataType::release_data(filename_data);
		DataType::release_data(record_data);
		throw Error(ValueError, L"文件未打开");
	}
	void single_expression(RESULT result) {
		DATA* data = evaluate((RPN_EXP*)result.args[0]);
		if (data) {
			if (not data->variable_data) {
				DataType::release_data(data);
			}
		}
	}
	void* executions[] = {
		empty_line, declaration, constant, type_header, type_ender, pointer_type_header, enumerated_type_header,
		assignment, output, input, if_header_1, if_header_2, then_tag, else_tag, if_ender, case_of_header,
		case_tag, otherwise_tag, case_of_ender, for_header_1, for_header_2, for_ender, while_header, while_ender,
		repeat_header, repeat_ender, procedure_header, procedure_ender, function_header, function_ender,
		continue_tag, break_tag, return_statement, try_header, except_tag, try_ender, openfile_statement,
		readfile_statement, writefile_statement, closefile_statement, seek_statement, getrecord_statement,
		putrecord_statement, single_expression };
	const USHORT instructions = sizeof(executions) / sizeof(void*);
}

DATA* addressing(wchar_t* expr) {
	BinaryTree::Node* node = find_variable(expr + 1);
	if (node) {
		DATA* data = new DATA{ 14, new DataType::Address(node) };
		return data;
	}
	else {
		throw Error(EvaluationError, L"求值的变量不存在");
	}
}

DATA* evaluate_single_element(wchar_t* expr) {
	if (Element::addressing(expr)) {
		return addressing(expr);
	}
	else if (Element::character(expr)) {
		return new DATA{ 2, new DataType::Char(expr) };
	}
	else if (Element::string(expr)) {
		return new DATA{ 3, new DataType::String(expr) };
	}
	else if (Element::variable_path(expr)) {
		DATA* result_data = evaluate_variable(expr);
		return result_data;
	}
	else if (Element::integer(expr)) {
		return new DATA{ 0, new DataType::Integer(expr) };
	}
	else if (Element::real(expr)) {
		return new DATA{ 1, new DataType::Real(expr) };
	}
	else if (expr[0] == 7) {
		return new DATA{ 4, new DataType::Boolean(false) };
	}
	else if (expr[0] == 8) {
		return new DATA{ 4, new DataType::Boolean(true) };
	}
	else if (Element::date(expr)) {
		return new DATA{ 5, new DataType::Date(expr) };
	}
	return nullptr;
}

DATA* array_access(DATA* array_data, wchar_t* expr) {
	USHORT last_comma = 0;
	USHORT brackets = 1;
	USHORT* indexes = new USHORT[10];
	USHORT dimension = 0;
	for (USHORT index = 1; expr[index] != 0; index++) {
		if (expr[index] == 91) {
			brackets++;
		}
		if (expr[index] == 44 or expr[index] == 93) {
			if (expr[index] == 93 and brackets != 1) {
				brackets--;
				continue;
			}
			wchar_t* new_index = new wchar_t[index - last_comma];
			memcpy(new_index, expr + last_comma + 1, ((size_t)index - last_comma - 1) * 2);
			new_index[index - last_comma - 1] = 0;
			if (Element::integer(new_index)) {
				indexes[dimension] = (USHORT)string_to_real(new_index);
			}
			else {
				RPN_EXP* rpn_out = nullptr;
				Element::expression(new_index, &rpn_out);
				DATA* data = evaluate(rpn_out);
				if (data->type == 0) {
					indexes[dimension] = (USHORT)((DataType::Integer*)data->value)->value;
				}
				else if (data->type == 1) {
					indexes[dimension] = (USHORT)((DataType::Real*)data->value)->value; // force casting to integer
				}
				else {
					throw Error(EvaluationError, L"下标必须为数字类型");
				}
			}
			delete[] new_index;
			last_comma = index;
			if (expr[index] == 93 and brackets == 1) {
				DATA* result_data = ((DataType::Array*)array_data->value)->read(indexes);
				if (result_data->type == 65535) {
					throw Error(EvaluationError, (wchar_t*)result_data->value);
				}
				return result_data;
			}
		}
	}
	return nullptr;
}

DATA* evaluate_variable(wchar_t* path, bool* constant) { // starting with a variable name
	// all the DATA structure pointer returned by this function should not be released
	if (constant) { *constant = false; }
	if (path[0] == L'^') { // dereference a pointer
		if (path[1] == L'(') { // in the form of ^(x.x).x
			USHORT nesting_level = 1;
			for (USHORT index = 0; path[index] != 0; index++) {
				if (path[index] == 40) { // nested brackets
					nesting_level++;
				}
				if (path[index] == 41) {
					if (nesting_level != 1) {
						nesting_level--;
						continue;
					}
					wchar_t* pointer_expr = new wchar_t[index - 1];
					memcpy(pointer_expr, path + 2, (size_t)(index - 2) * 2);
					pointer_expr[index - 2] = 0;
					DATA* ptr_data = evaluate_variable(pointer_expr);
					if (ptr_data->type != 10) {
						throw Error(EvaluationError, L"解引用操作符仅能用于指针变量");
					}
					else if (((DataType::Pointer*)ptr_data->value)->init) {
						throw Error(EvaluationError, L"指针未初始化");
					}
					else if (((DataType::Pointer*)ptr_data->value)->valid) {
						throw Error(EvaluationError, L"指针指向了已销毁的变量");
					}
					BinaryTree::Node* node = ((DataType::Pointer*)ptr_data->value)->dereference();
					delete ptr_data;
					return node->value;
				}
			}
		}
		else { // in the form of ^x.x
			for (USHORT index = 0; path[index] != 0; index++) {
				if (path[index] == 46) {
					wchar_t* pointer_expr = new wchar_t[index - 1];
					wchar_t* path_part = new wchar_t[wcslen(path) - index];
					memcpy(pointer_expr, path + 1, (size_t)(index - 2) * 2);
					memcpy(path_part, path + index, (wcslen(path) - index - 1) * 2);
					pointer_expr[index - 2] = 0;
					path_part[wcslen(path) - index - 1] = 0;
					BinaryTree& scope = current_locals ? *current_locals : *globals;
					DATA* pointer = scope.find(pointer_expr)->value;
					if (not pointer) {
						throw Error(EvaluationError, L"变量未定义");
					}
					DATA* dereferenced = ((DataType::Pointer*)pointer->value)->dereference()->value;
					delete pointer;
					return evaluate_variable(dereferenced, path_part);
				}
			}
			// in the form of ^x
			DATA* ptr_data = evaluate_variable(path + 1);
			if (ptr_data->type != 10) {
				throw Error(EvaluationError, L"解引用操作符仅能用于指针变量");
			}
			else if (not ((DataType::Pointer*)ptr_data->value)->init) {
				throw Error(EvaluationError, L"指针未初始化");
			}
			else if (not ((DataType::Pointer*)ptr_data->value)->valid) {
				throw Error(EvaluationError, L"指针指向了已销毁的变量");
			}
			BinaryTree::Node* node = ((DataType::Pointer*)ptr_data->value)->dereference();
			return node->value;
		}
	}
	for (USHORT index = 0; path[index] != 0; index++) {
		if (path[index] == 46) { 
			wchar_t* variable_part = new wchar_t[index + 1];
			memcpy(variable_part, path, (size_t)index * 2);
			variable_part[index] = 0;
			wchar_t* array_name = nullptr;
			wchar_t* indexes = nullptr;
			if (Element::array_element_access(variable_part, &array_name, &indexes)) { // x[x].x
				BinaryTree::Node* node = find_variable(array_name);
				delete[] array_name;
				if (not node) {
					delete[] indexes;
					throw Error(EvaluationError, L"变量未定义");
				}
				DATA* array_data = node->value;
				DATA* element_data = array_access(array_data, indexes);
				delete[] indexes;
				if (element_data->type == 65535) {
					throw Error(EvaluationError, (wchar_t*)element_data->value);
				}
				return evaluate_variable(element_data, path + index);
			}
			else { // x.x
				BinaryTree::Node* node = find_variable(variable_part);
				delete[] variable_part;
				if (not node) { 
					throw Error(EvaluationError, L"变量未定义");
				}
				DATA* variable_data = node->value;
				return evaluate_variable(variable_data, path + index);
			}
		}
	}
	wchar_t* array_name = nullptr;
	wchar_t* indexes = nullptr;
	if (Element::array_element_access(path, &array_name, &indexes)) { // x[x]
		BinaryTree::Node* node = find_variable(array_name);
		delete[] array_name;
		if (not node) {
			delete[] indexes;
			throw Error(EvaluationError, L"变量未定义");
		}
		DATA* array_data = node->value;
		DATA* element_data = array_access(array_data, indexes);
		delete[] indexes;
		if (element_data->type == 65535) {
			throw Error(EvaluationError, (wchar_t*)element_data->value);
		}
		return element_data;
	}
	// x (single variable)
	BinaryTree::Node* node = find_variable(path);
	if (node) {
		DATA* variable_data = node->value;
		if (constant) { *constant = node->constant; }
		return variable_data;
	}
	else {
		node = enumerations->find(path);
		if (node) {
			return node->value;
		}
		else {
			throw Error(EvaluationError, L"变量未定义");
		}
	}
}

DATA* evaluate_variable(DATA* current, wchar_t* path) { // based on previous evaluation and find more specific fields
	if (current->type != 8) {
		throw Error(EvaluationError, L"非结构体不能进行字段访问");
	}
	for (USHORT index = 1; path[index] != 0; index++) {
		if (path[index] == 46) {
			wchar_t* this_path = new wchar_t[index];
			memcpy(this_path, path + 1, (size_t)(index - 1) * 2);
			this_path[index - 1] = 0;
			if (Element::array_element_access(this_path)) {
				for (USHORT index2 = 0; this_path[index2] != 0; index2++) {
					if (this_path[index2] == 91) {
						this_path[index2] = 0;
						int offset = ((DataType::Record*)(current->value))->find(this_path);
						if (offset == -1) {
							throw Error(EvaluationError, L"结构体内不包含该字段");
						}
						DATA* current_point = ((DataType::Record*)(current->value))->read((USHORT)offset);
						this_path[index2] = 91;
						if (current_point->type != 6) {
							throw Error(EvaluationError, L"下标访问仅可用于数组");
						}
						current_point = array_access(current_point, this_path + index2);
						delete[] this_path;
						if (current_point->type == 65535) {
							return current_point;
						}
						DATA* result_point = evaluate_variable(current_point, path + index);
						return result_point;
					}
				}
			}
			else {
				int offset = ((DataType::Record*)(current->value))->find(this_path);
				delete[] this_path;
				if (offset == -1) {
					throw Error(EvaluationError, L"结构体内不包含该字段");
				}
				DATA* current_point = ((DataType::Record*)(current->value))->read((USHORT)offset);
				DATA* result_point = evaluate_variable(current_point, path + index);
				return result_point;
			}
		}
	}
	// last path
	if (Element::array_element_access(path)) {
		for (USHORT index2 = 0; path[index2] != 0; index2++) {
			if (path[index2] == 91) {
				path[index2] = 0;
				int offset = ((DataType::Record*)(current->value))->find(path);
				if (offset == -1) {
					throw Error(EvaluationError, L"结构体内不包含该字段");
				}
				DATA* last_point = ((DataType::Record*)(current->value))->read((USHORT)offset);
				path[index2] = 91;
				last_point = array_access(last_point, path + index2);
				return last_point;
			}
		}
		return nullptr;
	}
	else {
		int offset = ((DataType::Record*)(current->value))->find(path + 1);
		if (offset == -1) {
			throw Error(EvaluationError, L"结构体内不包含该字段");
		}
		DATA* last_point = ((DataType::Record*)(current->value))->read((USHORT)offset);
		return last_point;
	}
}

DATA* evaluate(RPN_EXP* rpn_in) {
	wchar_t*& expr = rpn_in->rpn;
	USHORT& number_of_element = rpn_in->number_of_element;
	size_t total_offset = 0;
	DATA** stack = new DATA* [128] {0};
	USHORT ptr = 0;
	for (USHORT element_index = 0; element_index != number_of_element; element_index++) {
		if (Element::operator_precedence(expr[total_offset]) and wcslen(expr + total_offset) == 1) { // operator
			wchar_t& operator_value = expr[total_offset];
			DATA* result = nullptr;
			switch (operator_value) {
			case 6: // one arity operator
			{
				ptr--;
				DATA* right_operand = stack[ptr];
				if (not ((DataType::Any*)right_operand->value)->init) {
					for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
					delete[] stack;
					throw Error(EvaluationError, L"右操作数尚未初始化");
				}
				if (operator_value == 6){
					if (right_operand->type == 4) {
						if (((DataType::Boolean*)right_operand->value)->value) {
							result = new DATA{ 4, new DataType::Boolean(false) };
						}
						else {
							result = new DATA{ 4, new DataType::Boolean(true) };
						}
					}
					else if (right_operand->type == 65535) {
						for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
						delete[] stack;
						throw Error(EvaluationError, L"不具有返回值的子程序不能用于运算");
					}
					else if (right_operand->type == 7 or right_operand->type == 9 or right_operand->type == 11) {
						for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
						delete[] stack;
						throw Error(EvaluationError, L"自定义类型不能用于运算");
					}
					else {
						for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
						delete[] stack;
						throw Error(EvaluationError, L"NOT操作符的操作数必须是布尔值");
					}
				}
				if (not right_operand->variable_data) {
					DataType::release_data(right_operand);
				}
				stack[ptr] = result;
				ptr++;
				break;
			}
			default: // two arity operator
			{
				ptr--;
				DATA* right_operand = stack[ptr];
				if (not ((DataType::Any*)right_operand->value)->init) {
					for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
					delete[] stack;
					throw Error(EvaluationError, L"右操作数尚未初始化");
				}
				ptr--;
				DATA* left_operand = stack[ptr];
				if (not ((DataType::Any*)left_operand->value)->init) {
					for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
					delete[] stack;
					throw Error(EvaluationError, L"左操作数尚未初始化");
				}
				// type conversion
				switch (left_operand->type) {
				case 0:
				{
					DATA* tag_data = new DATA{ 1, new DataType::Real((DataType::Integer*)left_operand->value) };
					if (not left_operand->variable_data) { DataType::release_data(left_operand); }
					left_operand = tag_data;
					break;
				}
				case 2:
				{
					DATA* tag_data = new DATA{ 3, new DataType::String((DataType::Char*)left_operand->value) };
					if (not left_operand->variable_data) { DataType::release_data(left_operand); }
					left_operand = tag_data;
					break;
				}
				case 10:
				{
					DATA* tag_data = new DATA{ 1, new DataType::Real((long double)((DataType::Enumerated*)left_operand->value)->current) };
					if (not left_operand->variable_data) { DataType::release_data(left_operand); }
					left_operand = tag_data;
					break;
				}
				case 65535:
					for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
					delete[] stack;
					throw Error(EvaluationError, L"不具有返回值的子程序不能用于运算");
				case 7: case 9: case 11:
					for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
					delete[] stack;
					throw Error(EvaluationError, L"自定义类型不能用于运算");
				default:
					if (left_operand->variable_data) {
						left_operand = DataType::copy(left_operand);
					}
					break;
				}
				// type conversion
				switch (right_operand->type) {
				case 0:
				{
					DATA* tag_data = new DATA{ 1, new DataType::Real((DataType::Integer*)right_operand->value) };
					if (not right_operand->variable_data) { DataType::release_data(right_operand); }
					right_operand = tag_data;
					break;
				}
				case 2:
				{
					DATA* tag_data = new DATA{ 3, new DataType::String((DataType::Char*)right_operand->value) };
					if (not right_operand->variable_data) { DataType::release_data(right_operand); }
					right_operand = tag_data;
					break;
				}
				case 10:
				{
					DATA* tag_data = new DATA{ 1, new DataType::Real((long double)((DataType::Enumerated*)right_operand->value)->current) };
					if (not right_operand->variable_data) { DataType::release_data(right_operand); }
					right_operand = tag_data;
					break;
				}
				case 65535:
					for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
					delete[] stack;
					throw Error(EvaluationError, L"不具有返回值的子程序不能用于运算");
				case 7: case 9: case 11:
					for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
					delete[] stack;
					throw Error(EvaluationError, L"自定义类型不能用于运算");
				default:
					if (right_operand->variable_data) {
						right_operand = DataType::copy(right_operand);
					}
					break;
				}
				if (operator_value == 61) {
					result = new DATA{ 4, new DataType::Boolean(DataType::check_identical(left_operand, right_operand)) };
				}
				else {
					switch ((left_operand->type << 8) + right_operand->type) {
					case 257: // numeric value
					{
						long double left_number = ((DataType::Real*)left_operand->value)->value;
						long double right_number = ((DataType::Real*)right_operand->value)->value;
						switch (operator_value) {
						case 43:
							result = new DATA{ 1, new DataType::Real(left_number + right_number) };
							break;
						case 45:
							result = new DATA{ 1, new DataType::Real(left_number - right_number) };
							break;
						case 42:
							result = new DATA{ 1, new DataType::Real(left_number * right_number) };
							break;
						case 47:
							if (right_number == 0) {
								for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
								delete[] stack;
								throw Error(EvaluationError, L"被除数不能为0");
							}
							result = new DATA{ 1, new DataType::Real(left_number / right_number) };
							break;
						case 60:
							result = new DATA{ 4, new DataType::Boolean(left_number < right_number) };
							break;
						case 62:
							result = new DATA{ 4, new DataType::Boolean(left_number > right_number) };
							break;
						case 1:
							result = new DATA{ 4, new DataType::Boolean(left_number <= right_number) };
							break;
						case 2:
							result = new DATA{ 4, new DataType::Boolean(left_number != right_number) };
							break;
						case 3:
							result = new DATA{ 4, new DataType::Boolean(left_number >= right_number) };
							break;
						default:
							for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
							delete[] stack;
							throw Error(EvaluationError, L"该运算符不能用于数值数据");
						}
						break;
					}
					case 771: // strings
					{
						switch (operator_value) {
						case 38:
						{
							DataType::String* string = ((DataType::String*)left_operand->value)->combine((DataType::String*)right_operand->value);
							result = new DATA{ 3, string };
							break;
						}
						case 1:
						{
							bool identical_or_smaller = DataType::check_identical(left_operand, right_operand) or
								((DataType::String*)left_operand->value)->smaller((DataType::String*)right_operand->value);
							result = new DATA{ 4, new DataType::Boolean(identical_or_smaller) };
							break;
						}
						case 2:
						{
							bool identical = DataType::check_identical(left_operand, right_operand);
							result = new DATA{ 4, new DataType::Boolean(not identical) };
							break;
						}
						case 3:
						{
							bool identical_or_larger = DataType::check_identical(left_operand, right_operand) or
								((DataType::String*)left_operand->value)->larger((DataType::String*)right_operand->value);
							result = new DATA{ 4, new DataType::Boolean(identical_or_larger) };
							break;
						}
						case 60:
						{
							bool smaller = ((DataType::String*)left_operand->value)->smaller((DataType::String*)right_operand->value);
							result = new DATA{ 4, new DataType::Boolean(smaller) };
							break;
						}
						case 62:
						{
							bool larger = ((DataType::String*)left_operand->value)->larger((DataType::String*)right_operand->value);
							result = new DATA{ 4, new DataType::Boolean(larger) };
							break;
						}
						default:
							for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
							delete[] stack;
							throw Error(EvaluationError, L"该运算符不能用于字符串数据");
							break;
						}
						break;
					}
					case 1028: // boolean values
					{
						bool left_bool = ((DataType::Boolean*)left_operand->value)->value;
						bool right_bool = ((DataType::Boolean*)right_operand->value)->value;
						switch (operator_value) {
						case 4:
							result = left_bool and right_bool ? new DATA{ 4, new DataType::Boolean(true) } : new DATA{ 4, new DataType::Boolean(false) };
							break;
						case 5:
							result = left_bool or right_bool ? new DATA{ 4, new DataType::Boolean(true) } : new DATA{ 4, new DataType::Boolean(false) };
							break;
						default:
							for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
							delete[] stack;
							throw Error(EvaluationError, L"该运算符不能用于布尔值数据");
						}
						break;
					}
					case 1285: // dates
					{
						if (operator_value == 43) {
							DataType::Date* result_date = ((DataType::Date*)left_operand->value)->add((DataType::Date*)right_operand->value);
							result = new DATA{ 5, result_date };
						}
						else {
							for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
							delete[] stack;
							throw Error(EvaluationError, L"该运算符不能用于日期与时间数据");
						}
						break;
					}
					default:
						for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
						delete[] stack;
						throw Error(EvaluationError, L"运算不合法");
					}
				}
				if (not left_operand->variable_data) {
					DataType::release_data(left_operand);
				}
				if (not right_operand->variable_data) {
					DataType::release_data(right_operand);
				}
				stack[ptr] = result;
				ptr++;
				break;
			}
			}
			total_offset += 2;
		}
		else {
			bool is_function = false;
			for (USHORT index = 0; (expr + total_offset)[index] != 0; index++) {
				if ((expr + total_offset)[index] == 9) { // function calling
					wchar_t* function_name = new wchar_t[index + 1];
					wchar_t* number_of_args_string = new wchar_t[wcslen(expr + total_offset) - index];
					memcpy(function_name, expr + total_offset, (size_t)index * 2);
					memcpy(number_of_args_string, expr + total_offset + index + 1, (wcslen(expr + total_offset) - index - 1) * 2);
					function_name[index] = 0;
					number_of_args_string[wcslen(expr + total_offset) - index - 1] = 0;
					USHORT number_of_args = (USHORT)string_to_real(number_of_args_string);
					delete[] number_of_args_string;
					DATA** data_ptr = new DATA* [number_of_args];
					for (USHORT data_index = 0; data_index != number_of_args; data_index++) {
						ptr--;
						data_ptr[number_of_args - data_index - 1] = stack[ptr];
					}
					DATA* data = function_calling(function_name, number_of_args, data_ptr);
					delete[] function_name;
					stack[ptr] = data;
					ptr++;
					total_offset += wcslen(expr + total_offset) + 1;
					is_function = true;
					break;
				}
			}
			if (is_function) { continue; }
			// operand
			if (ptr == 128) {
				for (USHORT index = 0; index != ptr; index++) { DataType::release_data(stack[index]); }
				delete[] stack;
				throw Error(EvaluationError, L"求值栈已满（请简化表达式）");
			}
			DATA* operand = evaluate_single_element(expr + total_offset);
			stack[ptr] = operand;
			ptr++;
			total_offset += wcslen(expr + total_offset) + 1;
		}
	}
	DATA* result_data = stack[0];
	delete[] stack;
	if (result_data->type == 65535) {
		return nullptr;
	}
	else if (result_data->variable_data) {
		return DataType::copy(result_data);
	}
	else {
		return result_data;
	}
}

DATA* function_calling(wchar_t* function_name, USHORT number_of_args, DATA** args) {
	// user-defined function calling
	BinaryTree::Node* node = find_variable(function_name);
	if (node) {
		if (node->value->type == 13) {
			if (callstack.ptr == 128) {
				throw Error(SyntaxError, L"递归深度达到上限");
			}
			if (((DataType::Function*)node->value->value)->number_of_args != number_of_args) {
				throw Error(ArgumentError, L"参数个数不匹配");
			}
			current_locals = new BinaryTree;
			callstack.stack[callstack.ptr] = CALLSTACK::FRAME{ current_instruction_index, function_name, wcslen(function_name) + 1, current_locals };
			if (not ((DataType::Function*)node->value->value)->push_args(current_locals, args)) {
				throw Error(ArgumentError, L"参数类型不匹配");
			}
			callstack.ptr++;
			for (current_instruction_index = ((DataType::Function*)node->value->value)->start_line + 1;; current_instruction_index++) {
				callstack.GetCurrentCall().line_number = current_instruction_index;
				if (debug) {
					if (CheckBreakpoint(current_instruction_index)) {
						ResetEvent(breakpoint_handled);
						SendSignal(SIGNAL_BREAKPOINT, BREAKPOINT_HIT, current_instruction_index);
						WaitForSingleObject(breakpoint_handled, INFINITE);
					}
					else if ((step_type == EXECUTION_STEPIN or step_type == EXECUTION_STEPOVER) and step_depth >= callstack.ptr) {
						ResetEvent(step_handled);
						SendSignal(SIGNAL_EXECUTION, EXECUTION_STEPPED, current_instruction_index);
						WaitForSingleObject(step_handled, INFINITE);
					}
				}
				try {
					((FUNCTION_PTR)Execution::executions[parsed_code[current_instruction_index]->syntax_index])(parsed_code[current_instruction_index]->result);
				}
				catch (Error& error){
					if (current_locals->error_handling_ptr) {
						current_locals->error_handling_ptr--;
						current_instruction_index = current_locals->error_handling_indexes[current_locals->error_handling_ptr];
					}
					else {
						throw error;
					}
				}
				if (return_value) {
					// invalidate pointers
					USHORT number_of_nodes = 0;
					BinaryTree::Node* node_list = current_locals->list_nodes(current_locals->root, &number_of_nodes);
					for (USHORT index = 0; index != number_of_nodes; index++) {
						DataType::Pointer::invalidate(node_list + index);
					}
					// update variables passed by reference
					for (USHORT index = 0; index != number_of_args; index++) {
						if (((DataType::Function*)node->value->value)->params[index].passed_by_ref and args[index]->variable_data) {
							BinaryTree::Node* local_variable = current_locals->find(((DataType::Function*)node->value->value)->params[index].name);
							memcpy(args[index], local_variable->value, sizeof(DATA));
							args[index]->variable_data = true;
						}
					}
					current_locals->clear();
					// restore running information
					callstack.ptr--;
					current_instruction_index = callstack.GetCurrentCall().line_number;
					current_locals = callstack.GetCurrentCall().local_variables;
					// return value
					if (step_type == EXECUTION_STEPOUT and step_depth == callstack.ptr) {
						ResetEvent(step_handled);
						SendSignal(SIGNAL_EXECUTION, EXECUTION_STEPPED, current_instruction_index);
						WaitForSingleObject(step_handled, INFINITE);
					}
					if (return_value->type == 65535) {
						if (((DataType::Function*)node->value->value)->return_type->type != 65535) {
							throw Error(ValueError, L"必须提供返回值");
						}
						else {
							DATA* temp = return_value;
							return_value = nullptr;
							return temp;
						}
					}
					else {
						DATA* temp = DataType::type_adaptation(return_value, ((DataType::Function*)node->value->value)->return_type);
						DataType::release_data(return_value);
						return_value = nullptr;
						if (not temp) {
							throw Error(ValueError, L"返回值类型不匹配");
						}
						else {
							return temp;
						}
					}
				}
			}
		}
		else {
			throw Error(EvaluationError, L"该对象不可被调用");
		}
	}
	// builtin function calling
	USHORT args_number_out = 0;
	void* function_ptr = Builtins::find(function_name, &args_number_out);
	if (function_ptr) {
		if (number_of_args != args_number_out) {
			throw Error(EvaluationError, L"调用的实参个数与定义不符");
		}
		else {
			DATA* data = ((DATA* (*)(DATA**))function_ptr)(args);
			for (USHORT data_index = 0; data_index != number_of_args; data_index++) {
				if (not args[data_index]->variable_data) {
					DataType::release_data(args[data_index]);
				}
			}
			return data;
		}
	}
	else {
		throw Error(EvaluationError, L"调用的函数不存在");
	}
}
