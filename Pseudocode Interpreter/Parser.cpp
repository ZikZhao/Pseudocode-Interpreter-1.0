#include <Windows.h>
#include <cmath>
#include "Parser.h"

LONGLONG current_instruction_index = 0;

void strip(wchar_t* line) {
	size_t start = 0;
	size_t end = wcslen(line);
	if (start == end) { return; }
	for (size_t index = 0; line[index] != 0; index++) {
		if (line[index] != 9 and line[index] != 32) {
			start = index;
			break;
		}
	}
	for (size_t index = wcslen(line) - 1;; index--) {
		if (line[index] != 9 and line[index] != 32) {
			end = index + 1;
			break;
		}
		if (index == 0) { break; }
	}
	memmove(line, line + start, (end - start) * 2);
	line[end - start] = 0;
}

long double string_to_real(wchar_t* expr) {
	double result = 0;
	USHORT start = expr[0] == 43 or expr[0] == 45 ? 1 : 0;
	for (USHORT index = start; expr[index] != 0; index++) {
		if (expr[index] >= 48 and expr[index] <= 57 or expr[index] == 46) {
			if (expr[index] != 46) {
				result *= 10;
				result += expr[index] - 48;
			}
			else {
				double weight = 1;
				for (USHORT index2 = index + 1; expr[index2] != 0; index2++) {
					weight /= 10;
					if (expr[index2] >= 48 and expr[index2] <= 57) {
						result += (expr[index2] - 48) * weight;
					}
					else {
						return 0;
					}
				}
				break;
			}
		}
		else {
			return 0;
		}
	}
	if (start == 1 and expr[0] == 45) {
		result = -result;
	}
	return result;
}

wchar_t* length_limiting(wchar_t* expr, size_t length_target) {
	// format a string with specific length
	size_t length_now = wcslen(expr);
	wchar_t* result = nullptr;
	if (length_now == length_target) {
		result = new wchar_t[length_target + 1];
		memcpy(result, expr, length_target * 2);
		result[length_target] = 0;
		result = expr;
	}
	else if (length_now > length_target) {
		result = new wchar_t[length_target + 1];
		memcpy(result, expr + (length_now - length_target), length_target * 2);
		result[length_target] = 0;
	}
	else {
		result = new wchar_t[length_target + 1];
		for (USHORT index = 0; index != length_target - length_now; index++) {
			result[index] = 48;
		}
		memcpy(result + (length_target - length_now), expr, length_now * 2);
		result[length_target] = 0;
	}
	return result;
}

size_t skip_string(wchar_t* expr) {
	// find the ender of a string when evaluating an expression
	char ender = expr[0] == 34 ? 34 : 39;
	for (size_t index = 1; expr[index] != 0; index++) {
		if (expr[index] == ender) {
			if (index != 1) {
				if (expr[index - 1] != 92) {
					return index;
				}
			}
			else {
				return index;
			}
		}
	}
	return wcslen(expr) - 1;
}

long match_keyword(wchar_t* expr, const wchar_t* keyword) {
	// find keyword in a string and returns its starting index
	if (wcslen(expr) < wcslen(keyword)) { return -1; }
	long upper_bound = wcslen(expr) - wcslen(keyword) + 1;
	size_t length = wcslen(keyword);
	for (long index = 0; index != upper_bound; index++) {
		if (expr[index] == keyword[0]) {
			if (index + wcslen(keyword) > wcslen(expr)) {
				return -1;
			}
			bool matched = true;
			for (long index2 = 0; index2 != length; index2++) {
				if (keyword[index2] != expr[index + index2]) {
					matched = false;
					break;
				}
			}
			if (matched) {
				return index;
			}
		}
	}
	return -1;
}

PARAMETER::~PARAMETER()
{
	delete[] name;
	delete[] type_string;
}

RPN_EXP::~RPN_EXP() {
	if (rpn) {
		delete[] rpn;
	}
}

void CONSTRUCT::release_tokens() {
	delete result.tokens;
}
CONSTRUCT::~CONSTRUCT() {
	if (not result.matched) { return; }
	switch (syntax_index) {
	case Construct::_declaration:
		for (USHORT index = 0; index != *(USHORT*)result.args[1]; index++) {
			delete[]((wchar_t**)result.args[2])[index];
		}
		delete (USHORT*)result.args[1];
		delete[] (wchar_t**)result.args[2];
		delete[] (wchar_t*)result.args[3];
		if (((wchar_t*)result.args[0])[0] == L'A') {
			delete[] (USHORT*)result.args[4];
		}
		break;
	case Construct::_constant:
		delete[] (wchar_t*)result.args[0];
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_type_header:
		delete[] (wchar_t*)result.args[0];
		break;
	case Construct::_pointer_type_header:
		delete[] (wchar_t*)result.args[0];
		delete[] (wchar_t*)result.args[1];
		break;
	case Construct::_enumerated_type_header:
		delete[] (wchar_t*)result.args[0];
		delete[] (wchar_t*)result.args[1];
		break;
	case Construct::_assignment:
		delete[] (wchar_t*)result.args[0];
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_output:
		delete (RPN_EXP*)result.args[0];
		break;
	case Construct::_input:
		delete[] (wchar_t*)result.args[0];
		break;
	case Construct::_if_header_1: case Construct::_if_header_2:
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_if_ender:
		delete (Nesting*)result.args[0];
		break;
	case Construct::_case_of_header:
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_case_of_ender:
		delete (Nesting*)result.args[0];
		break;
	case Construct::_for_header_1: case Construct::_for_header_2:
		delete[] (wchar_t*)result.args[1];
		delete (RPN_EXP*)result.args[2];
		delete (RPN_EXP*)result.args[3];
		break;
	case Construct::_for_ender:
		delete (Nesting*)result.args[0];
		break;
	case Construct::_while_header:
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_while_ender:
		delete (Nesting*)result.args[0];
		break;
	case Construct::_repeat_ender:
		delete (Nesting*)result.args[0];
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_procedure_header:
		delete[](wchar_t*)result.args[1];
		delete (USHORT*)result.args[2];
		delete[](PARAMETER*)result.args[3];
		break;
	case Construct::_procedure_ender:
		delete (Nesting*)result.args[0];
		break;
	case Construct::_function_header:
		delete[] (wchar_t*)result.args[1];
		delete (USHORT*)result.args[2];
		delete[] (PARAMETER*)result.args[3];
		delete[] (wchar_t*)result.args[4];
		break;
	case Construct::_function_ender:
		delete (Nesting*)result.args[0];
		break;
	case Construct::_return_statement:
		if (*(bool*)result.args[1]) {
			delete (RPN_EXP*)result.args[2];
		}
		delete (bool*)result.args[1];
		break;
	case Construct::_try_ender:
		delete (Nesting*)result.args[0];
		break;
	case Construct::_openfile_statement:
		delete (RPN_EXP*)result.args[0];
		delete (wchar_t*)result.args[1];
		break;
	case Construct::_readfile_statement:
		delete (RPN_EXP*)result.args[0];
		delete[] (wchar_t*)result.args[1];
		break;
	case Construct::_writefile_statement:
		delete (RPN_EXP*)result.args[0];
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_closefile_statement:
		delete (RPN_EXP*)result.args[0];
		break;
	case Construct::_seek_statement:
		delete (RPN_EXP*)result.args[0];
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_getrecord_statement:
		delete (RPN_EXP*)result.args[0];
		delete[] (wchar_t*)result.args[1];
		break;
	case Construct::_putrecord_statement:
		delete (RPN_EXP*)result.args[0];
		delete (RPN_EXP*)result.args[1];
		break;
	case Construct::_single_expression:
		delete (RPN_EXP*)result.args[0];
		break;
	}
	delete[] (void*)result.args;
}

Error::Error(ErrorType error_type_in, const wchar_t* error_message_in) {
	this->error_type = error_type_in;
	this->error_message = error_message_in;
}

void BinaryTree::insert(wchar_t* key, DATA* value, bool constant) {
	BinaryTree::Node* node = new BinaryTree::Node{ key, wcslen(key) + 1, value, constant };
	if (this->root == nullptr) {
		this->root = node;
	}
	else {
		BinaryTree::Node* previous = nullptr;
		BinaryTree::Node* current = root;
		bool turnleft = false;
		while (current != nullptr) {
			USHORT index = 0;
			while ((*current).key[index] == key[index]) {
				index++;
			}
			previous = current;
			if ((*current).key[index] > key[index]) {
				turnleft = true;
				current = (*current).left;
			}
			else {
				turnleft = false;
				current = (*current).right;
			}
		}
		if (turnleft) {
			(*previous).left = node;
		}
		else {
			(*previous).right = node;
		}
	}
	this->number++;
}
BinaryTree::Node* BinaryTree::find(wchar_t* key) {
	if (this->root == nullptr or key == nullptr) { return nullptr; }
	BinaryTree::Node* current = root;
	while (current != nullptr) {
		bool flag = false;
		for (USHORT index = 0; key[index] != 0 and current->key[index]; index++) {
			if (key[index] != (*current).key[index]) {
				if ((*current).key[index] > key[index]) {
					current = (*current).left;
				}
				else {
					current = (*current).right;
				}
				flag = true;
				break;
			}
		}
		if (flag == false) {
			if (wcslen(key) == wcslen(current->key)) {
				return current;
			}
			else if (wcslen(key) > wcslen(current->key)) {
				current = (*current).right;
			}
			else {
				current = (*current).left;
			}
		}
	}
	return nullptr;
}
BinaryTree::Node* BinaryTree::list_nodes(BinaryTree::Node* current, USHORT* out_number) {
	if (not this->root) { return nullptr; }
	USHORT left_number = 0;
	USHORT right_number = 0;
	BinaryTree::Node* left_nodes = nullptr;
	BinaryTree::Node* right_nodes = nullptr;
	if (current->left) {
		left_nodes = this->list_nodes(current->left, &left_number);
	}
	if (current->right) {
		right_nodes = this->list_nodes(current->right, &right_number);
	}
	BinaryTree::Node* result = new BinaryTree::Node[left_number + right_number + 1];
	if (left_nodes) {
		memcpy(result, left_nodes, sizeof(BinaryTree::Node) * left_number);
		delete[] left_nodes;
	}
	memcpy(result + left_number, current, sizeof(BinaryTree::Node));
	if (right_nodes) {
		memcpy(result + 1 + left_number, right_nodes, sizeof(BinaryTree::Node) * right_number);
		delete[] right_nodes;
	}
	if (out_number) {
		*out_number = 1 + left_number + right_number;
	}
	return result;
}
BinaryTree::Node* BinaryTree::list_nodes_copy(BinaryTree::Node* current, HANDLE hProcess, USHORT* out_number)
{
	if (not this->root) { return nullptr; }
	USHORT left_number = 0;
	USHORT right_number = 0;
	BinaryTree::Node* left_nodes = nullptr;
	BinaryTree::Node* right_nodes = nullptr;
	if (current->left) {
		BinaryTree::Node* duplicate_left = (BinaryTree::Node*)malloc(sizeof(BinaryTree::Node));
		ReadProcessMemory(hProcess, current->left, duplicate_left, sizeof(BinaryTree::Node), nullptr);
		left_nodes = this->list_nodes_copy(duplicate_left, hProcess, &left_number);
		free(duplicate_left);
	}
	if (current->right) {
		BinaryTree::Node* duplicate_right = (BinaryTree::Node*)malloc(sizeof(BinaryTree::Node));
		ReadProcessMemory(hProcess, current->right, duplicate_right, sizeof(BinaryTree::Node), nullptr);
		right_nodes = this->list_nodes_copy(duplicate_right, hProcess, &right_number);
		free(duplicate_right);
	}
	BinaryTree::Node* result = new BinaryTree::Node[left_number + right_number + 1];
	if (left_nodes) {
		memcpy(result, left_nodes, sizeof(BinaryTree::Node) * left_number);
		delete[] left_nodes;
	}
	memcpy(result + left_number, current, sizeof(BinaryTree::Node));
	if (right_nodes) {
		memcpy(result + 1 + left_number, right_nodes, sizeof(BinaryTree::Node) * right_number);
		delete[] right_nodes;
	}
	if (out_number) {
		*out_number = 1 + left_number + right_number;
	}
	return result;
}
void BinaryTree::clear(BinaryTree::Node* current) {
	if (current->left) {
		this->clear(current->left);
	}
	if (current->right) {
		this->clear(current->right);
	}
	delete current;
}
void BinaryTree::clear() {
	if (this->root->left) {
		this->clear(this->root->left);
	}
	if (this->root->right) {
		this->clear(this->root->right);
	}
	delete this->root;
	this->root = nullptr;
	this->number = 0;
}

DataType::Any::Any() {
	this->init = false;
}
DataType::Integer::Integer() {
	this->init = false;
	this->value = 0;
}
DataType::Integer::Integer(wchar_t* expr) {
	this->init = true;
	this->value = (long long)string_to_real(expr);
}
DataType::Integer::Integer(long long value) {
	this->init = true;
	this->value = value;
}
DataType::Real::Real() {
	this->init = false;
	this->value = 0;
}
DataType::Real::Real(wchar_t* expr) {
	this->init = true;
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == 101) {
			wchar_t* mantissa = new wchar_t[index + 1];
			memcpy(mantissa, expr, index * 2);
			mantissa[index] = 0;
			this->value = string_to_real(mantissa);
			delete[] mantissa;
			wchar_t* exponent = new wchar_t[wcslen(expr) - index];
			memcpy(exponent, expr + index + 1, (wcslen(expr) - index - 1) * 2);
			exponent[wcslen(expr) - index - 1] = 0;
			this->value *= pow(10, string_to_real(exponent));
			return;
		}
	}
	this->value = string_to_real(expr);
}
DataType::Real::Real(long double value) {
	this->init = true;
	this->value = value;
}
DataType::Real::Real(Integer* int_obj) {
	this->init = true;
	this->value = (long double)int_obj->value;
}
DataType::Char::Char() {
	this->init = false;
	this->value = 0;
}
DataType::Char::Char(wchar_t* expr) {
	this->init = true;
	this->value = expr[1];
}
DataType::Char::Char(wchar_t value) {
	this->init = true;
	this->value = value;
}
DataType::String::String() {
	this->init = false;
	this->length = 0;
	this->string = nullptr;
}
DataType::String::String(size_t length, wchar_t* values) { // for internal creation
	this->init = true;
	if (wcslen(values) < length) {
		length = wcslen(values) + 1;
	}
	this->length = length;
	this->string = new wchar_t[length + 1];
	memcpy(this->string, values, (size_t)length * 2);
	this->string[length] = 0;
}
DataType::String::String(wchar_t* expr) { // created by evaluating string expression
	this->init = true;
	this->length = 1;
	this->string = new wchar_t[wcslen(expr) - 1];
	memcpy(this->string, expr + 1, (wcslen(expr) - 1) * 2);
	this->string[wcslen(expr) - 2] = 0;
	for (unsigned index = 0; this->string[index] != 0; index++) {
		if (this->string[index] == 92) {
			for (USHORT index2 = 0; index2 != 3; index2++) {
				if (this->string[index + 1] == escape_character_in[index2]) {
					this->string[index] = escape_character_out[index2];
					memmove(this->string + index + 1, this->string + index + 2, (wcslen(this->string) - index - 1) * 2);
					break;
				}
			}
		}
		this->length++;
	}
}
DataType::String::String(Char* character) {
	this->init = true;
	this->length = 1;
	this->string = new wchar_t[1] {character->value};
}
DataType::String::~String() {
	delete[] this->string;
}
void DataType::String::assign(unsigned index, Char character) {
	USHORT value = character.value;
	this->string[index] = value;
}
wchar_t* DataType::String::read(unsigned index) {
	wchar_t* result = new wchar_t[2];
	result[0] = this->string[index];
	result[1] = 0;
	return result;
}
DataType::String* DataType::String::combine(String* right_operand) {
	size_t final_length = this->length + (*right_operand).length;
	wchar_t* final_string = new wchar_t[final_length + 1];
	memcpy(final_string, this->string, (size_t)this->length * 2);
	memcpy(final_string + this->length, (*right_operand).string, (size_t)(*right_operand).length * 2);
	final_string[final_length] = 0;
	return new String(final_length, final_string);
}
bool DataType::String::smaller(String* right_operand) {
	wchar_t*& string1 = this->string;
	wchar_t*& string2 = right_operand->string;
	size_t bound = min(this->length, right_operand->length);
	for (size_t index = 0; index != bound; index++) {
		if (string1[index] < string2[index]) {
			return true;
		}
		else if (string1[index] > string2[index]) {
			return false;
		}
	}
	return false;
}
bool DataType::String::larger(String* right_operand) {
	return not smaller(right_operand);
}
DataType::Boolean::Boolean() {
	this->init = false;
	this->value = false;
}
DataType::Boolean::Boolean(bool value) {
	this->init = true;
	this->value = value;
}
DataType::Date::Date() {
	this->init = false;
	this->year = this->month = this->day = this->hour = this->minute = this->second = 0;
}
DataType::Date::Date(wchar_t* expr) {
	this->init = true;
	this->year = this->month = this->day = this->hour = this->minute = this->second = 0;
	USHORT* ptr[] = { &this->year, &this->month, &this->day,
		&this->hour, &this->minute, &this->second };
	int last_sign = -1;
	USHORT current = 0;
	for (USHORT index = 0; expr[index] != 0; index++) {
		if (expr[index] == 95) {
			wchar_t* part = new wchar_t[index - last_sign];
			memcpy(part, expr + last_sign + 1, (size_t)(index - last_sign - 1) * 2);
			part[index - last_sign - 1] = 0;
			last_sign = index;
			*ptr[current] = (USHORT)string_to_real(part);
			delete[] part;
			current++;
		}
		else if (expr[index] == 58) {
			if (current == 0) { current = 3; }
			wchar_t* part = new wchar_t[index - last_sign];
			memcpy(part, expr + last_sign + 1, (size_t)(index - last_sign - 1) * 2);
			part[index - last_sign - 1] = 0;
			last_sign = index;
			*ptr[current] = (USHORT)string_to_real(part);
			delete[] part;
			current++;
		}
	}
	wchar_t* part = new wchar_t[wcslen(expr) - last_sign];
	memcpy(part, expr + last_sign + 1, (size_t)(wcslen(expr) - last_sign - 1) * 2);
	part[wcslen(expr) - last_sign - 1] = 0;
	*ptr[current] = (USHORT)string_to_real(part);
	delete[] part;
}
DataType::Date::Date(USHORT second, USHORT minute, USHORT hour,
	USHORT day, USHORT month, USHORT year) {
	this->init = true;
	this->second = second;
	this->minute = minute;
	this->hour = hour;
	this->day = day;
	this->month = month;
	this->year = year;
}
DataType::Date* DataType::Date::add(Date* offset) {
	Date* new_date = new Date(this->second, this->minute, this->hour,
		this->day, this->month, this->year);
	new_date->second += offset->second;
	new_date->minute += offset->minute;
	new_date->hour += offset->hour;
	while (new_date->second >= 60) {
		new_date->minute++; new_date->second -= 60;
	}
	while (new_date->minute >= 60) {
		new_date->hour++; new_date->minute -= 60;
	}
	while (new_date->hour >= 60) {
		new_date->day++; new_date->hour -= 60;
	}
	bool flag = false;
	while (!flag) {
		flag = true;
		switch (new_date->month) {
		case 1: case 3: case 5: case 7: case 8: case 10:
			if (new_date->day >= 32) {
				new_date->month++;
				new_date->day -= 32;
				flag = false;
			}
			break;
		case 4: case 6: case 9: case 11:
			if (new_date->day >= 31) {
				new_date->month++;
				new_date->day -= 31;
				flag = false;
			}
			break;
		case 2:
			if (new_date->year % 4 == 0) {
				if (new_date->day >= 30) {
					new_date->month++;
					new_date->day -= 30;
					flag = false;
				}
			}
			else {
				if (new_date->day >= 29) {
					new_date->month++;
					new_date->day -= 29;
					flag = false;
				}
			}
			break;
		case 12:
			if (new_date->day >= 32) {
				new_date->year++;
				new_date->month = 1;
				new_date->day -= 32;
				flag = false;
			}
			break;
		}
	}
	return new_date;
}
wchar_t* DataType::Date::output() {
	wchar_t* result = new wchar_t[20];
	result[4] = result[7] = '_';
	result[10] = '+';
	result[13] = result[16] = ':';
	result[19] = 0;
	USHORT* ptr[] = { &this->year, &this->month, &this->day,
		&this->hour, &this->minute, &this->second };
	for (USHORT index = 0; index != 6; index++) {
		if (index == 0) {
			wchar_t* string = new wchar_t[5];
			StringCchPrintfW(string, 5, L"%04d", *ptr[index]);
			memcpy(result, string, 8);
			delete[] string;
		}
		else {
			wchar_t* string = new wchar_t[3];
			StringCchPrintfW(string, 3, L"%02d", *ptr[index]);
			memcpy(result + 2 + index * 3, string, 4);
			delete[] string;
		}
	}
	return result;
}
DataType::Array::Array() {
	this->init = false;
	this->size = nullptr;
	this->start_indexes = nullptr;
	this->type = 0;
	this->total_size = 0;
	this->dimension = 0;
	this->memory = nullptr;
}
DataType::Array::Array(DATA* type_data, USHORT* boundaries) {
	this->init = true;
	this->size = new USHORT[10];
	this->type = type_data;
	this->start_indexes = new USHORT[10];
	this->total_size = 1;
	this->dimension = 0;
	for (USHORT index = 0; index != 20; index += 2) {
		if (boundaries[index] != 0 or boundaries[index + 1] != 0) {
			USHORT range = boundaries[index + 1] - boundaries[index] + 1;
			this->size[(USHORT)(index / 2)] = range;
			this->total_size *= range;
			this->start_indexes[(USHORT)(index / 2)] = boundaries[index];
			this->dimension++;
		}
		else {
			break;
		}
	}
	this->memory = new DATA * [this->total_size];
	for (USHORT index = 0; index != this->total_size; index++) {
		this->memory[index] = new_empty_data(type_data);
		this->memory[index]->variable_data = true;
	}
}
DATA* DataType::Array::read(USHORT* indexes) {
	size_t offset = 0;
	for (USHORT index = 0; index != this->dimension; index++) {
		if (indexes[index] < this->start_indexes[index] or
			indexes[index] > this->start_indexes[index] + this->size[index]) {
			return new DATA{ 65535, (void*)L"下标超出可接受范围" };
		}
		offset *= this->size[index];
		offset += ((size_t)indexes[index] - this->start_indexes[index]);
	}
	if (offset < total_size) {
		return this->memory[offset];
	}
	else {
		return new DATA{ 65535, (void*)L"下标超出边界" };
	}
}
DataType::RecordType::RecordType(BinaryTree* field_info) {
	this->number_of_fields = 0;
	BinaryTree::BinaryTree::Node* all_fields = field_info->list_nodes(field_info->root, &this->number_of_fields);
	this->fields = new wchar_t* [this->number_of_fields];
	this->lengths = new size_t[this->number_of_fields];
	this->types = new DATA[this->number_of_fields];
	for (USHORT index = 0; index != this->number_of_fields; index++) {
		this->fields[index] = all_fields[index].key;
		this->lengths[index] = wcslen(this->fields[index]) + 1;
		memcpy(this->types + index, all_fields[index].value, sizeof(DATA));
	}
}
DataType::Record::Record() {
	this->init = false;
	this->source = nullptr;
	this->values = nullptr;
}
DataType::Record::Record(RecordType* source) {
	this->source = source;
	USHORT number = source->number_of_fields;
	this->values = new DATA * [number];
	for (USHORT index = 0; index != number; index++) {
		this->values[index] = new_empty_data(this->source->types + index);
		this->values[index]->variable_data = true;
	}
}
int DataType::Record::find(wchar_t* key) {
	for (USHORT index = 0; index != this->source->number_of_fields; index++) {
		if (wcslen(key) == wcslen(this->source->fields[index])) {
			bool matched = true;
			for (USHORT index2 = 0; key[index2] != 0; index2++) {
				if (key[index2] != this->source->fields[index][index2]) {
					matched = false;
					break;
				}
			}
			if (matched) {
				return index;
			}
		}
	}
	return -1;
}
DATA* DataType::Record::read(USHORT index) {
	return this->values[index];
}
DataType::PointerType::PointerType(DATA* type) {
	this->type = type;
}
DataType::PointerType::~PointerType() {
	if (this->type->type < 6) {
		delete this->type;
	}
}
DataType::Pointer::Pointer() {
	this->source = nullptr;
	this->node = nullptr;
}
DataType::Pointer::Pointer(PointerType* source) {
	this->source = source;
	this->node = nullptr; // validate by calling BinaryTree::new_pointer
}
void DataType::Pointer::reference(BinaryTree::BinaryTree::Node* target_node) {
	this->node = target_node;
	this->init = true;
	this->valid = true;
}
BinaryTree::Node* DataType::Pointer::dereference() {
	if (this->valid == false) {
		return nullptr;
	}
	else {
		return this->node;
	}
}
void DataType::Pointer::add_link(BinaryTree::BinaryTree::Node* referenced_node, DATA* pointer) {
	Pointer& pointer_object = *(Pointer*)pointer->value;
	pointer_object.forward_link = referenced_node->last_pointer;
	if (referenced_node->last_pointer) {
		((Pointer*)referenced_node->last_pointer->value)->backward_link = pointer;
	}
	referenced_node->last_pointer = pointer;
}
void DataType::Pointer::remove_link(DATA* pointer) {
	Pointer* pointer_object = (Pointer*)pointer->value;
	if (pointer_object->node) {
		if (pointer_object->backward_link) {
			((Pointer*)pointer_object->backward_link->value)->forward_link = pointer_object->forward_link;
		}
		else {
			pointer_object->node->last_pointer = pointer_object->forward_link;
		}
	}
}
void DataType::Pointer::invalidate(BinaryTree::BinaryTree::Node* node) {
	DATA* current_pointer = node->last_pointer;
	if (current_pointer) {
		do {
			((Pointer*)current_pointer->value)->valid = false;
			current_pointer = ((Pointer*)current_pointer->value)->forward_link;
		} while (current_pointer);
	}
}
DataType::EnumeratedType::EnumeratedType(wchar_t* expr) {
	USHORT last_sign = 0;
	this->length = 0;
	for (USHORT index = 1; expr[index] != 41; index++) {
		if (expr[index] == 44) {
			this->length++;
		}
	}
	this->length++;
	last_sign = 0;
	this->values = new wchar_t* [this->length];
	wchar_t* tag_expr = new wchar_t[wcslen(expr) + 1];
	memcpy(tag_expr, expr, (wcslen(expr) + 1) * 2);
	tag_expr[wcslen(expr) - 1] = 44;
	USHORT count = 0;
	for (USHORT index = 1; tag_expr[index] != 0; index++) {
		if (tag_expr[index] == 44) {
			wchar_t* new_value = new wchar_t[index - last_sign];
			memcpy(new_value, tag_expr + last_sign + 1, (size_t)(index - last_sign - 1) * 2);
			new_value[index - last_sign - 1] = 0;
			strip(new_value);
			this->values[count] = new_value;
			count++;
			last_sign = index;
		}
	}
	delete[] tag_expr;
}
void DataType::EnumeratedType::add_constants(EnumeratedType* type, BinaryTree* enumerations) {
	for (USHORT index = 0; index != type->length; index++) {
		BinaryTree::BinaryTree::Node* node = enumerations->find(type->values[index]);
		if (node) {
			delete node->value->value;
			node->value->value = new Integer(index);
		}
		else {
			DATA* data = new DATA{ 0, new Integer(index) };
			enumerations->insert(type->values[index], data);
		}
	}
}
DataType::Enumerated::Enumerated() {
	this->source = nullptr;
	this->current = 0;
}
DataType::Enumerated::Enumerated(EnumeratedType* source_in) {
	this->source = source_in;
	this->current = 0;
}
void DataType::Enumerated::assign(USHORT value) {
	this->current = value;
	this->init = true;
}
USHORT DataType::Enumerated::read() {
	return this->current;
}
DataType::Function::Function() {
	this->init = false;
	this->start_line = 0;
	this->number_of_args = 0;
	this->params = nullptr;
	this->return_type = nullptr;
}
DataType::Function::Function(size_t start_line_in, USHORT number_of_args_in, PARAMETER* params_in,
	DATA* return_type_in) {
	this->init = true;
	this->start_line = start_line_in;
	this->number_of_args = number_of_args_in;
	this->params = params_in;
	this->return_type = return_type_in;
}
bool DataType::Function::push_args(BinaryTree* locals, DATA** args) {
	for (USHORT index = 0; index != this->number_of_args; index++) {
		if (not ((Any*)args[index]->value)->init) {
			wchar_t* error_message = new wchar_t[17 + GET_DIGITS(index)];
			memcpy(error_message, L"参数 ", 6);
			USHORT value = index + 1;
			USHORT offset = 0;
			while (value >= 1) {
				error_message[3 + offset] = value % 10 + 48;
				value /= 10;
				offset += 1;
			}
			memcpy(error_message + 3 + GET_DIGITS(index), L" 尚未初始化", 14);
			throw Error(ValueError, error_message);
		}
		if (this->params[index].type->type < 7) {
			DATA* args_data = nullptr;
			if (args[index]->type != this->params[index].type->type) {
				switch ((args[index]->type << 8) + this->params[index].type->type) {
				case 1:
					args_data = new DATA{ 1, new Real((Integer*)args[index]->value) };
					break;
				case 256:
					args_data = new DATA{ 0, new Integer((long long)((Real*)args[index]->value)->value) };
					break;
				case 515:
					args_data = new DATA{ 3, new String((Char*)args[index]->value) };
					break;
				case 770:
					if (((String*)args[index]->value)->length != 1) { return false; }
					args_data = new DATA{ 2, new Char(((String*)args[index]->value)->string[0]) };
					break;
				default:
					return false;
				}
			}
			else {
				args_data = DataType::copy(args[index]);
			}
			args_data->variable_data = true;
			locals->insert(this->params[index].name, args_data);
		}
		else {
			if (args[index]->type != this->params[index].type->type + 1) {
				return false;
			}
			switch (args[index]->type) {
			case 8:
				if (((DataType::Record*)args[index]->value)->source != this->params[index].type->value) {
					return false;
				}
				break;
			case 10:
				if (((DataType::Pointer*)args[index]->value)->source != this->params[index].type->value) {
					return false;
				}
				break;
			case 12:
				if (((DataType::Enumerated*)args[index]->value)->source != this->params[index].type->value) {
					return false;
				}
				break;
			}
			DATA* args_data = DataType::copy(args[index]);
			args_data->variable_data = true;
			locals->insert(this->params[index].name, args_data);
		}
		if (not args[index]->variable_data) {
			DataType::release_data(args[index]);
		}
	}
	return true;
}
DataType::Address::Address(void* target) {
	this->value = target;
}

DATA* DataType::new_empty_data(DATA* type_data) { // value of this structure represent source
	void* object = nullptr;
	switch (type_data->type) {
	case 0:
		object = new DataType::Integer;
		break;
	case 1:
		object = new DataType::Real;
		break;
	case 2:
		object = new DataType::Char;
		break;
	case 3:
		object = new DataType::String;
		break;
	case 4:
		object = new DataType::Boolean;
		break;
	case 5:
		object = new DataType::Date;
		break;
	case 7:
		object = new DataType::Record((DataType::RecordType*)type_data->value);
		break;
	case 9:
		object = new DataType::Pointer((DataType::PointerType*)type_data->value);
		break;
	case 11:
		object = new DataType::Enumerated((DataType::EnumeratedType*)type_data->value);
		break;
	}
	if (type_data->type < 7) {
		return new DATA{ type_data->type, object };
	}
	else {
		return new DATA{ (USHORT)(type_data->type + 1), object };
	}
}
DATA* DataType::copy(DATA* original) {
	DATA* result_data = new DATA{ original->type, nullptr };
	switch (original->type) {
	case 0:
		result_data->value = new Integer;
		memcpy(result_data->value, original->value, sizeof(Integer));
		break;
	case 1:
		result_data->value = new Real;
		memcpy(result_data->value, original->value, sizeof(Real));
		break;
	case 2:
		result_data->value = new Char;
		memcpy(result_data->value, original->value, sizeof(Char));
		break;
	case 3:
		result_data->value = new String;
		memcpy(result_data->value, original->value, sizeof(String));
		if (not ((DataType::String*)result_data->value)->init) { break; }
		((String*)result_data->value)->string = new wchar_t[((String*)original->value)->length];
		memcpy(((String*)result_data->value)->string, ((String*)original->value)->string, (((String*)original->value)->length) * 2);
		break;
	case 4:
		result_data->value = new Boolean(*(DataType::Boolean*)original->value);
		break;
	case 5:
		result_data->value = new Date;
		memcpy(result_data->value, original->value, sizeof(Date));
		break;
	case 6:
	{
		USHORT* start = new USHORT[10];
		memcpy(start, ((Array*)original->value)->start_indexes, sizeof(USHORT) * 10);
		USHORT* size = new USHORT[10];
		memcpy(size, ((Array*)original->value)->size, sizeof(USHORT) * 10);
		result_data->value = new Array;
		memcpy(result_data->value, original->value, sizeof(Array));
		((Array*)result_data->value)->start_indexes = start;
		((Array*)result_data->value)->size = size;
		((Array*)result_data->value)->memory = new DATA * [((Array*)original->value)->total_size];
		for (size_t index = 0; index != ((Array*)original->value)->total_size; index++) {
			((Array*)result_data->value)->memory[index] = copy(((Array*)original->value)->memory[index]);
			((Array*)result_data->value)->memory[index]->variable_data = true;
		}
		((Array*)result_data->value)->type = new DATA;
		memcpy(((Array*)result_data->value)->type, ((Array*)original->value)->type, sizeof(DATA));
		break;
	}
	case 8:
	{
		result_data->value = new Record;
		memcpy(result_data->value, original->value, sizeof(Record));
		((Record*)result_data->value)->values = new DATA * [((Record*)result_data->value)->source->number_of_fields];
		for (USHORT index = 0; index != ((Record*)result_data->value)->source->number_of_fields; index++) {
			((Record*)result_data->value)->values[index] = copy(((Record*)original->value)->values[index]);
			((Record*)result_data->value)->values[index]->variable_data = true;
		}
		break;
	}
	case 10:
	{
		Pointer* ptr = new Pointer;
		memcpy(ptr, original->value, sizeof(Pointer));
		ptr->forward_link = nullptr;
		ptr->backward_link = nullptr;
		result_data->value = ptr;
		if (ptr->valid and ptr->init) {
			ptr->reference(((Pointer*)original->value)->dereference());
			Pointer::add_link(ptr->node, result_data);
		}
		break;
	}
	case 12:
		result_data->value = new Enumerated;
		memcpy(result_data->value, original->value, sizeof(Enumerated));
		break;
	case 14:
		result_data->value = new Address(((Address*)original->value)->value);
		break;
	}
	return result_data;
}
DATA* DataType::get_type(DATA* data_in) {
	DATA* type = new DATA{ data_in->type, nullptr };
	switch (data_in->type) {
	case 8:
		type->value = ((Record*)data_in->value)->source;
		break;
	case 10:
		type->value = ((Pointer*)data_in->value)->source;
		break;
	case 12:
		type->value = ((Enumerated*)data_in->value)->source;
		break;
	}
	return type;
}
DATA* DataType::type_adaptation(DATA* data_in, DATA* target_type) {
	// original data structure is not being released
	USHORT& type_in = data_in->type;
	USHORT& type_out = target_type->type;
	if (type_in == type_out or type_in == 14 and type_out == 10) {
		return copy(data_in);
	}
	DATA* data = nullptr;
	switch ((type_in << 8) + type_out) {
	case 1:
	{
		long long value = ((Integer*)(data_in->value))->value;
		data = new DATA{ 1, new Real((long double)value) };
		break;
	}
	case 256:
	{
		long double value = ((Real*)(data_in->value))->value;
		data = new DATA{ 0, new Integer((long long)value) };
		break;
	}
	case 515:
	{
		data = new DATA{ 3, new String((Char*)data_in->value) };
		break;
	}
	case 770:
	{
		if (((String*)data_in->value)->length != 1) {
			return nullptr;
		}
		else {
			wchar_t value = ((String*)(data_in->value))->string[0];
			data = new DATA{ 2, new Char(value) };
		}
		break;
	}
	case 12:
	{
		Enumerated* obj = new Enumerated((EnumeratedType*)target_type->value);
		obj->assign(((Integer*)data_in->value)->value);
		data = new DATA{ 12, obj };
		break;
	}
	default:
	{
		return nullptr;
	}
	}
	return data;
}
void DataType::release_data(DATA* data) {
	switch (data->type) {
	case 0:
		delete (Integer*)data->value;
		break;
	case 1:
		delete (Real*)data->value;
		break;
	case 2:
		delete (Char*)data->value;
		break;
	case 3:
		delete (String*)data->value;
		break;
	case 4:
		delete (Boolean*)data->value;
		break;
	case 5:
		delete (Date*)data->value;
		break;
	case 6:
		for (size_t index = 0; index != ((Array*)data->value)->total_size; index++) {
			release_data(((Array*)data->value)->memory[index]);
		}
		delete ((Array*)data->value)->memory;
		delete ((Array*)data->value)->type;
		delete ((Array*)data->value)->size;
		delete ((Array*)data->value)->start_indexes;
		delete (Array*)data->value;
		break;
	case 8:
		for (size_t index = 0; index != ((RecordType*)((Record*)data->value)->source)->number_of_fields; index++) {
			release_data(((Record*)data->value)->values[index]);
		}
		delete (Record*)data->value;
		break;
	case 10:
		Pointer::remove_link(data);
		delete (Pointer*)data->value;
		break;
	case 12:
		delete (Enumerated*)data->value;
		break;
	case 14:
		delete (Address*)data->value;
		break;
	}
	delete data;
}
bool DataType::check_identical(DATA* left, DATA* right) {
	if (left->type != right->type) { return false; }
	if (not ((Any*)left->value)->init or not ((Any*)right->value)->init) { return false; }
	switch (left->type) {
	case 0:
		return ((Integer*)left->value)->value == ((Integer*)right->value)->value;
	case 1:
		return ((Real*)left->value)->value == ((Real*)right->value)->value;
	case 2:
		return ((Char*)left->value)->value == ((Char*)right->value)->value;
	case 3:
	{
		wchar_t* left_string = ((String*)left->value)->string;
		wchar_t* right_string = ((String*)right->value)->string;
		if (wcslen(left_string) != wcslen(right_string)) { return false; }
		else {
			for (size_t index = 0; left_string[index] != 0; index++) {
				if (left_string[index] != right_string[index]) {
					return false;
				}
			}
			return true;
		}
	}
	case 4:
		return ((Boolean*)left->value)->value == ((Boolean*)right->value)->value;
	case 5:
	{
		Date* Ldate = (Date*)left->value;
		Date* Rdate = (Date*)right->value;
		size_t left_timestamp = (size_t)Ldate->second + ((size_t)Ldate->minute << 6) + ((size_t)Ldate->hour << 12) +
			((size_t)Ldate->day << 17) + ((size_t)Ldate->month << 22) + ((size_t)Ldate->year << 26);
		size_t right_timestamp = (size_t)Rdate->second + ((size_t)Rdate->minute << 6) + ((size_t)Rdate->hour << 12) +
			((size_t)Rdate->day << 17) + ((size_t)Rdate->month << 22) + ((size_t)Rdate->year << 26);
		return left_timestamp == right_timestamp;
	}
	case 6:
	{
		Array* Larray = (Array*)left->value;
		Array* Rarray = (Array*)right->value;
		if (Larray->total_size != Rarray->total_size) { return false; }
		for (size_t element_index = 0; element_index != Larray->total_size; element_index++) {
			if (not check_identical(Larray->memory[element_index], Rarray->memory[element_index])) {
				return false;
			}
		}
		return true;
	}
	case 8:
	{
		Record* Lrecord = (Record*)left->value;
		Record* Rrecord = (Record*)right->value;
		if (Lrecord->source != Rrecord->source) { return false; }
		for (size_t field_index = 0; field_index != ((RecordType*)Lrecord->source)->number_of_fields; field_index++) {
			if (not check_identical(Lrecord->values[field_index], Rrecord->values[field_index])) {
				return false;
			}
		}
		return true;
	}
	case 10:
	{
		Pointer* Lpointer = (Pointer*)left->value;
		Pointer* Rpointer = (Pointer*)right->value;
		return Lpointer->node == Rpointer->node;
	}
	case 12:
		return ((Enumerated*)left->value)->current == ((Enumerated*)right->value)->current;
	}
	return false;
}
wchar_t* DataType::output_data(DATA* data, size_t& count_out) {
	if (not ((DataType::Any*)data->value)->init) {
		count_out = 6;
		return new wchar_t[] { L"<未初始化>" };
	}
	switch (data->type) {
	case 0:
	{
		long long value = ((DataType::Integer*)data->value)->value;
		count_out = GET_DIGITS(value);
		wchar_t* string = new wchar_t[count_out + 1];
		StringCchPrintfW(string, count_out + 1, L"%d", value);
		return string;
	}
	case 1:
	{
		long double value = ((DataType::Real*)data->value)->value;
		count_out = GET_DIGITS(value) + 7;
		wchar_t* string = new wchar_t[count_out + 1];
		StringCchPrintfW(string, count_out + 1, L"%.6f", value);
		return string;
	}
	case 2:
	{
		wchar_t* buffer = new wchar_t[2];
		buffer[0] = ((DataType::Char*)data->value)->value;
		buffer[1] = 0;
		count_out = 1;
		return buffer;
	}
	case 3:
	{
		wchar_t* buffer = new wchar_t[((DataType::String*)data->value)->length];
		memmove(buffer, ((DataType::String*)data->value)->string, ((DataType::String*)data->value)->length * 2);
		count_out = ((DataType::String*)data->value)->length - 1;
		return buffer;
	}
	case 4:
	{
		if (((DataType::Boolean*)data->value)->value) {
			count_out = 4;
			return new wchar_t[] { L"TRUE" };
		}
		else {
			count_out = 5;
			return new wchar_t[] { L"FALSE" };
		}
	}
	case 5:
	{
		wchar_t* expr = ((DataType::Date*)data->value)->output();
		count_out = wcslen(expr);
		return expr;
	}
	default:
	{
		wchar_t* content = new wchar_t[] { L"<该数据仅能以对象形式输出>" };
		count_out = 14;
		return content;
	}
	}
}
wchar_t* DataType::output_data_as_object(DATA* data, size_t& count_out) {
	if (not ((DataType::Any*)data->value)->init) {
		count_out = 6;
		return new wchar_t[] { L"<未初始化>" };
	}
	switch (data->type) {
	case 0:
	{
		wchar_t* content = output_data(data, count_out);
		wchar_t* result_message = new wchar_t[count_out + 5];
		StringCchPrintfW(result_message, count_out + 5, L"整数(%s)", content);
		delete[] content;
		count_out += 4;
		return result_message;
	}
	case 1:
	{
		wchar_t* content = output_data(data, count_out);
		wchar_t* result_message = new wchar_t[count_out + 6];
		StringCchPrintfW(result_message, count_out + 6, L"浮点数(%s)", content);
		delete[] content;
		count_out += 5;
		return result_message;
	}
	case 2:
	{
		wchar_t* content = output_data(data, count_out);
		wchar_t* result_message = new wchar_t[count_out + 7];
		StringCchPrintfW(result_message, count_out + 7, L"字符(\'%s\')", content);
		delete[] content;
		count_out += 6;
		return result_message;
	}
	case 3:
	{
		wchar_t* content = output_data(data, count_out);
		wchar_t* result_message = new wchar_t[count_out + 8];
		StringCchPrintfW(result_message, count_out + 8, L"字符串(\"%s\")", content);
		delete[] content;
		count_out += 7;
		return result_message;
	}
	case 4:
	{
		wchar_t* content = output_data(data, count_out);
		wchar_t* result_message = new wchar_t[count_out + 6];
		StringCchPrintfW(result_message, count_out + 6, L"布尔值(%s)", content);
		delete[] content;
		count_out += 5;
		return result_message;
	}
	case 5:
	{
		wchar_t* content = output_data(data, count_out);
		wchar_t* result_message = new wchar_t[count_out + 8];
		StringCchPrintfW(result_message, count_out + 8, L"日期与时间(%s)", content);
		delete[] content;
		count_out += 7;
		return result_message;
	}
	case 6:
	{
		wchar_t** ptr = new wchar_t* [((DataType::Array*)data->value)->total_size];
		size_t* counts = new size_t[((DataType::Array*)data->value)->total_size];
		DWORD total_count = 3;
		for (size_t index = 0; index != ((DataType::Array*)data->value)->total_size; index++) {
			ptr[index] = output_data_as_object(((DataType::Array*)data->value)->memory[index], counts[index]);
			total_count += counts[index] + 2;
		}
		wchar_t* result_message = new wchar_t[total_count];
		static const wchar_t* head = L"数组[";
		memcpy(result_message, head, 6);
		DWORD offset = 3;
		for (LONG64 index = 0; index != ((DataType::Array*)data->value)->total_size; index++) {
			memcpy(result_message + offset, ptr[index], counts[index] * 2);
			static const wchar_t* spliter = L", ";
			memcpy(result_message + offset + counts[index], spliter, 4);
			offset += counts[index] + 2;
			delete[] ptr[index];
		}
		static const wchar_t* tail = L"]";
		memcpy(result_message + offset - 2, tail, 2);
		delete[] ptr;
		count_out = total_count;
		return result_message;
	}
	case 7:
	{
		USHORT length = ((DataType::RecordType*)data->value)->number_of_fields;
		DWORD total_count = 5;
		for (USHORT index = 0; index != length; index++) {
			total_count += wcslen(((DataType::RecordType*)data->value)->fields[index]) + 2;
		}
		wchar_t* result_message = new wchar_t[total_count + 2];
		static const wchar_t* head = L"结构体类型(";
		memcpy(result_message, head, 12);
		DWORD offset = 6;
		for (USHORT index = 0; index != length; index++) {
			size_t length = wcslen(((DataType::RecordType*)data->value)->fields[index]);
			memcpy(
				result_message + offset,
				((DataType::RecordType*)data->value)->fields[index],
				length * 2
			);
			static const wchar_t* spliter = L", ";
			memcpy(result_message + offset + length, spliter, 4);
			offset += length + 2;
		}
		static const wchar_t tail = L')';
		memcpy(result_message + offset - 2, &tail, 2);
		count_out = total_count;
		return result_message;
	}
	case 8:
	{
		USHORT length = ((DataType::RecordType*)((DataType::Record*)data->value)->source)->number_of_fields;
		wchar_t** value_ptr = new wchar_t* [length];
		size_t* value_lengths = new size_t[length];
		size_t total_count = 4;
		for (USHORT index = 0; index != length; index++) {
			DATA* value_data = ((DataType::Record*)data->value)->read(index);
			value_ptr[index] = output_data_as_object(value_data, value_lengths[index]);
			total_count += wcslen(((DataType::RecordType*)((DataType::Record*)data->value)->source)->fields[index]) + value_lengths[index] + 3;
		}
		wchar_t* result_message = new wchar_t[total_count];
		memcpy(result_message, L"结构体{", 8);
		size_t offset = 4;
		for (USHORT index = 0; index != length; index++) {
			wchar_t*& field = ((DataType::RecordType*)((DataType::Record*)data->value)->source)->fields[index];
			size_t field_length = wcslen(field);
			memcpy(result_message + offset, field, field_length * 2);
			result_message[offset + field_length] = L'=';
			memcpy(result_message + offset + field_length + 1, value_ptr[index], value_lengths[index] * 2);
			memcpy(result_message + offset + field_length + 1 + value_lengths[index], L", ", 4);
			offset += field_length + value_lengths[index] + 3;
			delete[] value_ptr[index];
		}
		delete[] value_ptr;
		delete[] value_lengths;
		memcpy(result_message + offset - 2, L"}", 2);
		count_out = total_count;
		return result_message;
	}
	case 9:
	{
		const wchar_t* content = nullptr;
		switch (((DataType::PointerType*)data->value)->type->type) {
		case 0:
			count_out = 4;
			content = L"整数类型";
			break;
		case 1:
			count_out = 5;
			content = L"浮点数类型";
			break;
		case 2:
			count_out = 4;
			content = L"字符类型";
			break;
		case 3:
			count_out = 5;
			content = L"字符串类型";
			break;
		case 4:
			count_out = 5;
			content = L"布尔值类型";
			break;
		case 5:
			count_out = 7;
			content = L"日期与时间类型";
			break;
		case 6:
			count_out = 4;
			content = L"数组类型";
			break;
		case 8:
			count_out = 5;
			content = L"结构体类型";
			break;
		case 10:
			count_out = 4;
			content = L"指针类型";
			break;
		case 12:
			count_out = 4;
			content = L"枚举类型";
			break;
		}
		wchar_t* message = new wchar_t[count_out + 11];
		StringCchPrintfW(message, count_out + 11, L"指针类型(目标: %s）", content);
		count_out += 10;
		return message;
	}
	case 10:
	{
		if (not ((Pointer*)data->value)->valid) {
			count_out = 6;
			return new wchar_t[] { L"<指针无效>" };
		}
		count_out = wcslen(((Pointer*)data->value)->node->key);
		wchar_t* message = new wchar_t[count_out + 9];
		StringCchPrintfW(message, count_out + 9, L"指针(目标：%s)", ((Pointer*)data->value)->node->key);
		count_out += 8;
		return message;
	}
	case 11:
	{
		wchar_t**& terms = ((DataType::EnumeratedType*)data->value)->values;
		DWORD total_count = 4;
		for (size_t index = 0; index != ((DataType::EnumeratedType*)data->value)->length; index++) {
			total_count += wcslen(terms[index]) + 2;
		}
		wchar_t* buffer = new wchar_t[total_count + 2];
		static const wchar_t* head = L"枚举类型(";
		memcpy(buffer, head, 10);
		DWORD offset = 5;
		for (USHORT index = 0; index != ((DataType::EnumeratedType*)data->value)->length; index++) {
			size_t count_out = wcslen(terms[index]);
			memcpy(buffer + offset, terms[index], count_out * 2);
			static const wchar_t* spliter = L", ";
			memcpy(buffer + offset + count_out, spliter, 4);
			offset += count_out + 2;
		}
		static const wchar_t tail = L')';
		buffer[total_count - 1] = tail;
		count_out = total_count;
		return buffer;
	}
	case 12:
	{
		USHORT choice = ((DataType::Enumerated*)data->value)->current;
		wchar_t*& term = ((DataType::EnumeratedType*)((DataType::Enumerated*)data->value)->source)->values[choice];
		count_out = wcslen(term) + GET_DIGITS(choice) + 12;
		wchar_t* message = new wchar_t[count_out + 1];
		StringCchPrintfW(message, count_out + 1, L"枚举(值：%d，枚举常量：%s)", choice, term);
		return message;
	}
	}
	return nullptr;
}
char* DataType::sequentialise(DATA* data) {
	char* digest = nullptr;
	if (not ((Any*)data->value)->init) {
		return digest;
	}
	switch (data->type) {
	case 0:
	{
		digest = new char[1 + sizeof(long long)];
		digest[0] = 0;
		memcpy(digest + 1, &((Integer*)data->value)->value, sizeof(long long));
		break;
	}
	case 1:
	{
		digest = new char[1 + sizeof(long double)];
		digest[0] = 1;
		memcpy(digest + 1, &((Real*)data->value)->value, sizeof(long double));
		break;
	}
	case 2:
	{
		digest = new char[1 + sizeof(char)];
		digest[0] = 2;
		memcpy(digest + 1, &((Char*)data->value)->value, sizeof(char));
		break;
	}
	case 3:
	{
		size_t length = ((String*)data->value)->length;
		digest = new char[1 + length * 2];
		digest[0] = 3;
		memcpy(digest + 1, &length, 8);
		memcpy(digest + 9, ((String*)data->value)->string, length * 2);
		break;
	}
	case 4:
	{
		digest = new char[1 + sizeof(bool)];
		digest[0] = 4;
		memcpy(digest + 1, &((Boolean*)data->value)->value, sizeof(bool));
		break;
	}
	case 5:
	{
		Date* date = (Date*)data->value;
		size_t timestamp = (size_t)date->second + ((size_t)date->minute << 6) + ((size_t)date->hour << 12) +
			((size_t)date->day << 17) + ((size_t)date->month << 22) + ((size_t)date->year << 26);
		digest = new char[1 + sizeof(size_t)];
		digest[0] = 5;
		memcpy(digest + 1, &timestamp, sizeof(size_t));
		break;
	}
	}
	return digest;
}
DATA* DataType::desequentialise(char* digest) {
	DATA* data = nullptr;
	switch (digest[0]) {
	case 0:
	{
		long long value = 0;
		memcpy(&value, digest + 1, sizeof(long long));
		data = new DATA{ 0, new Integer(value) };
		break;
	}
	case 1:
	{
		long double value = 0;
		memcpy(&value, digest + 1, sizeof(long double));
		data = new DATA{ 1, new Real(value) };
		break;
	}
	case 2:
	{
		wchar_t value = 0;
		memcpy(&value, digest + 1, sizeof(wchar_t));
		data = new DATA{ 2, new Char(value) };
		break;
	}
	case 3:
	{
		size_t length = *(digest + 1);
		wchar_t* string = new wchar_t[length];
		memcpy(string, digest + 9, length * 2);
		data = new DATA{ 3, new String(wcslen(string), string) };
		break;
	}
	case 4:
	{
		bool value = 0;
		memcpy(&value, digest + 1, sizeof(bool));
		data = new DATA{ 4, new Boolean(value) };
		break;
	}
	case 5:
	{
		size_t timestamp = 0;
		memcpy(&timestamp, digest + 1, sizeof(size_t));
		USHORT second = 0, minute = 0, hour = 0, day = 0, month = 0, year = 0;
		year = (USHORT)((timestamp >> 26) & 0b11111111111111111111111111111111111111);
		month = (USHORT)(timestamp >> 22) & 0b1111;
		day = (USHORT)(timestamp >> 17) & 0b11111;
		hour = (USHORT)(timestamp >> 12) & 0b11111;
		minute = (USHORT)(timestamp >> 6) & 0b111111;
		second = (USHORT)timestamp & 0b111111;
		data = new DATA{ 5, new Date(second, minute, hour, day, month, year) };
		break;
	}
	}
	return data;
}

bool Element::variable(wchar_t* expr) {
	static const wchar_t* keywords[] = {
		L"DECLARE", L"CONSTANT", L"TYPE", L"ENDTYPE", L"OUTPUT", L"INPUT", L"IF", L"THEN", L"ELSE", L"ENDIF",
		L"CASE OF", L"OTHERWISE", L"ENDCASE", L"FOR", L"TO", L"NEXT", L"WHILE", L"ENDWHILE", L"REPEAT", L"UNTIL",
		L"PROCEDURE", L"ENDPROCEDURE", L"FUNCTION", L"RETURNS", L"ENDFUNCTION", L"CONTINUE", L"BREAK", L"RETURN",
		L"TRY", L"EXCEPT", L"ENDTRY", L"OPENFILE", L"READFILE", L"WRITEFILE", L"CLOSEFILE", L"SEEK", L"GETRECORD",
		L"PUTRECORD"
	};
	if (expr[0] == 0) { return false; }
	for (size_t index = 1; expr[index] != 0; index++) {
		if (not (expr[index] >= L'A' and expr[index] <= L'Z' or
			expr[index] >= L'a' and expr[index] <= L'z' or
			expr[index] == L'_' or
			expr[index] >= L'0' and expr[index] <= L'9')) {
			return false;
		}
	}
	if (not (expr[0] >= L'A' and expr[0] <= L'Z' or
		expr[0] >= L'a' and expr[0] <= L'z' or
		expr[0] == L'_')) {
		return false;
	}
	for (USHORT keyword_index = 0; keyword_index != sizeof(keywords) / sizeof(const wchar_t*); keyword_index++) {
		if (match_keyword(expr, keywords[keyword_index]) == 0 and wcslen(expr) == wcslen(keywords[keyword_index])) {
			return false;
		}
	}
	return true;
}
bool Element::integer(wchar_t* expr) {
	if (not wcslen(expr)) { return false; }
	USHORT start = expr[0] == 43 or expr[0] == 45 ? 1 : 0;
	for (size_t index = start; expr[index] != 0; index++) {
		if (not (expr[index] >= 48 and expr[index] <= 57)) {
			return false;
		}
	}
	return true;
}
bool Element::float_point_number(wchar_t* expr) {
	// format1: floating-point number
	bool point = false;
	USHORT start = expr[0] == 43 or expr[0] == 45 ? 1 : 0;
	for (size_t index = start; expr[index] != 0; index++) {
		if (not (expr[index] >= 48 and expr[index] <= 57 or expr[index] == 46)) {
			return false;
		}
		else if (expr[index] == 46) {
			if (point == true) {
				return false;
			}
			else {
				point = true;
			}
		}
	}
	return true;
}
bool Element::real(wchar_t* expr) {
	if (not wcslen(expr)) { return false; }
	if (float_point_number(expr)) {
		return true;
	}
	// format2: scientific expression
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == 101 and index != wcslen(expr) - 1 and index != 0) {
			wchar_t* mantissa = new wchar_t[index + 1];
			memcpy(mantissa, expr, index * 2);
			mantissa[index] = 0;
			USHORT exponent_length = (USHORT)(wcslen(expr) - 1 - index);
			wchar_t* exponent = new wchar_t[exponent_length + 1];
			memcpy(exponent, expr + (index + 1), (size_t)exponent_length * 2);
			exponent[exponent_length] = 0;
			bool match_result = float_point_number(mantissa) and integer(exponent);
			delete[] mantissa;
			delete[] exponent;
			return match_result;
		}
	}
	return false;
}
bool Element::character(wchar_t* expr) {
	if (wcslen(expr) == 4) {
		if (expr[0] == 39 and expr[3] == 39) {
			if (expr[1] == 92) { // escape character
				return true;
			}
		}
	}
	else if (wcslen(expr) == 3) {
		if (expr[0] == 39 and expr[2] == 39) {
			if (expr[1] != 92) {
				return true;
			}
		}
	}
	return false;
}
bool Element::string(wchar_t* expr) {
	if (not wcslen(expr) or wcslen(expr) == 1) { return false; }
	if (expr[0] == 34 and expr[wcslen(expr) - 1] == 34) {
		if (wcslen(expr) == 2) {
			return true; // empty string
		}
		else if (expr[wcslen(expr) - 2] != 92) { // escape character
			return true;
		}
	}
	return false;
}
bool Element::boolean(wchar_t* expr) {
	const wchar_t* keyword = nullptr;
	if (wcslen(expr) == 5) {
		keyword = L"false";
	}
	else if (wcslen(expr) == 4) {
		keyword = L"true";
	}
	else {
		return false;
	}
	return match_keyword(expr, keyword) == 0;
}
bool Element::date(wchar_t* expr) {
	if (not wcslen(expr)) { return false; }
	bool is_time = false;
	bool is_date = false;
	bool leap_year = false;
	size_t current = 0;
	size_t last_sign = 0;
	USHORT month = 0;
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == 95) { // date expression : xxxx_xx_xx
			if (is_time or current == 3) { return false; }
			if (current != 0) { if (index - last_sign > 2 or index - last_sign == 0) { return false; } }
			wchar_t* part = new wchar_t[index - last_sign + 1];
			memcpy(part, expr + last_sign, (size_t)(index - last_sign) * 2);
			part[index - last_sign] = 0;
			bool match_result = integer(part);
			if (not match_result) {
				delete[] part;
				return false;
			}
			else {
				is_date = true;
				USHORT result = (USHORT)string_to_real(part);
				delete[] part;
				switch (current) {
				case 0:
					if (result % 4 == 0) {
						leap_year = true;
					}
					break;
				case 1:
					if (result <= 0 or result >= 13) {
						return false;
					}
					else {
						month = result;
					}
					break;
				}
			}
			current++;
			last_sign = index + 1;
		}
		else if (expr[index] == 58) { // time expression : xx:xx:xx
			if (is_date or current == 6) { return false; }
			if (current == 0) { current = 3; }
			if (index - last_sign > 2 or index - last_sign == 0) { return false; }
			wchar_t* part = new wchar_t[index - last_sign + 1];
			memcpy(part, expr + last_sign, (index - last_sign) * 2);
			part[index - last_sign] = 0;
			bool match_result = integer(part);
			if (not match_result) {
				delete[] part;
				return false;
			}
			else {
				is_time = true;
				USHORT result = (USHORT)string_to_real(part);
				delete[] part;
				switch (current) {
				case 3:
					if (result < 0 or result >= 24) {
						return false;
					}
					break;
				case 4:
					if (result < 0 or result >= 60) {
						return false;
					}
					break;
				}
				current++;
				last_sign = index + 1;
			}
		}
	}
	if (current != 2 and current != 5 or last_sign == wcslen(expr)) {
		return false;
	}
	if (!integer(expr + last_sign)) {
		return false;
	}
	USHORT last_part = (USHORT)string_to_real(expr + last_sign);
	if (is_time) {
		if (last_part < 0 or last_part >= 60) {
			return false;
		}
	}
	else {
		USHORT upper_limit = 0;
		switch (month) {
		case 1: case 3: case 5: case 7: case 8: case 10: case 12:
			upper_limit = 31;
			break;
		case 4: case 6: case 9: case 11:
			upper_limit = 30;
			break;
		case 2:
			if (leap_year) {
				upper_limit = 29;
			}
			else {
				upper_limit = 28;
			}
		}
		if (last_part <= 0 or last_part > upper_limit) {
			return false;
		}
	}
	return true;
}
bool Element::fundamental_type(wchar_t* expr, USHORT* type) {
	static const wchar_t* fundamentals[] = { L"INTEGER", L"REAL", L"CHAR", L"STRING", L"boolEAN", L"DATE" };
	size_t length = wcslen(expr);
	for (USHORT index = 0; index != 6; index++) {
		if (length == wcslen(fundamentals[index])) {
			if (match_keyword(expr, fundamentals[index]) == 0) {
				if (type) { *type = index; }
				return true;
			}
		}
	}
	return false;
}
bool Element::array_type(wchar_t* expr, USHORT** boundaries_out, wchar_t** type) {
	static const wchar_t* keyword = L"ARRAY";
	static const wchar_t* keyword2 = L" OF ";
	if (match_keyword(expr, keyword) == 0) {
		if (expr[5] != 91) { return false; }
		size_t last_comma = 0;
		size_t last_colon = 0;
		USHORT lower_bound = 0;
		USHORT upper_bound = 0;
		bool sum_lower = true;
		USHORT dimension = 0;
		USHORT* boundaries = new USHORT[20]; // maximum dimensions: 10
		memset(boundaries, 0, sizeof(USHORT) * 20);
		for (size_t index = 6; expr[index] != 0; index++) {
			switch (expr[index]) {
			case 93:
			{
				if (last_comma > last_colon or dimension == 10) {
					return false;
				}
				boundaries[dimension * 2] = lower_bound;
				boundaries[dimension * 2 + 1] = upper_bound;
				dimension++;
				if (wcslen(expr) <= index + 4) { return false; }
				for (size_t index2 = index + 1; index2 != index + 5; index2++) {
					if (expr[index2] != keyword2[index2 - index - 1]) {
						return false;
					}
				}
				wchar_t* type_part = new wchar_t[wcslen(expr) - index - 4];
				memcpy(type_part, expr + index + 5, (wcslen(expr) - index - 5) * 2);
				type_part[wcslen(expr) - index - 5] = 0;
				if (not variable(type_part)) {
					delete[] type_part;
					delete[] boundaries;
					return false;
				}
				if (boundaries_out) { *boundaries_out = boundaries; }
				else { delete[] boundaries; }
				if (type) { *type = type_part; }
				else { delete[] type_part; }
				return true;
			}
			case 58:
			{
				last_colon = index;
				sum_lower = false;
				break;
			}
			case 44:
			{
				if (last_comma > last_colon or dimension == 10) {
					return false;
				}
				boundaries[dimension * 2] = lower_bound;
				boundaries[dimension * 2 + 1] = upper_bound;
				dimension++;
				last_comma = index;
				upper_bound = 0;
				lower_bound = 0;
				sum_lower = true;
				break;
			}
			case 32:
			{
				continue;
			}
			default:
			{
				if (not (expr[index] >= 48 and expr[index] <= 57)) {
					return false;
				}
				if (sum_lower) {
					lower_bound *= 10;
					lower_bound += expr[index] - 48;
				}
				else {
					upper_bound *= 10;
					upper_bound += expr[index] - 48;
				}
			}
			}
		}
	}
	return false;
}
bool Element::pointer_type(wchar_t* expr) {
	if (expr[0] == 94) {
		expr++;
		strip(expr);
		return variable(expr);
	}
	return false;
}
bool Element::enumerated_type(wchar_t* expr) {
	if (wcslen(expr) < 2) { return false; }
	if (not (expr[0] == 40 and expr[wcslen(expr) - 1] == 41)) { return false; }
	size_t last_spliter = 0;
	for (size_t index = 1; expr[index] != 0; index++) {
		if (expr[index] == 44 or expr[index] == 41) {
			wchar_t* new_value = new wchar_t[index - last_spliter];
			memcpy(new_value, expr + last_spliter + 1, (size_t)(index - last_spliter - 1) * 2);
			new_value[index - last_spliter - 1] = 0;
			strip(new_value);
			if (!variable(new_value)) {
				delete[] new_value;
				return false;
			}
			delete[] new_value;
			if (expr[index] == 41) {
				if (wcslen(expr) - 1 != index) {
					return false;
				}
			}
			last_spliter = index;
		}
	}
	return true;
}
bool Element::indexes(wchar_t* expr) {
	if (not wcslen(expr)) { return false; }
	if (expr[0] != 91 or expr[wcslen(expr) - 1] != 93) {
		return false;
	}
	size_t last_comma = 0;
	size_t dimension = 0; // maximum dimension: 10
	size_t bracket = 1;
	for (size_t index = 1; expr[index] != 0; index++) {
		if (expr[index] == 91) {
			bracket++;
		}
		if (expr[index] == 44) {
			if (dimension == 10) {
				return false;
			}
			wchar_t* new_index = new wchar_t[index - last_comma];
			memcpy(new_index, expr + last_comma + 1, ((size_t)index - last_comma - 1) * 2);
			new_index[index - last_comma - 1] = 0;
			strip(new_index);
			bool match_result = expression(new_index);
			delete[] new_index;
			if (not match_result) {
				return false;
			}
			dimension++;
			last_comma = index;
		}
		else if (expr[index] == 93) {
			if (bracket != 1) {
				bracket--;
				continue;
			}
			if (dimension == 10) {
				return false;
			}
			wchar_t* new_index = new wchar_t[index - last_comma];
			memcpy(new_index, expr + last_comma + 1, (index - last_comma - 1) * 2);
			new_index[index - last_comma - 1] = 0;
			strip(new_index);
			bool match_result = expression(new_index) or integer(new_index);
			delete[] new_index;
			return match_result;
		}
	}
	return false;
}
bool Element::array_element_access(wchar_t* expr, wchar_t** array_name, wchar_t** index_out) {
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == 91) {
			wchar_t* array_part = new wchar_t[index + 1];
			memcpy(array_part, expr, index * 2);
			array_part[index] = 0;
			if (variable(array_part) and indexes(expr + index)) {
				if (array_name) { *array_name = array_part; }
				else { delete[] array_name; }
				if (index_out) {
					*index_out = new wchar_t[wcslen(expr) - index + 1];
					memcpy(*index_out, expr + index, (wcslen(expr) - index) * 2);
					(*index_out)[wcslen(expr) - index] = 0;
				}
				return true;
			}
			else {
				delete[] array_part;
				return false;
			}
		}
	}
	return false;
}
bool Element::variable_path(wchar_t* expr) {
	if (variable(expr) or array_element_access(expr)) {
		return true;
	}
	if (integer(expr) or float_point_number(expr)) {
		return false;
	}
	if (expr[0] == 94) { // dereference
		if (expr[1] == 40) { // in the form of ^(x.x).x
			for (size_t index = 2; expr[index] != 0; index++) {
				if (expr[index] == 34 or expr[index] == 39) {
					index += skip_string(expr + index);
				}
				else if (expr[index] == 41) {
					wchar_t* pointer_path = new wchar_t[index - 1];
					wchar_t* other_path = new wchar_t[wcslen(expr) - index];
					memcpy(pointer_path, expr + 2, (size_t)(index - 2) * 2);
					memcpy(other_path + 1, expr + index + 1, (wcslen(expr) - index - 2) * 2);
					pointer_path[index - 2] = 0;
					other_path[0] = 97;
					other_path[wcslen(expr) - index - 2] = 0;
					bool result = variable_path(pointer_path) and variable_path(pointer_path);
					delete[] pointer_path;
					delete[] other_path;
					return result;
				}
			}
			return false;
		}
		else { // in the form of ^x.x
			return variable_path(expr + 1);
		}
	}
	size_t last_sign = 0;
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == 46) { // field
			expr[index] = 0;
			if (not (variable(expr + last_sign) or array_element_access(expr + last_sign))) {
				expr[index] = 46;
				return false;
			}
			expr[index] = 46;
			last_sign = index + 1;
		}
	}
	return variable(expr + last_sign) or array_element_access(expr + last_sign); // last field
}
bool Element::addressing(wchar_t* expr) {
	if (not wcslen(expr)) { return false; }
	if (expr[0] == 64) {
		expr++;
		strip(expr);
		return variable(expr);
	}
	return false;
}
bool Element::operator_precedence(wchar_t character, USHORT* precedence_out) {
	// @ and ^ are special operators because their mechanism so we do not process it in an expression
	// for ^, see valid_variable_path
	// for @, see addressing
	USHORT precedence = 0;
	switch (character) {
	case 42: case 47: // * /
		precedence = 1;
		break;
	case 43: case 45: case 38: // + - &
		precedence = 2;
		break;
	case 60: case 61: case 62: case 1: case 2: case 3: // comparison operator
		precedence = 3;
		break;
	case 6: // NOT
		precedence = 4;
		break;
	case 4: // AND
		precedence = 5;
		break;
	case 5: // OR
		precedence = 6;
		break;
	}
	if (precedence_out) {
		*precedence_out = precedence;
	}
	return precedence != 0;
}
bool Element::parameter_list(wchar_t* expr, PARAMETER** param_out, USHORT* count_out) {
	if (wcslen(expr) == 2) {
		if (expr[0] == L'(' and expr[1] == L')') {
			if (param_out) { *param_out = new PARAMETER[0]; }
			if (count_out) { *count_out = 0; }
			return true;
		}
	}
	USHORT count = 1;
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == L',') {
			count++;
		}
	}
	size_t last_spliter = 0;
	size_t last_colon = 0;
	PARAMETER* params = new PARAMETER[count];
	count = 0;
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == L',' or expr[index] == L')') {
			if (last_spliter == last_colon or last_spliter > last_colon) {
				for (USHORT arg_index = 0; arg_index != count; arg_index++) {
					delete[] params[arg_index].name;
					delete[] params[arg_index].type;
				}
				delete[] params;
				return false;
			}
			wchar_t* former_part = new wchar_t[last_colon - last_spliter];
			memcpy(former_part, expr + last_spliter + 1, (last_colon - last_spliter - 1) * 2);
			former_part[last_colon - last_spliter - 1] = 0;
			strip(former_part);
			size_t start = 0; // start index of parameter name
			for (size_t index2 = 0; former_part[index2] != 0; index2++) {
				if (former_part[index2] == L' ') {
					start = index2 + 1;
					break;
				}
			}
			wchar_t* parameter_name = new wchar_t[wcslen(former_part) - start + 1];
			memcpy(parameter_name, former_part + start, (wcslen(former_part) - start + 1) * 2);
			strip(parameter_name);
			wchar_t* keyword = nullptr;
			if (start) {
				keyword = new wchar_t[start];
				memcpy(keyword, former_part, (start - 1) * 2);
				keyword[start - 1] = 0;
				strip(keyword);
			}
			delete[] former_part;
			wchar_t* type_part = new wchar_t[index - last_colon];
			memcpy(type_part, expr + last_colon + 1, (index - last_colon - 1) * 2);
			type_part[index - last_colon - 1] = 0;
			strip(type_part);
			bool valid = variable(parameter_name) and (variable(type_part) or array_type(type_part));
			static const wchar_t* keyword_ref = L"BYREF";
			static const wchar_t* keyword_value = L"BYVALUE";
			bool byref = false;
			if (keyword) {
				byref = match_keyword(keyword, keyword_ref) == 0 and wcslen(keyword) == wcslen(keyword_ref);
				valid = valid and (byref or match_keyword(keyword, keyword_value) == 0 and wcslen(keyword) == wcslen(keyword_value));
				delete[] keyword;
			}
			if (not valid) {
				for (USHORT arg_index = 0; arg_index != count; arg_index++) {
					delete[] params[arg_index].name;
					delete[] params[arg_index].type;
				}
				delete[] params;
				return false;
			}
			params[count].name = parameter_name;
			params[count].type_string = type_part;
			params[count].passed_by_ref = byref;
			last_spliter = index;
			last_colon = 0;
			count++;
			if (expr[index] == 41) { break; }
		}
		else if (expr[index] == 58 and not last_colon) {
			last_colon = index;
		}
	}
	if (param_out) { *param_out = params; }
	if (count_out) { *count_out = count; }
	return true;
}
bool Element::function_call(wchar_t* expr, RPN_EXP** rpn_out)
{
	RPN_EXP* rpn_exp = new RPN_EXP;
	size_t last_spliter = 0;
	USHORT number_of_bracket = 0;
	USHORT number_of_args = 0; // number of arguments passed to the function called
	size_t total_args_length = 0; // counting of every single character in combined rpn, including \0
	wchar_t* function_name = nullptr; // in the final stage it will be in a form of f/x, where x is the number of args
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == L'(') {
			last_spliter = index;
			number_of_bracket++;
			if (number_of_bracket != 1) {
				continue;
			}
			function_name = new wchar_t[index + 1];
			memcpy(function_name, expr, index * 2);
			function_name[index] = 0;
			if (not variable(function_name)) {
				delete[] function_name;
				delete rpn_exp;
				return false;
			}
			if (expr[index + 1] == L')') { // no argument
				wchar_t* final_function_name = new wchar_t[wcslen(function_name) + 3];
				memcpy(final_function_name, function_name, wcslen(function_name) * 2);
				final_function_name[wcslen(function_name)] = 9;
				final_function_name[wcslen(function_name) + 1] = L'0';
				final_function_name[wcslen(function_name) + 2] = 0;
				delete[] function_name;
				rpn_exp->rpn = final_function_name;
				rpn_exp->number_of_element = 1;
				if (rpn_out) { *rpn_out = rpn_exp; }
				else { delete rpn_exp; }
				return true;
			}
		}
		else if (expr[index] == L',' or expr[index] == L')') {
			wchar_t* this_arg = new wchar_t[index - last_spliter];
			memcpy(this_arg, expr + last_spliter + 1, (index - last_spliter - 1) * 2);
			this_arg[index - last_spliter - 1] = 0;
			strip(this_arg);
			RPN_EXP* arg_rpn = nullptr;
			if (not (expression(this_arg, &arg_rpn) and expr[index + 1] == 0)) {
				delete[] this_arg;
				delete[] function_name;
				delete rpn_exp;
				delete arg_rpn;
				return false;
			}
			delete[] this_arg;
			size_t this_args_length = 0;
			for (USHORT element_index = 0; element_index != arg_rpn->number_of_element; element_index++) {
				this_args_length += wcslen(arg_rpn->rpn + this_args_length) + 1;
			}
			wchar_t* combined_rpn_string = new wchar_t[total_args_length + this_args_length];
			if (rpn_exp->rpn) {
				memcpy(combined_rpn_string, rpn_exp->rpn, total_args_length * 2);
				delete[] rpn_exp->rpn;
			}
			memcpy(combined_rpn_string + total_args_length, arg_rpn->rpn, this_args_length * 2);
			total_args_length += this_args_length;
			rpn_exp->rpn = combined_rpn_string;
			rpn_exp->number_of_element += arg_rpn->number_of_element;
			delete arg_rpn;
			number_of_args++;
			last_spliter = index;
			if (expr[index] == L')') { // end of function calling
				if (number_of_bracket == 1) {
					//wchar_t* final_number_of_args = unsigned_to_string(number_of_args);
					size_t length = wcslen(function_name) + GET_DIGITS(number_of_args) + 2;
					wchar_t* final_function_name = new wchar_t[length];
					StringCchPrintfW(final_function_name, length, L"%s\x9%d", function_name, number_of_args);
					wchar_t* rpn_out_string = new wchar_t[total_args_length + wcslen(final_function_name) + 1];
					delete[] function_name;
					memcpy(rpn_out_string, rpn_exp->rpn, total_args_length * 2);
					memcpy(rpn_out_string + total_args_length, final_function_name, (wcslen(final_function_name) + 1) * 2);
					delete[] final_function_name;
					delete[] rpn_exp->rpn;
					rpn_exp->rpn = rpn_out_string;
					rpn_exp->number_of_element++;
					if (rpn_out) { *rpn_out = rpn_exp; }
					else { delete rpn_exp; }
					return true;
				}
				else {
					number_of_bracket--;
				}
			}
		}
	}
	delete rpn_exp;
}
bool Element::expression(wchar_t* expr, RPN_EXP** rpn_out) {
	if (not wcslen(expr)) { return false; }
	RPN_EXP* rpn_exp = new RPN_EXP;
	size_t length = wcslen(expr);
	wchar_t* tag_expr = new wchar_t[length + 1];
	memcpy(tag_expr, expr, (length + 1) * 2);
	// stage one: replacement of symbols, boolean constant
	/*
	replacement:
	1: <=
	2: <>
	3: >=
	4: AND
	5: OR
	6: NOT
	7: false
	8: true
	*/
	for (size_t index = 0; tag_expr[index] != 0; index++) {
		if (not index == 0) {
			if (tag_expr[index - 1] != 32) {
				continue;
			}
		}
		switch (tag_expr[index]) {
		case 34: case 39:
			index += skip_string(tag_expr + index);
			break;
		case 65:
			if (index < length - 2) { // AND
				if (tag_expr[index + 1] == 78 and tag_expr[index + 2] == 68 and (tag_expr[index + 3] == 32 or tag_expr[index + 3] == 0)) {
					tag_expr[index] = 4;
					memcpy(tag_expr + index + 1, tag_expr + index + 3, (length - index - 2) * 2);
					length -= 2;
				}
			}
			break;
		case 79:
			if (index < length - 1) { // OR
				if (tag_expr[index + 1] == 82 and (tag_expr[index + 2] == 32 or tag_expr[index + 2] == 0)) {
					tag_expr[index] = 5;
					memcpy(tag_expr + index + 1, tag_expr + index + 2, (length - index - 1) * 2);
					length -= 1;
				}
			}
			break;
		case 78:
			if (index < length - 2) { // NOT
				if (tag_expr[index + 1] == 79 and tag_expr[index + 2] == 84 and (tag_expr[index + 3] == 32 or tag_expr[index + 3] == 0)) {
					tag_expr[index] = 6;
					memcpy(tag_expr + index + 1, tag_expr + index + 3, (length - index - 2) * 2);
					length -= 2;
				}
			}
			break;
		case 84:
			if (index < length - 3) { // TRUE
				if (tag_expr[index + 1] == 82 and tag_expr[index + 2] == 85 and tag_expr[index + 3] == 69 and (tag_expr[index + 4] == 32 or tag_expr[index + 4] == 0)) {
					tag_expr[index] = 8;
					memcpy(tag_expr + index + 1, tag_expr + index + 4, (length - index - 3) * 2);
					length -= 3;
				}
			}
			break;
		case 70:
			if (index < length - 4) { // FALSE
				if (tag_expr[index + 1] == 65 and tag_expr[index + 2] == 76 and tag_expr[index + 3] == 83 and tag_expr[index + 4] == 69 and (tag_expr[index + 5] == 32 or tag_expr[index + 5] == 0)) {
					tag_expr[index] = 7;
					memcpy(tag_expr + index + 1, tag_expr + index + 5, (length - index - 4) * 2);
					length -= 4;
				}
			}
			break;
		case 60: case 61: case 62:
			if (index < length - 1) { // compound logic operator
				switch ((tag_expr[index] << 8) + tag_expr[index + 1]) {
				case 15933: case 15421: case 15422:
					tag_expr[index] = tag_expr[index] + tag_expr[index + 1] - 120;
					memcpy(tag_expr + index + 1, tag_expr + index + 2, (wcslen(tag_expr) - index - 1) * 2);
					length--;
				}
			}
			break;
		}
	}
	// stage two: convert to RPN expression
	size_t operator_index = 0;
	USHORT precedence = 0;
	USHORT tag_precedence = 0;
	int bracket = 0;
	int braces = 0;
	bool bracket_exist = false;
	for (size_t index = 0; tag_expr[index] != 0; index++) {
		switch (tag_expr[index]) {
		case 34: case 39:
			index += skip_string(tag_expr + index);
			break;
		case 40:
			bracket++;
			bracket_exist = true;
			break;
		case 91:
			braces++;
			break;
		case 41:
			bracket--;
			break;
		case 93:
			braces--;
			break;
		default:
			if (operator_precedence(tag_expr[index], &tag_precedence)) {
				if (precedence <= tag_precedence and not bracket and not braces) { // index != 0 is used to identify negative sign before integer
					if (not ((tag_expr[index] == 43 or tag_expr[index] == 45) and index == 0)) {
						precedence = tag_precedence;
						operator_index = index;
					}
				}
			}
		}
	}
	// stage three: check whether the expression is valid and convert into RPN
	if (not (bracket or braces)) {
		if (precedence) {
			// contains operator
			wchar_t operator_value = tag_expr[operator_index];
			wchar_t* left_operand = new wchar_t[operator_index + 1];
			wchar_t* right_operand = new wchar_t[length - operator_index];
			memcpy(left_operand, tag_expr, (size_t)operator_index * 2);
			memcpy(right_operand, tag_expr + operator_index + 1, (length - operator_index - 1) * 2);
			left_operand[operator_index] = 0;
			right_operand[length - operator_index - 1] = 0;
			strip(left_operand);
			strip(right_operand);
			RPN_EXP* rpn_left = nullptr;
			RPN_EXP* rpn_right = nullptr;
			bool result1 = expression(left_operand, &rpn_left);
			bool result2 = expression(right_operand, &rpn_right);
			if (operator_value != 6 and left_operand[0] != 0) {
				// two arity operator
				if (result1 and result2) {
					size_t length_left = 0;
					for (unsigned element_index = 0; element_index != rpn_left->number_of_element; element_index++) {
						length_left += wcslen(rpn_left->rpn + length_left) + 1;
					}
					size_t length_right = 0;
					for (unsigned element_index = 0; element_index != rpn_right->number_of_element; element_index++) {
						length_right += wcslen(rpn_right->rpn + length_right) + 1;
					}
					wchar_t* part_rpn = new wchar_t[length_left + length_right + 2];
					memcpy(part_rpn, rpn_left->rpn, length_left * 2);
					memcpy(part_rpn + length_left, rpn_right->rpn, length_right * 2);
					part_rpn[length_left + length_right] = operator_value;
					part_rpn[length_left + length_right + 1] = 0;
					rpn_exp->rpn = part_rpn;
					rpn_exp->number_of_element = rpn_left->number_of_element + rpn_right->number_of_element + 1;
					delete rpn_left;
					delete rpn_right;
					delete[] left_operand;
					delete[] right_operand;
					delete[] tag_expr;
					if (rpn_out) { *rpn_out = rpn_exp; }
					else { delete rpn_exp; }
					return true;
				}
				if (result1) {
					delete rpn_left;
				}
				else {
					delete rpn_right;
				}
			}
			else if (operator_value == 6 and left_operand[0] == 0) {
				// one arity operator
				size_t length_right = 0;
				for (unsigned element_index = 0; element_index != rpn_right->number_of_element; element_index++) {
					length_right += wcslen(rpn_right->rpn + length_right) + 1;
				}
				wchar_t* part_rpn = new wchar_t[length_right + 2];
				memcpy(part_rpn, rpn_right->rpn, length_right * 2);
				part_rpn[length_right] = operator_value;
				part_rpn[length_right + 1] = 0;
				rpn_exp->rpn = part_rpn;
				rpn_exp->number_of_element = rpn_right->number_of_element + 1;
				delete rpn_right;
				delete[] left_operand;
				delete[] right_operand;
				if (rpn_out) { *rpn_out = rpn_exp; }
				else { delete rpn_exp; }
				return true;
			}
			delete[] left_operand;
			delete[] right_operand;
		}
		else {
			// no operator found
			if (bracket_exist) {
				if (tag_expr[0] == L'(') {
					// is fully enclosed in brackets
					wchar_t* part_expr = new wchar_t[length - 1];
					memcpy(part_expr, tag_expr + 1, (size_t)(length - 2) * 2);
					part_expr[length - 2] = 0;
					RPN_EXP* part_rpn = new RPN_EXP;
					bool result = expression(part_expr, &part_rpn);
					delete[] part_expr;
					if (result) {
						delete[] tag_expr;
						delete rpn_exp;
						if (rpn_out) { *rpn_out = part_rpn; }
						else { delete part_rpn; }
						return true;
					}
				}
				else {
					// function calling
					RPN_EXP* rpn_call;
					if (function_call(tag_expr, &rpn_call)) {
						delete[] tag_expr;
						delete rpn_exp;
						if (rpn_out) { *rpn_out = rpn_call; }
						else { delete rpn_call; }
						return true;
					}
				}
			}
			else { // single element
				if (integer(tag_expr) or real(tag_expr) or character(tag_expr) or string(tag_expr) or
					date(tag_expr) or variable_path(tag_expr) or addressing(tag_expr)) {
					rpn_exp->rpn = tag_expr;
					rpn_exp->number_of_element = 1;
					if (rpn_out) { *rpn_out = rpn_exp; }
					else { delete rpn_exp; }
					return true;
				}
				else if ((tag_expr[0] == 7 or tag_expr[0] == 8) and tag_expr[1] == 0) {
					rpn_exp->rpn = tag_expr;
					rpn_exp->number_of_element = 1;
					if (rpn_out) { *rpn_out = rpn_exp; }
					else { delete rpn_exp; }
					return true;
				}
			}
		}
	}
	delete[] tag_expr;
	delete rpn_exp;
	return false;
}

RESULT Construct::empty_line(wchar_t* expr) {
	RESULT result;
	result.matched = true;
	if (wcslen(expr) == 0) { return result; }
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] != 32 and expr[index] != 9) {
			result.matched = false;
		}
	}
	if (result.matched) {
		result.tokens = new TOKEN[]{
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::declaration(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"DECLARE ") == 0) {
		USHORT count = 1;
		for (size_t index = 8; expr[index] != 0; index++) {
			if (expr[index] == L',') {
				count++;
			}
			else if (expr[index] == L':') {
				break;
			}
		}
		size_t last_spliter = 7;
		wchar_t** variables = new wchar_t* [count];
		TOKEN* variable_tokens = new TOKEN[count * 2];
		count = 0;
		for (size_t index = 7; expr[index] != 0; index++) {
			if (expr[index] == L',' or expr[index] == L':') {
				wchar_t* this_variable = new wchar_t[index - last_spliter];
				memcpy(this_variable, expr + last_spliter + 1, (index - last_spliter - 1) * 2);
				this_variable[index - last_spliter - 1] = 0;
				strip(this_variable);
				if (Element::variable(this_variable)) {
					variables[count] = this_variable;
					variable_tokens[count * 2] = TOKEN{ (USHORT)(index - last_spliter - 1), TOKENTYPE::Variable };
					variable_tokens[count * 2 + 1] = TOKEN{ 1, TOKENTYPE::Punctuation };
					count++;
				}
				else {
					delete[] this_variable;
					for (USHORT variable_index = 0; variable_index != count; variable_index++) {
						delete[] variables[variable_index];
					}
					return result;
				}
				last_spliter = index;
				if (expr[index] == L':') {
					wchar_t* type_part = new wchar_t[wcslen(expr) - index];
					memcpy(type_part, expr + index + 1, (wcslen(expr) - index) * 2);
					strip(type_part);
					USHORT* boundaries_out = nullptr;
					wchar_t* type_out = nullptr;
					if (Element::variable(type_part)) {
						result.matched = true;
						result.args = new void* [] {
							(void*)L"V",
								new USHORT{ count },
								variables,
								type_part,
						};
						result.tokens = new TOKEN[3 + count * 2];
						result.tokens[0] = TOKEN{ 8, TOKENTYPE::Keyword };
						memcpy(result.tokens + 1, variable_tokens, sizeof(TOKEN) * (count * 2));
						result.tokens[1 + count * 2] = TOKEN{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Type };
						result.tokens[2 + count * 2] = ENDTOKEN;
						delete[] variable_tokens;
					}
					else if (Element::array_type(type_part, &boundaries_out, &type_out)) {
						result.matched = true;
						delete[] type_part;
						result.args = new void* [] {
							(void*)L"A",
								new USHORT{ count },
								variables,
								type_out,
								boundaries_out,
						};
						result.tokens = new TOKEN[3 + count * 2];
						result.tokens[0] = TOKEN{ 8, TOKENTYPE::Keyword };
						memcpy(result.tokens + 1, variable_tokens, sizeof(TOKEN)* (count * 2));
						result.tokens[1 + count * 2] = TOKEN{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Type };
						result.tokens[2 + count * 2] = ENDTOKEN;
						delete[] variable_tokens;
					}
					else {
						for (USHORT variable_index = 0; variable_index != count; variable_index++) {
							delete[] variables[variable_index];
						}
						delete[] type_part;
					}
					return result;
				}
			}
		}
	}
	return result;
}
RESULT Construct::constant(wchar_t* expr) {
	RESULT result;
	static const wchar_t* keyword = L"CONSTANT ";
	if (match_keyword(expr, keyword) == 0) {
		for (size_t index = 9; expr[index] != 0; index++) {
			if (expr[index] == L'=') {
				wchar_t* variable_part = new wchar_t[index - 8];
				wchar_t* expression_part = new wchar_t[wcslen(expr) - index];
				memcpy(variable_part, expr + 9, 2 * ((size_t)index - 9));
				memcpy(expression_part, expr + (index + 1), (wcslen(expr) - index - 1) * 2);
				variable_part[index - 9] = 0;
				expression_part[wcslen(expr) - index - 1] = 0;
				strip(variable_part);
				strip(expression_part);
				RPN_EXP* rpn_out = nullptr;
				if (Element::variable(variable_part) and Element::expression(expression_part, &rpn_out)) {
					delete[] expression_part;
					result.matched = true;
					result.args = new void* [] {
						variable_part,
							rpn_out
					};
					result.tokens = new TOKEN[]{
						{ 9, TOKENTYPE::Keyword },
						{ (USHORT)(index - 9), TOKENTYPE::Variable },
						{ 1, TOKENTYPE::Punctuation },
						{ (USHORT)(wcslen(expr) - index - 1), TOKENTYPE::Expression },
						ENDTOKEN,
					};
					return result;
				}
				else {
					delete[] variable_part;
					delete[] expression_part;
					return result;
				}
			}
		}
	}
	return result;
}
RESULT Construct::type_header(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"TYPE ") == 0) {
		wchar_t* type_part = new wchar_t[wcslen(expr) - 4];
		memcpy(type_part, expr + 5, (wcslen(expr) - 5) * 2);
		type_part[wcslen(expr) - 5] = 0;
		strip(type_part);
		if (Element::variable(type_part)) {
			result.matched = true;
			result.args = new void* [] { type_part };
			result.tokens = new TOKEN[]{
				{ 5, TOKENTYPE::Keyword },
				{ (USHORT)(wcslen(expr) - 5), TOKENTYPE::Type },
				ENDTOKEN
			};
		}
		else {
			delete[] type_part;
		}
	}
	return result;
}
RESULT Construct::type_ender(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"ENDTYPE") == 0 and wcslen(expr) == 7) {
		result.matched = true;
		result.tokens = new TOKEN[]{
			{ 7, TOKENTYPE::Keyword },
			ENDTOKEN
		};
	}
	return result;
}
RESULT Construct::pointer_type_header(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"TYPE ") == 0) {
		for (size_t index = 5; expr[index] != 0; index++) {
			if (expr[index] == L'←') {
				wchar_t* type_part = new wchar_t[index - 4];
				wchar_t* pointer_part = new wchar_t[wcslen(expr) - index];
				memcpy(type_part, expr + 5, (index - 5) * 2);
				memcpy(pointer_part, expr + index + 1, (wcslen(expr) - index - 1) * 2);
				type_part[index - 5] = 0;
				pointer_part[wcslen(expr) - index - 1] = 0;
				strip(type_part);
				strip(pointer_part);
				if (Element::variable(type_part) and Element::pointer_type(pointer_part)) {
					result.matched = true;
					result.args = new void* [] {
						type_part,
							pointer_part,
					};
					result.tokens = new TOKEN[]{
						{ 5, TOKENTYPE::Keyword },
						{ (USHORT)(index - 5), TOKENTYPE::Type },
						{ 1, TOKENTYPE::Punctuation },
						{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Type },
						ENDTOKEN,
					};
				}
				else {
					delete[] type_part;
					delete[] pointer_part;
				}
				return result;
			}
		}
	}
	return result;
}
RESULT Construct::enumerated_type_header(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"TYPE ") == 0) {
		for (size_t index = 5; expr[index] != 0; index++) {
			if (expr[index] == L'=') {
				wchar_t* type_part = new wchar_t[index - 4];
				wchar_t* value_part = new wchar_t[wcslen(expr) - index];
				memcpy(type_part, expr + 5, (size_t)(index - 5) * 2);
				memcpy(value_part, expr + index + 1, (size_t)(wcslen(expr) - index - 1) * 2);
				type_part[index - 5] = 0;
				value_part[wcslen(expr) - index - 1] = 0;
				strip(type_part);
				strip(value_part);
				if (Element::variable(type_part) and Element::enumerated_type(value_part)) {
					result.matched = true;
					result.args = new void* [] {
						type_part,
							value_part,
					};
					result.tokens = new TOKEN[]{
						{ 5, TOKENTYPE::Keyword },
						{ (USHORT)(index - 5), TOKENTYPE::Type },
						{ 1, TOKENTYPE::Punctuation },
						{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Enumeration },
						ENDTOKEN,
					};
				}
				else {
					delete[] type_part;
					delete[] value_part;
				}
				return result;
			}
		}
	}
	return result;
}
RESULT Construct::assignment(wchar_t* expr) {
	RESULT result;
	for (size_t index = 0; expr[index] != 0; index++) {
		if (expr[index] == L'←') {
			wchar_t* variable_part = new wchar_t[index + 1];
			wchar_t* expression_part = new wchar_t[wcslen(expr) - index];
			memcpy(variable_part, expr, index * 2);
			memcpy(expression_part, expr + index + 1, (wcslen(expr) - index - 1) * 2);
			variable_part[index] = 0;
			expression_part[wcslen(expr) - index - 1] = 0;
			strip(expression_part);
			strip(variable_part);
			RPN_EXP* rpn_out = nullptr;
			if (Element::expression(expression_part, &rpn_out) and Element::variable_path(variable_part)) {
				delete[] expression_part;
				result.matched = true;
				result.args = new void* [] {
					variable_part,
						rpn_out,
				};
				result.tokens = new TOKEN[]{
					{ (USHORT)index, TOKENTYPE::Variable },
					{ 1, TOKENTYPE::Punctuation },
					{ (USHORT)(wcslen(expr) - index - 1), TOKENTYPE::Expression },
					ENDTOKEN,
				};
			}
			else {
				delete[] variable_part;
				delete[] expression_part;
			}
			return result;
		}
	}
	return result;
}
RESULT Construct::output(wchar_t* expr) {
	RESULT result;
	if (wcslen(expr) < 7) {
		return result;
	}
	if (match_keyword(expr, L"OUTPUT ") == 0) {
		wchar_t* expr_part = new wchar_t[wcslen(expr) - 6];
		memcpy(expr_part, expr + 7, (wcslen(expr) - 7) * 2);
		expr_part[wcslen(expr) - 7] = 0;
		RPN_EXP* rpn_out;
		if (Element::expression(expr_part, &rpn_out)) {
			delete[] expr_part;
			result.matched = true;
			result.args = new void* [1] { rpn_out };
			result.tokens = new TOKEN[]{
				{ 7, TOKENTYPE::Keyword },
				{ (USHORT)(wcslen(expr) - 7), TOKENTYPE::Expression },
				ENDTOKEN,
			};
		}
		else {
			delete[] expr_part;
		}
	}
	return result;
}
RESULT Construct::input(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"INPUT ") == 0) {
		if (Element::variable_path(expr + 6)) {
			wchar_t* variable_part = new wchar_t[wcslen(expr) - 5];
			memcpy(variable_part, expr + 6, (wcslen(expr) - 6) * 2);
			variable_part[wcslen(expr) - 6] = 0;
			result.matched = true;
			result.args = new void* [1] { variable_part };
			result.tokens = new TOKEN[]{
				{ 6, TOKENTYPE::Keyword },
				{ (USHORT)(wcslen(expr) - 6), TOKENTYPE::Expression },
				ENDTOKEN,
			};
		}
	}
	return result;
}
RESULT Construct::if_header_1(wchar_t* expr) {
	// IF <condition>
	//    THEN ...
	RESULT result;
	if (match_keyword(expr, L"IF ") == 0) {
		RPN_EXP* rpn_out = nullptr;
		if (Element::expression(expr + 3, &rpn_out)) {
			result.matched = true;
			result.args = new void* [] {
				nullptr,
					rpn_out,
			};
			result.tokens = new TOKEN[]{
				{ 3, TOKENTYPE::Keyword },
				{ (USHORT)(wcslen(expr) - 3), TOKENTYPE::Expression },
				ENDTOKEN,
			};
		}
	}
	return result;
}
RESULT Construct::if_header_2(wchar_t* expr) {
	// IF <condition> THEN
	//     ...
	RESULT result;
	long start = match_keyword(expr, L"IF ");
	long end = match_keyword(expr, L" THEN");
	if (start == -1 or end == -1) {
		return result;
	}
	if (start != 0 or end != wcslen(expr) - 5) {
		return result;
	}
	start += 2;
	wchar_t* condition = new wchar_t[end - start];
	memcpy(condition, expr + 3, (end - start - 1) * 2);
	condition[end - start - 1] = 0;
	strip(condition);
	RPN_EXP* rpn_out = nullptr;
	bool is_expression = Element::expression(condition, &rpn_out);
	delete[] condition;
	if (is_expression) {
		result.matched = true;
		result.args = new void* [] {
			nullptr,
			rpn_out,
		};
		result.tokens = new TOKEN[]{
			{ 3, TOKENTYPE::Keyword },
			{ (USHORT)(end - start - 1), TOKENTYPE::Expression },
			{ 5, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::then_tag(wchar_t* expr) {
	// THEN
	//     <statements>
	RESULT result;
	if (match_keyword(expr, L"THEN") == 0 and wcslen(expr) == 4) {
		result.matched = true;
		result.args = new void* [] { nullptr };
		result.tokens = new TOKEN[]{
			{ 4, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::else_tag(wchar_t* expr) {
	// ELSE
	//     <statements>
	RESULT result;
	if (match_keyword(expr, L"ELSE") == 0 and wcslen(expr) == 4) {
		result.matched = true;
		result.args = new void* [] { nullptr };
		result.tokens = new TOKEN[]{
			{ 4, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::if_ender(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"ENDIF") == 0 and wcslen(expr) == 5) {
		result.matched = true;
		result.args = new void* [] { nullptr };
		result.tokens = new TOKEN[]{
			{ 5, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::case_of_header(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"CASE OF ") == 0) {
		RPN_EXP* rpn_out = nullptr;
		if (Element::expression(expr + 8, &rpn_out)) {
			result.matched = true;
			result.args = new void* [] {
				nullptr,
					rpn_out,
			};
			result.tokens = new TOKEN[]{
				{ 8, TOKENTYPE::Keyword },
				{ (USHORT)(wcslen(expr) - 8), TOKENTYPE::Expression },
				ENDTOKEN,
			};
		}
	}
	return result;
}
RESULT Construct::case_tag(wchar_t* expr) {
	RESULT result;
	if (expr[wcslen(expr) - 1] == L':') {
		expr[wcslen(expr) - 1] = 0;
		RPN_EXP* rpn_out = nullptr;
		bool is_expression = Element::expression(expr, &rpn_out);
		expr[wcslen(expr)] = L':';
		if (is_expression) {
			result.matched = true;
			result.args = new void* [] {
				nullptr,
					rpn_out,
			};
			result.tokens = new TOKEN[]{
				{ (USHORT)(wcslen(expr) - 1), TOKENTYPE::Expression },
				{ 1, TOKENTYPE::Punctuation },
				ENDTOKEN,
			};
		}
	}
	return result;
}
RESULT Construct::otherwise_tag(wchar_t* expr) {
	RESULT result;
	if (wcslen(expr) != 9) { return result; }
	if (match_keyword(expr, L"OTHERWISE") == 0 and wcslen(expr) == 9) {
		result.matched = true;
		result.args = new void* [] { nullptr };
		result.tokens = new TOKEN[]{
			{ 9, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::case_of_ender(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"ENDCASE") == 0 and wcslen(expr) == 7) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 7, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::for_header_1(wchar_t* expr) {
	// FOR <variable> ← <lower_bound> TO <upper_bound>
	RESULT result;
	static const wchar_t* keywords[] = { L"FOR ", L"←", L" TO " };
	long positions[3] = {};
	for (USHORT index = 0; index != 3; index++) {
		positions[index] = match_keyword(expr, keywords[index]);
		if (positions[index] == -1) {
			return result;
		}
	}
	if (positions[0] != 0) {
		return result;
	}
	wchar_t* variable_part = new wchar_t[positions[1] - 3];
	wchar_t* lower_bound = new wchar_t[positions[2] - positions[1]];
	wchar_t* upper_bound = new wchar_t[wcslen(expr) - positions[2] - 3];
	memcpy(variable_part, expr + 4, (positions[1] - 4) * 2);
	memcpy(lower_bound, expr + positions[1] + 1, (positions[2] - positions[1] - 1) * 2);
	memcpy(upper_bound, expr + positions[2] + 4, (wcslen(expr) - positions[2] - 4) * 2);
	variable_part[positions[1] - 4] = 0;
	lower_bound[positions[2] - positions[1] - 1] = 0;
	upper_bound[wcslen(expr) - positions[2] - 4] = 0;
	strip(variable_part);
	strip(lower_bound);
	strip(upper_bound);
	RPN_EXP* rpn_lower_bound = nullptr;
	RPN_EXP* rpn_upper_bound = nullptr;
	bool matched = Element::variable(variable_part) and Element::expression(lower_bound, &rpn_lower_bound)
		and Element::expression(upper_bound, &rpn_upper_bound);
	delete[] lower_bound;
	delete[] upper_bound;
	if (not matched) {
		delete[] variable_part;
		if (rpn_lower_bound) { delete rpn_lower_bound; }
		if (rpn_upper_bound) { delete rpn_upper_bound; }
	}
	else {
		result.matched = true;
		result.args = new void* [] {
			nullptr,
				variable_part,
				rpn_lower_bound,
				rpn_upper_bound,
		};
		result.tokens = new TOKEN[]{
			{ 4, TOKENTYPE::Keyword },
			{ (USHORT)(positions[1] - 4), TOKENTYPE::Variable },
			{ 1, TOKENTYPE::Punctuation },
			{ (USHORT)(positions[2] - positions[1] - 1), TOKENTYPE::Expression },
			{ 4, TOKENTYPE::Keyword },
			{ (USHORT)(wcslen(expr) - positions[2] - 4), TOKENTYPE::Expression },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::for_header_2(wchar_t* expr) {
	// FOR <variable> ← <lower_bound> TO <upper_bound> STEP <step>
	RESULT result;
	long position = match_keyword(expr, L" STEP ");
	if (position != -1) {
		wchar_t* former_part = new wchar_t[position + 1];
		memcpy(former_part, expr, position * 2);
		former_part[position] = 0;
		RPN_EXP* rpn_out = nullptr;
		if (not Element::expression(expr + position + 6, &rpn_out)) {
			delete[] former_part;
			return result;
		}
		RESULT former_result = for_header_1(former_part);
		if (not former_result.matched) {
			delete[] former_part;
			delete rpn_out;
			return result;
		}
		result.matched = true;
		result.args = new void* [5];
		memcpy(result.args, former_result.args, sizeof(void*) * 4);
		result.args[4] = rpn_out;
		delete[] former_result.args;
		result.tokens = new TOKEN[9];
		memcpy(result.tokens, former_result.tokens, sizeof(TOKEN) * 6);
		result.tokens[6] = { 6, TOKENTYPE::Keyword };
		result.tokens[7] = { (USHORT)(wcslen(expr) - position - 5), TOKENTYPE::Expression };
		result.tokens[8] = ENDTOKEN;
		delete[] former_result.tokens;
	}
	return result;
}
RESULT Construct::for_ender(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"NEXT ") == 0) {
		if (Element::variable_path(expr + 5)) {
			result.matched = true;
			result.args = new void* [] {
				nullptr,
					expr + 5, // do not free
			};
			result.tokens = new TOKEN[]{
				{ 5, TOKENTYPE::Keyword },
				{ (USHORT)(wcslen(expr) - 5), TOKENTYPE::Variable },
				ENDTOKEN,
			};
		}
	}
	return result;
}
RESULT Construct::while_header(wchar_t* expr) {
	RESULT result;
	long positions[2] = {};
	positions[0] = match_keyword(expr, L"WHILE ");
	positions[1] = match_keyword(expr, L" DO");
	if (positions[0] != 0) { return result; }
	if (positions[1] != (long)wcslen(expr) - 3) { return result; }
	wchar_t* condition = new wchar_t[wcslen(expr) - 8];
	memcpy(condition, expr + 6, (wcslen(expr) - 9) * 2);
	condition[wcslen(expr) - 9] = 0;
	RPN_EXP* rpn_out = nullptr;
	bool matched_result = Element::expression(condition, &rpn_out);
	delete[] condition;
	if (matched_result) {
		result.matched = true;
		result.args = new void* [] {
			nullptr,
				rpn_out,
		};
		result.tokens = new TOKEN[]{
			{ 6, TOKENTYPE::Keyword },
			{ (USHORT)(wcslen(expr) - 9), TOKENTYPE::Expression },
			{ 3, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::while_ender(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"ENDWHILE") == 0 and wcslen(expr) == 8) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 8, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::repeat_header(wchar_t* expr) {
	RESULT result;
	static const wchar_t* keyword = L"REPEAT";
	if (match_keyword(expr, keyword) == 0) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 6, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::repeat_ender(wchar_t* expr) {
	RESULT result;
	static const wchar_t* keyword = L"UNTIL ";
	if (match_keyword(expr, keyword) == 0) {
		RPN_EXP* rpn_out = nullptr;
		if (Element::expression(expr + 6, &rpn_out)) {
			result.matched = true;
			result.args = new void* [] {
				nullptr,
					rpn_out,
			};
			result.tokens = new TOKEN[]{
				{ 6, TOKENTYPE::Keyword },
				{ (USHORT)(wcslen(expr) - 6), TOKENTYPE::Expression },
				ENDTOKEN,
			};
		}
	}
	return result;
}
RESULT Construct::procedure_header(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"PROCEDURE ") == 0 and expr[wcslen(expr) - 1] == L')') {
		for (size_t index = 0; expr[index] != 0; index++) {
			if (expr[index] == L'(') {
				wchar_t* function_name = new wchar_t[index - 9];
				memcpy(function_name, expr + 10, (index - 10) * 2);
				function_name[index - 10] = 0;
				strip(function_name);
				bool valid_function_name = Element::variable(function_name);
				if (not valid_function_name) {
					delete[] function_name;
					return result;
				}
				wchar_t* parameter_string = new wchar_t[wcslen(expr) - index + 1];
				memcpy(parameter_string, expr + index, (wcslen(expr) - index) * 2);
				parameter_string[wcslen(expr) - index] = 0;
				PARAMETER* params = nullptr;
				USHORT number_of_args = 0;
				bool valid_parameter_list = Element::parameter_list(parameter_string, &params, &number_of_args);
				delete[] parameter_string;
				if (not valid_parameter_list) {
					delete[] function_name;
					return result;
				}
				result.matched = true;
				result.args = new void* [] {
					nullptr,
						function_name,
						new USHORT{ number_of_args },
						params,
				};
				result.tokens = new TOKEN[]{
					{ 10, TOKENTYPE::Keyword },
					{ (USHORT)(index - 10), TOKENTYPE::Subroutine },
					{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Parameter },
					ENDTOKEN,
				};
				break;
			}
		}
	}
	return result;
}
RESULT Construct::procedure_ender(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"ENDPROCEDURE") == 0 and wcslen(expr) == 12) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 12, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::function_header(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"FUNCTION ") == 0) {
		long position = match_keyword(expr, L"RETURNS ");
		if (position == -1) { return result; }
		for (size_t index = 9; expr[index] != 0; index++) {
			if (expr[index] == L'(') {
				wchar_t* function_name = new wchar_t[index - 8];
				wchar_t* parameter_part = new wchar_t[position - index + 1];
				wchar_t* return_type = new wchar_t[wcslen(expr) - position - 7];
				memcpy(function_name, expr + 9, (index - 9) * 2);
				memcpy(parameter_part, expr + index, (position - index) * 2);
				memcpy(return_type, expr + position + 8, (wcslen(expr) - position - 8) * 2);
				function_name[index - 9] = 0;
				strip(function_name);
				parameter_part[position - index] = 0;
				strip(parameter_part);
				return_type[wcslen(expr) - position - 8] = 0;
				strip(return_type);
				PARAMETER* params;
				USHORT number_of_args;
				bool valid_name = Element::variable(function_name) and Element::variable(return_type);
				if (not valid_name) {
					delete[] function_name;
					delete[] return_type;
					delete[] parameter_part;
					return result;
				}
				bool match_result = Element::parameter_list(parameter_part, &params, &number_of_args);
				delete[] parameter_part;
				if (match_result) {
					result.matched = true;
					result.args = new void* [] {
						nullptr,
							function_name,
							new USHORT{ number_of_args },
							params,
							return_type,
					};
					result.tokens = new TOKEN[]{
						{ 9, TOKENTYPE::Keyword },
						{ (USHORT)(index - 9), TOKENTYPE::Subroutine },
						{ (USHORT)(position - index), TOKENTYPE::Parameter },
						{ 8, TOKENTYPE::Keyword },
						{ (USHORT)(wcslen(expr) - position - 8), TOKENTYPE::Type },
						ENDTOKEN,
					};
				}
				else {
					delete[] function_name;
					delete[] return_type;
				}
				break;
			}
		}
	}
	return result;
}
RESULT Construct::function_ender(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"ENDFUNCTION") == 0) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 11, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::continue_tag(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"CONTINUE") == 0) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 8, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::break_tag(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"BREAK") == 0) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 5, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::return_statement(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"RETURN") == 0) {
		if (wcslen(expr) > 7) {
			RPN_EXP* rpn_out = nullptr;
			if (Element::expression(expr + 7, &rpn_out)) {
				result.matched = true;
				result.args = new void* [] {
					nullptr,
						new bool{ true },
						rpn_out,
				};
				result.tokens = new TOKEN[]{
					{ 7, TOKENTYPE::Keyword },
					{ (USHORT)(wcslen(expr) - 7), TOKENTYPE::Expression },
					ENDTOKEN,
				};
			}
		}
		else if (wcslen(expr) == 7) {
			return result;
		}
		else {
			result.matched = true;
			result.args = new void* [] {
				nullptr,
					new bool{ false },
			};
			result.tokens = new TOKEN[]{
				{ 6, TOKENTYPE::Keyword },
				ENDTOKEN,
			};
		}
	}
	return result;
}
RESULT Construct::try_header(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"TRY") == 0 and wcslen(expr) == 3) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 3, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::except_tag(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"EXCEPT") == 0 and wcslen(expr) == 6) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 6, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::try_ender(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"ENDTRY") == 0 and wcslen(expr) == 6) {
		result.matched = true;
		result.args = new void* [] {nullptr};
		result.tokens = new TOKEN[]{
			{ 6, TOKENTYPE::Keyword },
			ENDTOKEN,
		};
	}
	return result;
}
RESULT Construct::openfile_statement(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"OPENFILE ") == 0) {
		long position = match_keyword(expr, L" FOR ");
		if (position == -1) {
			return result;
		}
		else {
			wchar_t* filename_string = new wchar_t[position - 8];
			memcpy(filename_string, expr + 9, (position - 9) * 2);
			filename_string[position - 9] = 0;
			RPN_EXP* rpn_out = nullptr;
			bool match_result = Element::expression(filename_string, &rpn_out);
			delete[] filename_string;
			if (match_result) {
				static const wchar_t* modes[] = { L"READ", L"WRITE", L"APPEND" };
				for (USHORT mode_index = 0; mode_index != 3; mode_index++) {
					if (match_keyword(expr, modes[mode_index]) != -1) {
						result.matched = true;
						result.args = new void* [] {
							rpn_out,
								new USHORT{ mode_index },
						};
						result.tokens = new TOKEN[]{
							{ 9, TOKENTYPE::Keyword },
							{ (USHORT)(position - 9), TOKENTYPE::Expression },
							{ 5, TOKENTYPE::Keyword },
							{ (USHORT)(wcslen(expr) - position - 4), TOKENTYPE::Keyword },
							ENDTOKEN,
						};
						return result;
					}
				}
			}
		}
	}
	return result;
}
RESULT Construct::readfile_statement(wchar_t* expr) {
	RESULT result;
	static const wchar_t* keyword = L"READFILE ";
	if (match_keyword(expr, keyword) == 0) {
		for (size_t index = 9; expr[index] != 0; index++) {
			if (expr[index] == L',') {
				wchar_t* filename = new wchar_t[index - 8];
				wchar_t* variable_path = new wchar_t[wcslen(expr) - index];
				memcpy(filename, expr + 9, (index - 9) * 2);
				memcpy(variable_path, expr + index + 1, (wcslen(expr) - index - 1) * 2);
				filename[index - 9] = 0;
				variable_path[wcslen(expr) - index - 1] = 0;
				strip(filename);
				strip(variable_path);
				RPN_EXP* rpn_out = nullptr;
				bool match_result = Element::expression(filename, &rpn_out) and Element::variable_path(variable_path);
				delete[] filename;
				if (match_result) {
					result.matched = true;
					result.args = new void* [] {
						rpn_out,
							variable_path,
					};
					result.tokens = new TOKEN[]{
						{ 9, TOKENTYPE::Keyword },
						{ (USHORT)(index - 9), TOKENTYPE::Expression },
						{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Expression },
						ENDTOKEN,
					};
				}
				else {
					if (rpn_out) {
						delete rpn_out;
					}
					delete[] variable_path;
				}
				return result;
			}
		}
	}
	return result;
}
RESULT Construct::writefile_statement(wchar_t* expr) {
	RESULT result;
	static const wchar_t* keyword = L"WRITEFILE ";
	if (match_keyword(expr, keyword) == 0) {
		for (size_t index = 10; expr[index] != 0; index++) {
			if (expr[index] == 44) {
				wchar_t* filename_string = new wchar_t[index - 9];
				wchar_t* message = new wchar_t[wcslen(expr) - index + 1];
				memcpy(filename_string, expr + 10, (index - 10) * 2);
				memcpy(message, expr + index + 1, (wcslen(expr) - index) * 2);
				filename_string[index - 10] = 0;
				message[wcslen(expr) - index] = 0;
				strip(filename_string);
				strip(message);
				RPN_EXP* rpn_out_filename = nullptr;
				RPN_EXP* rpn_out_message = nullptr;
				bool match_result = Element::expression(filename_string, &rpn_out_filename) and Element::expression(message, &rpn_out_message);
				delete[] filename_string;
				delete[] message;
				if (match_result) {
					result.matched = true;
					result.args = new void* [] {
						rpn_out_filename,
							rpn_out_message,
					};
					result.tokens = new TOKEN[]{
						{ 10, TOKENTYPE::Keyword },
						{ (USHORT)(index - 10), TOKENTYPE::Expression },
						{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Expression },
						ENDTOKEN,
					};
					return result;
				}
				else {
					if (rpn_out_filename) {
						delete rpn_out_filename;
					}
					if (rpn_out_message) {
						delete rpn_out_message;
					}
				}
				return result;
			}
		}
	}
	return result;
}
RESULT Construct::closefile_statement(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"CLOSEFILE ") == 0) {
		RPN_EXP* rpn_out = nullptr;
		if (Element::expression(expr + 10, &rpn_out)) {
			result.matched = true;
			result.args = new void* [] {
				rpn_out,
			};
			result.tokens = new TOKEN[]{
				{ 10, TOKENTYPE::Keyword },
				{ (USHORT)(wcslen(expr) - 10), TOKENTYPE::Expression },
				ENDTOKEN,
			};
		}
	}
	return result;
}
RESULT Construct::seek_statement(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"SEEK ") == 0) {
		for (size_t index = 5; expr[index] != 0; index++) {
			if (expr[index] == L',') {
				wchar_t* filename_string = new wchar_t[index - 4];
				wchar_t* address_expression = new wchar_t[wcslen(expr) - index];
				memcpy(filename_string, expr + 5, (index - 5) * 2);
				memcpy(address_expression, expr + index + 1, (wcslen(expr) - index - 1) * 2);
				filename_string[index - 5] = 0;
				address_expression[wcslen(expr) - index - 1] = 0;
				strip(filename_string);
				strip(address_expression);
				RPN_EXP* rpn_out_filename = nullptr;
				RPN_EXP* rpn_out_address = nullptr;
				bool match_result = Element::expression(filename_string, &rpn_out_filename) and \
					Element::expression(address_expression, &rpn_out_address);
				delete[] filename_string;
				if (match_result) {
					result.matched = true;
					result.args = new void* [] {
						rpn_out_filename,
							rpn_out_address
					};
					result.tokens = new TOKEN[]{
						{ 5, TOKENTYPE::Keyword },
						{ (USHORT)(index - 5), TOKENTYPE::Expression },
						{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Expression },
						ENDTOKEN,
					};
					return result;
				}
				else {
					if (rpn_out_filename) {
						delete rpn_out_filename;
					}
					if (rpn_out_address) {
						delete rpn_out_address;
					}
				}
			}
		}
	}
	return result;
}
RESULT Construct::getrecord_statement(wchar_t* expr) {
	RESULT result;
	if (match_keyword(expr, L"GETRECORD ") == 0) {
		for (size_t index = 10; expr[index] != 0; index++) {
			if (expr[index] == L',') {
				wchar_t* filename = new wchar_t[index - 8];
				wchar_t* variable_path = new wchar_t[wcslen(expr) - index];
				memcpy(filename, expr + 9, (index - 9) * 2);
				memcpy(variable_path, expr + index + 1, (wcslen(expr) - index - 1) * 2);
				filename[index - 9] = 0;
				variable_path[wcslen(expr) - index - 1] = 0;
				strip(filename);
				strip(variable_path);
				RPN_EXP* rpn_out = nullptr;
				bool match_result = Element::expression(filename, &rpn_out) and Element::variable_path(variable_path);
				delete[] filename;
				if (match_result) {
					result.matched = true;
					result.args = new void* [] {
						rpn_out,
							variable_path,
					};
					result.tokens = new TOKEN[]{
						{ 10, TOKENTYPE::Keyword },
						{ (USHORT)(index - 10), TOKENTYPE::Expression },
						{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Expression },
						ENDTOKEN,
					};
				}
				else {
					if (rpn_out) {
						delete rpn_out;
					}
					delete[] variable_path;
				}
				return result;
			}
		}
	}
	return result;
}
RESULT Construct::putrecord_statement(wchar_t* expr) {
	RESULT result;
	static const wchar_t* keyword = L"PUTRECORD ";
	if (match_keyword(expr, keyword) == 0) {
		for (size_t index = 10; expr[index] != 0; index++) {
			if (expr[index] == 44) {
				wchar_t* filename_string = new wchar_t[index - 9];
				wchar_t* message = new wchar_t[wcslen(expr) - index + 1];
				memcpy(filename_string, expr + 10, (index - 10) * 2);
				memcpy(message, expr + index + 1, (wcslen(expr) - index) * 2);
				filename_string[index - 10] = 0;
				message[wcslen(expr) - index] = 0;
				strip(filename_string);
				strip(message);
				RPN_EXP* rpn_out_filename = nullptr;
				RPN_EXP* rpn_out_message = nullptr;
				bool match_result = Element::expression(filename_string, &rpn_out_filename) and Element::expression(message, &rpn_out_message);
				delete[] filename_string;
				delete[] message;
				if (match_result) {
					result.matched = true;
					result.args = new void* [] {
						rpn_out_filename,
							rpn_out_message,
					};
					result.tokens = new TOKEN[]{
						{ 10, TOKENTYPE::Keyword },
						{ (USHORT)(index - 10), TOKENTYPE::Expression },
						{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Expression },
						ENDTOKEN,
					};
					return result;
				}
				else {
					if (rpn_out_filename) {
						delete rpn_out_filename;
					}
					if (rpn_out_message) {
						delete rpn_out_message;
					}
				}
				return result;
			}
		}
	}
	return result;
}
RESULT Construct::single_expression(wchar_t* expr) {
	RESULT result;
	RPN_EXP* rpn_out;
	if (Element::expression(expr, &rpn_out)) {
		result.matched = true;
		result.args = new void* [] {
			rpn_out,
		};
		result.tokens = new TOKEN[]{
			{ (USHORT)wcslen(expr), TOKENTYPE::Expression },
			ENDTOKEN,
		};
	}
	return result;
}
CONSTRUCT* Construct::parse(wchar_t* line) {
	for (USHORT index = 0; index != number_of_constructs; index++) {
		RESULT result = ((RESULT(*)(wchar_t*))constructs[index])(line);
		if (result.matched) {
			return new CONSTRUCT{ index, result };
		}
	}
	return new CONSTRUCT{ NULL, RESULT{} };
}

Nesting::Nesting(NestType type, LONGLONG first_tag_line_number) {
	this->nest_type = type;
	this->tag_number = 1;
	this->line_numbers = new LONGLONG[1]{ first_tag_line_number };
	if (this->nest_type == CASE or this->nest_type == FOR) {
		this->nest_info = new Info{};
	}
	else {
		this->nest_info = nullptr;
	}
}
Nesting::~Nesting() {
	delete[] this->line_numbers;
	if (this->nest_info) {
		if (this->nest_type == CASE) {
			delete[] this->nest_info->case_of_info.values;
		}
		else {
			DataType::release_data(this->nest_info->for_info.upper_bound);
		}
		delete this->nest_info;
	}
}
void Nesting::add_tag(LONGLONG new_tag_line_number) {
	LONGLONG* temp_line_numbers = new LONGLONG[this->tag_number + 1];
	memcpy(temp_line_numbers, this->line_numbers, sizeof(LONGLONG) * this->tag_number);
	temp_line_numbers[this->tag_number] = new_tag_line_number;
	delete[] this->line_numbers;
	this->line_numbers = temp_line_numbers;
	this->tag_number++;
}

File::File() {
	this->handle = nullptr;
	this->filename = nullptr;
	this->mode = 0;
	this->size = 0;
}
File::File(HANDLE handle_in, wchar_t* filename_in, USHORT mode_in) {
	this->handle = handle_in;
	this->filename = filename_in;
	this->mode = mode_in;
	GetFileSize(this->handle, &this->size);
}
wchar_t* File::readline(bool retain_new_line) {
	if (this->eof) {
		return nullptr;
	}
	char* buffer = nullptr;
	size_t offset = 0;
	static const USHORT part_size = 1024;
	while (true) {
		char* new_buffer = new char[part_size + offset];
		if (buffer) {
			memcpy(new_buffer, buffer, offset);
		}
		DWORD read = 0;
		if (not ReadFile(this->handle, new_buffer + offset, part_size, &read, nullptr)) {
			return nullptr;
		}
		else {
			delete[] buffer;
			for (size_t index = offset; index != offset + read; index++) {
				if (new_buffer[index] == 10) {
					buffer = new char[retain_new_line ? index + 2 : index];
					memcpy(buffer, new_buffer, retain_new_line ? index + 1 : index - 1);
					buffer[retain_new_line ? index + 1 : index - 1] = 0;
					if (retain_new_line) {
						buffer[index - 1] = '\r';
						buffer[index] = '\n';
					}
					delete[] new_buffer;
					LARGE_INTEGER large_int{};
					large_int.QuadPart = -(long long)read + index + 1;
					SetFilePointerEx(this->handle, large_int, nullptr, FILE_CURRENT);
					int tag_size = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, buffer, -1, nullptr, 0);
					wchar_t* wchar_buffer = new wchar_t[tag_size];
					MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, buffer, -1, wchar_buffer, tag_size);
					return wchar_buffer;
				}
			}
			if (read != part_size) { // EOF
				buffer = new char[read + 1];
				memcpy(buffer, new_buffer, read);
				buffer[read] = 0;
				delete[] new_buffer;
				int tag_size = MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, buffer, -1, nullptr, 0);
				wchar_t* wchar_buffer = new wchar_t[tag_size];
				MultiByteToWideChar(CP_UTF8, MB_COMPOSITE, buffer, -1, wchar_buffer, tag_size);
				this->eof = true;
				return wchar_buffer;
			}
		}
		buffer = new_buffer;
		offset += read;
	}
}
bool File::writeline(wchar_t* message, bool new_line, bool flush) {
	int tag_size = WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, message, (int)wcslen(message), nullptr, 0, 0, nullptr);
	char* buffer = new char[new_line ? tag_size + 2 : tag_size];
	WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, message, (int)wcslen(message), buffer, tag_size, 0, nullptr);
	if (new_line) {
		buffer[tag_size] = '\r';
		buffer[tag_size + 1] = '\n';
	}
	bool result = WriteFile(this->handle, buffer, new_line ? tag_size + 2 : tag_size, nullptr, nullptr);
	if (flush) {
		FlushFileBuffers(this->handle);
	}
	return result;
}
void File::putrecord(char* digest) {
	if (digest[0] == 3) {
		for (size_t tag_size = 1;; tag_size++) {
			if (digest[tag_size] == 0) {
				WriteFile(this->handle, digest, (DWORD)((tag_size + 1) * sizeof(char)), nullptr, nullptr);
				return;
			}
		}
	}
	else {
		WriteFile(this->handle, digest, (DWORD)size_of_record[digest[0]] + 1, nullptr, nullptr);
	}
}
char* File::getrecord() {
	char type = 0;
	if (this->eof) {
		return nullptr;
	}
	if (not ReadFile(this->handle, &type, 1, nullptr, nullptr)) {
		return nullptr;
	}
	if (type >= 6) {
		return nullptr;
	}
	if (type == 3) {
		static const USHORT part_size = 1024;
		char* buffer = new char[part_size];
		size_t offset = 0;
		DWORD read = 0;
		while (true) {
			if (not ReadFile(this->handle, buffer + offset, part_size, &read, nullptr)) {
				delete[] buffer;
				return nullptr;
			}
			for (size_t index = offset; index != offset + read; index++) {
				if (buffer[index] == 0) {
					char* digest = new char[index + 2];
					digest[0] = type;
					memcpy(digest + 1, buffer, index + 1);
					delete[] buffer;
					LARGE_INTEGER large_int{};
					large_int.QuadPart = -(long long)read + index + 1;
					SetFilePointerEx(this->handle, large_int, nullptr, FILE_CURRENT);
					return digest;
				}
			}
			if (read != part_size) {
				this->eof = true;
				return nullptr;
			}
		}
	}
	else {
		char* digest = new char[size_of_record[type] + 1];
		digest[0] = type;
		DWORD read = 0;
		if (not ReadFile(this->handle, digest + 1, (DWORD)size_of_record[type], &read, nullptr)) {
			delete[] digest;
			return nullptr;
		}
		else if (read != size_of_record[type]) {
			delete[] digest;
			return nullptr;
		}
		return digest;
	}
}
void File::closefile() {
	delete[] this->filename;
	CloseHandle(this->handle);
}
void File::seek(size_t address) {
	LARGE_INTEGER offset{};
	offset.QuadPart = address;
	LARGE_INTEGER position{};
	SetFilePointerEx(this->handle, offset, &position, FILE_BEGIN);
	if ((size_t)position.QuadPart == (size_t)this->size - 1) {
		this->eof = true;
	}
}
