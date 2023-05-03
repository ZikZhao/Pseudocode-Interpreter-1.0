#pragma once

enum AVAILABLE_SETTINGS {
	AUTOMATIC_NEW_LINE_AFTER_OUTPUT = 1,
	OUTPUT_AS_OBJECT = 2,
	DISCARD_CRLF_IN_READ = 4,
	AUTOMATIC_NEW_LINE_IN_WRITE = 8,
	FLUSH_FILE_BUFFERS_AFTER_WRITE = 16,
};
const unsigned default_settings = \
AUTOMATIC_NEW_LINE_AFTER_OUTPUT | \
OUTPUT_AS_OBJECT | \
DISCARD_CRLF_IN_READ | \
AUTOMATIC_NEW_LINE_IN_WRITE;
unsigned settings = default_settings;

void release_rpn(RPN_EXP* rpn) {
	size_t offset = 0;
	for (USHORT element_index = 0; element_index != rpn->number_of_element; element_index++) {
		size_t this_size = wcslen(rpn->rpn + offset);
		delete[] (wchar_t*)(rpn->rpn + offset);
		offset += this_size;
	}
	delete rpn;
}

size_t current_instruction_index = 0;
Nesting** nesting = new Nesting*[128]; // maximum nesting depth : 128
USHORT nesting_ptr = 0;
File* files = new File[8]; // maximum 8 files opened at the same time
USHORT file_ptr = 0;

void strip(wchar_t* line) {
	size_t start = 0;
	size_t end = wcslen(line);
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
	memcpy(line, line + start, (end - start) * 2);
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

wchar_t* unsigned_to_string(size_t expr) {
	if (expr == 0) {
		wchar_t* result = new wchar_t[2];
		result[0] = L'0';
		result[1] = 0;
		return result;
	}
	size_t tag_expr = expr;
	size_t digit = 0;
	while (tag_expr >= 1) {
		digit++;
		tag_expr /= 10;
	}
	wchar_t* result = new wchar_t[digit + 1];
	result[digit] = 0;
	for (size_t index = digit - 1; index != -1; index--){
		result[index] = (expr % 10) + 48;
		expr /= 10;
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

long long match_keyword(wchar_t* expr, const wchar_t* keyword) {
	// find keyword in a string and returns its starting index
	if (wcslen(expr) < wcslen(keyword)) { return -1; }
	size_t upper_bound = wcslen(expr) - wcslen(keyword) + 1;
	size_t length = wcslen(keyword);
	for (size_t index = 0; index != upper_bound; index++) {
		if (expr[index] == keyword[0]) {
			bool matched = true;
			for (size_t index2 = 0; index2 != length; index2++) {
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

namespace DataType {
	inline Data* new_empty_data(Data* type_data); // forward declaration
	inline Data* copy(Data* original); // forward declaration
	inline void release_data(Data* data_in); // forward declaration
	class Any {
		// only used to check whether a data object is initialised
	public:
		bool init;
		Any() {
			this->init = false;
		}
	};
	class Integer {
	public:
		bool init;
		long long value;
		Integer() {
			this->init = false;
			this->value = 0;
		}
		Integer(wchar_t* expr) {
			this->init = true;
			this->value = (long long)string_to_real(expr);
		}
		Integer(long long value) {
			this->init = true;
			this->value = value;
		}
	};
	class Real {
	public:
		bool init;
		long double value;
		Real() {
			this->init = false;
			this->value = 0;
		}
		Real(wchar_t* expr) {
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
		Real(long double value) {
			this->init = true;
			this->value = value;
		}
		Real(Integer* int_obj) {
			this->init = true;
			this->value = (long double)int_obj->value;
		}
	};
	class Char {
	public:
		bool init;
		wchar_t value;
		Char() {
			this->init = false;
			this->value = 0;
		}
		Char(wchar_t* expr) {
			this->init = true;
			this->value = expr[1];
		}
		Char(wchar_t value) {
			this->init = true;
			this->value = value;
		}
	};
	class String {
	public:
		bool init;
		size_t length;
		wchar_t* string;
		static inline USHORT escape_character_in[3] = { 116, 110, 92 };
		static inline const wchar_t escape_character_out[3] = { L'\t', L'\n', L'\\' };
		String() {
			this->init = false;
			this->length = 0;
			this->string = nullptr;
		}
		String(size_t length, wchar_t* values) { // for internal creation
			this->init = true;
			if (wcslen(values) < length) {
				length = wcslen(values);
			}
			this->length = length;
			this->string = new wchar_t[length + 1];
			memcpy(this->string, values, (size_t)length * 2);
			this->string[length] = 0;
		}
		String(wchar_t* expr) { // created by evaluating string expression
			this->init = true;
			this->length = 0;
			this->string = new wchar_t[wcslen(expr) - 1];
			memcpy(this->string, expr + 1, (wcslen(expr) - 1) * 2);
			this->string[wcslen(expr) - 2] = 0;
			for (unsigned index = 0; this->string[index] != 0; index++) {
				if (this->string[index] == 92) {
					for (USHORT index2 = 0; index2 != 3; index2++) {
						if (this->string[index + 1] == escape_character_in[index2]) {
							this->string[index] = escape_character_out[index2];
							memcpy(this->string + index + 1, this->string + index + 2, (wcslen(this->string) - index - 1) * 2);
							break;
						}
					}
				}
				this->length++;
			}
		}
		String(Char* character) {
			this->init = true;
			this->length = 1;
			this->string = new wchar_t[1] {character->value};
		}
		~String() {
			delete[] this->string;
		}
		void assign(unsigned index, Char character) {
			USHORT value = character.value;
			this->string[index] = value;
		}
		wchar_t* read(unsigned index) {
			wchar_t* result = new wchar_t[2];
			result[0] = this->string[index];
			result[1] = 0;
			return result;
		}
		String* combine(String* right_operand) {
			size_t final_length = this->length + (*right_operand).length;
			wchar_t* final_string = new wchar_t[final_length + 1];
			memcpy(final_string, this->string, (size_t)this->length * 2);
			memcpy(final_string + this->length, (*right_operand).string, (size_t)(*right_operand).length * 2);
			final_string[final_length] = 0;
			return new String(final_length, final_string);
		}
		inline bool smaller(String* right_operand) {
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
		inline bool larger(String* right_operand) {
			return not smaller(right_operand);
		}
	};
	class Boolean {
	public:
		bool init;
		bool value;
		Boolean() {
			this->init = false;
			this->value = false;
		}
		Boolean(bool value) {
			this->init = true;
			this->value = value;
		}
	};
	class Date {
	public:
		bool init;
		USHORT year;
		USHORT month;
		USHORT day;
		USHORT hour;
		USHORT minute;
		USHORT second;
		Date() {
			this->init = false;
			this->year = this->month = this->day = this->hour = this->minute = this->second = 0;
		}
		Date(wchar_t* expr) {
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
		Date(USHORT second, USHORT minute, USHORT hour,
			USHORT day, USHORT month, USHORT year) {
			this->init = true;
			this->second = second;
			this->minute = minute;
			this->hour = hour;
			this->day = day;
			this->month = month;
			this->year = year;
		}
		Date* add(Date* offset) {
			Date* result = new Date(this->second, this->minute, this->hour,
				this->day, this->month, this->year);
			result->second += offset->second;
			result->minute += offset->minute;
			result->hour += offset->hour;
			while (result->second >= 60) {
				result->minute++; result->second -= 60;
			}
			while (result->minute >= 60) {
				result->hour++; result->minute -= 60;
			}
			while (result->hour >= 60) {
				result->day++; result->hour -= 60;
			}
			bool flag = false;
			while (!flag) {
				flag = true;
				switch (result->month) {
				case 1: case 3: case 5: case 7: case 8: case 10:
					if (result->day >= 32) {
						result->month++;
						result->day -= 32;
						flag = false;
					}
					break;
				case 4: case 6: case 9: case 11:
					if (result->day >= 31) {
						result->month++;
						result->day -= 31;
						flag = false;
					}
					break;
				case 2:
					if (result->year % 4 == 0) {
						if (result->day >= 30) {
							result->month++;
							result->day -= 30;
							flag = false;
						}
					}
					else {
						if (result->day >= 29) {
							result->month++;
							result->day -= 29;
							flag = false;
						}
					}
					break;
				case 12:
					if (result->day >= 32) {
						result->year++;
						result->month = 1;
						result->day -= 32;
						flag = false;
					}
					break;
				}
			}
			return result;
		}
		wchar_t* output() {
			wchar_t* result = new wchar_t[20];
			result[4] = result[7] = 95;
			result[10] = 43;
			result[13] = result[16] = 58;
			result[19] = 0;
			USHORT* ptr[] = { &this->year, &this->month, &this->day,
				&this->hour, &this->minute, &this->second };
			for (USHORT index = 0; index != 6; index++) {
				if (index == 0) {
					wchar_t* string = length_limiting(unsigned_to_string(*ptr[index]), 4);
					memcpy(result, string, 8);
					delete[] string;
				}
				else {
					wchar_t* string = length_limiting(unsigned_to_string(*ptr[index]), 2);
					memcpy(result + 2 + index * 3, string, 4);
					delete[] string;
				}
			}
			return result;
		}
	};
	class Array {
	public:
		bool init;
		Data** memory;
		USHORT* size;
		Data* type;
		USHORT* start_indexes;
		USHORT dimension;
		size_t total_size;
		Array() {
			this->init = false;
			this->size = nullptr;
			this->start_indexes = nullptr;
			this->type = 0;
			this->total_size = 0;
			this->dimension = 0;
			this->memory = nullptr;
		}
		Array(Data* type_data, USHORT* boundaries) {
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
					this->start_indexes[(USHORT)(index/2)] = boundaries[index];
					this->dimension++;
				}
				else {
					break;
				}
			}
			this->memory = new Data* [this->total_size];
			for (USHORT index = 0; index != this->total_size; index++) {
				this->memory[index] = new_empty_data(type_data);
				this->memory[index]->variable_data = true;
			}
		}
		Data* read(USHORT* indexes) {
			size_t offset = 0;
			for (USHORT index = 0; index != this->dimension; index++) {
				if (indexes[index] < this->start_indexes[index] or
					indexes[index] > this->start_indexes[index] + this->size[index]) {
					return new Data{ 65535, (void*)L"下标超出可接受范围" };
				}
				offset *= this->size[index];
				offset += ((size_t)indexes[index] - this->start_indexes[index]);
			}
			if (offset < total_size) {
				return this->memory[offset];
			}
			else {
				return new Data{ 65535, (void*)L"下标超出边界" };
			}
		}
	};
	class RecordType {
	public:
		bool init = true;
		USHORT number_of_fields;
		wchar_t** fields;
		Data* types;
		RecordType(BinaryTree* field_info) {
			this->number_of_fields = 0;
			BinaryTree::Node* all_fields = field_info->list_nodes(field_info->root, &this->number_of_fields);
			this->fields = new wchar_t* [this->number_of_fields];
			this->types = new Data[this->number_of_fields];
			for (USHORT index = 0; index != this->number_of_fields; index++) {
				this->fields[index] = all_fields[index].key;
				memcpy(this->types + index, all_fields[index].value, sizeof(Data));
			}
		}
	};
	class Record {
	public:
		bool init = true;
		RecordType* source;
		Data** values;
		Record() {
			this->init = false;
			this->source = nullptr;
			this->values = nullptr;
		}
		Record(RecordType* source) {
			this->source = source;
			USHORT number = source->number_of_fields;
			this->values = new Data* [number];
			for (USHORT index = 0; index != number; index++) {
				this->values[index] = new_empty_data(this->source->types + index);
				this->values[index]->variable_data = true;
			}
		}
		inline int find(wchar_t* key) {
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
		inline Data* read(USHORT index) {
			return this->values[index];
		}
	};
	class PointerType {
	public:
		bool init = true;
		Data* type;
		PointerType(Data* type) {
			this->type = type;
		}
		~PointerType() {
			if (this->type->type < 6) {
				delete this->type;
			}
		}
	};
	class Pointer {
		// Be aware that pointer is quite special in this interpreter
		// if multiple pointers have referenced to the same data unit,
		// each pointer will reference to next pointer (forward_link),
		// so dereference a pointer is a recursive process.
		// Also, if the data unit is not valid any more,
		// state of all the pointers referencing to it need to be changed
		// It is a iterative started from the last pointer referenced to it
		// because the data unit will have a reverse reference to it.
		// Thus, all the pointers related to the data unit and itself
		// forms a closed loop.
	public:
		bool init = false;
		bool valid = false;
		PointerType* source;
		BinaryTree::Node* node;
		Data* forward_link = nullptr;
		Data* backward_link = nullptr;
		Pointer() {
			this->source = nullptr;
			this->node = nullptr;
		}
		Pointer(PointerType* source) {
			this->source = source;
			this->node = nullptr; // validate by calling BinaryTree::new_pointer
		}
		inline void reference(BinaryTree::Node* target_node) {
			this->node = target_node;
			this->init = true;
			this->valid = true;
		}
		inline BinaryTree::Node* dereference() {
			if (this->valid == false) {
				return nullptr;
			}
			else {
				return this->node;
			}
		}
		static inline void add_link(BinaryTree::Node* referenced_node, Data* pointer) {
			Pointer& pointer_object = *(Pointer*)pointer->value;
			pointer_object.forward_link = referenced_node->last_pointer;
			if (referenced_node->last_pointer) {
				((Pointer*)referenced_node->last_pointer->value)->backward_link = pointer;
			}
			referenced_node->last_pointer = pointer;
		}
		static inline void remove_link(Data* pointer) {
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
		static inline void invalidate(BinaryTree::Node* node) {
			Data* current_pointer = node->last_pointer;
			if (current_pointer) {
				do {
					((Pointer*)current_pointer->value)->valid = false;
					current_pointer = ((Pointer*)current_pointer->value)->forward_link;
				} while (current_pointer);
			}
		}
	};
	class EnumeratedType {
	public:
		bool init = true;
		USHORT length;
		wchar_t** values = nullptr;
		EnumeratedType(wchar_t* expr) {
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
		static inline void add_constants(EnumeratedType* type, BinaryTree* enumerations) {
			for (USHORT index = 0; index != type->length; index++) {
				BinaryTree::Node* node = enumerations->find(type->values[index]);
				if (node) {
					delete node->value->value;
					node->value->value = new Integer(index);
				}
				else {
					Data* data = new Data{ 0, new Integer(index) };
					enumerations->insert(type->values[index], data);
				}
			}
		}
	};
	class Enumerated {
	public:
		bool init = false;
		EnumeratedType* source;
		USHORT current;
		Enumerated() {
			this->source = nullptr;
			this->current = 0;
		}
		Enumerated(EnumeratedType* source_in) {
			this->source = source_in;
			this->current = 0;
		}
		inline void assign(USHORT value) {
			this->current = value;
			this->init = true;
		}
		inline USHORT read() {
			return this->current;
		}
	};
	class Function {
	public:
		bool init;
		size_t start_line;
		USHORT number_of_args;
		Parameter* params;
		Data* return_type; // if it does not return, then return_type->type = 65535
		Function() {
			this->init = false;
			this->start_line = 0;
			this->number_of_args = 0;
			this->params = nullptr;
			this->return_type = nullptr;
		}
		Function(size_t start_line_in, USHORT number_of_args_in, Parameter* params_in,
			Data* return_type_in = new Data{ 65535, nullptr }) {
			this->init = true;
			this->start_line = start_line_in;
			this->number_of_args = number_of_args_in;
			this->params = params_in;
			this->return_type = return_type_in;
		}
		bool push_args(BinaryTree* locals, Data** args) {
			for (USHORT index = 0; index != this->number_of_args; index++) {
				if (not ((Any*)args[index]->value)->init) {
					wchar_t* error_message = new wchar_t[18 + (USHORT)log10(index)];
					memcpy(error_message, L"参数 ", 6);
					USHORT value = index + 1;
					USHORT offset = 0;
					while (value >= 1) {
						error_message[3 + offset] = value % 10 + 48;
						value /= 10;
						offset += 1;
					}
					memcpy(error_message + 4 + (USHORT)log10(index), L" 尚未初始化", 14);
					throw Error(ValueError, error_message);
				}
				if (this->params[index].type->type < 7) {
					Data* args_data = nullptr;
					if (args[index]->type != this->params[index].type->type) {
						switch ((args[index]->type << 8) + this->params[index].type->type) {
						case 1:
							args_data = new Data{ 1, new Real((Integer*)args[index]->value) };
							break;
						case 256:
							args_data = new Data{ 0, new Integer((long long)((Real*)args[index]->value)->value) };
							break;
						case 515:
							args_data = new Data{ 3, new String((Char*)args[index]->value) };
							break;
						case 770:
							if (((String*)args[index]->value)->length != 1) { return false; }
							args_data = new Data{ 2, new Char(((String*)args[index]->value)->string[0])};
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
					Data* args_data = DataType::copy(args[index]);
					args_data->variable_data = true;
					locals->insert(this->params[index].name, args_data);
				}
				if (not args[index]->variable_data) {
					DataType::release_data(args[index]);
				}
			}
			return true;
		}
	};
	class Address {
	public:
		bool init = true;
		void* value;
		Address(void* target) {
			this->value = target;
		}
	};
	Data* new_empty_data(Data* type_data) { // value of this structure represent source
		void* object = nullptr;
		switch(type_data->type){
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
			return new Data{ type_data->type, object };
		}
		else {
			return new Data{ (USHORT)(type_data->type + 1), object };
		}
	}
	Data* copy(Data* original) {
		Data* result = new Data{ original->type, nullptr };
		switch (original->type) {
		case 0:
			result->value = new Integer;
			memcpy(result->value, original->value, sizeof(Integer));
			break;
		case 1:
			result->value = new Real;
			memcpy(result->value, original->value, sizeof(Real));
			break;
		case 2:
			result->value = new Char;
			memcpy(result->value, original->value, sizeof(Char));
			break;
		case 3:
			result->value = new String;
			memcpy(result->value, original->value, sizeof(String));
			if (not ((DataType::String*)result->value)->init) { break; }
			((String*)result->value)->string = new wchar_t[((String*)original->value)->length + 1];
			memcpy(((String*)result->value)->string, ((String*)original->value)->string, (((String*)original->value)->length + 1) * 2);
			break;
		case 4:
		{
			result->value = new Boolean(*(DataType::Boolean*)original->value);
			break;
		}
		case 5:
			result->value = new Date;
			memcpy(result->value, original->value, sizeof(Date));
			break;
		case 6:
		{
			USHORT* start = new USHORT[10];
			memcpy(start, ((Array*)original->value)->start_indexes, sizeof(USHORT) * 10);
			USHORT* size = new USHORT[10];
			memcpy(size, ((Array*)original->value)->size, sizeof(USHORT) * 10);
			result->value = new Array;
			memcpy(result->value, original->value, sizeof(Array));
			((Array*)result->value)->start_indexes = start;
			((Array*)result->value)->size = size;
			((Array*)result->value)->memory = new Data * [((Array*)original->value)->total_size];
			for (size_t index = 0; index != ((Array*)original->value)->total_size; index++) {
				((Array*)result->value)->memory[index] = copy(((Array*)original->value)->memory[index]);
				((Array*)result->value)->memory[index]->variable_data = true;
			}
			((Array*)result->value)->type = new Data;
			memcpy(((Array*)result->value)->type, ((Array*)original->value)->type, sizeof(Data));
			break;
		}
		case 8:
		{
			result->value = new Record;
			memcpy(result->value, original->value, sizeof(Record));
			((Record*)result->value)->values = new Data* [((Record*)result->value)->source->number_of_fields];
			for (USHORT index = 0; index != ((Record*)result->value)->source->number_of_fields; index++) {
				((Record*)result->value)->values[index] = copy(((Record*)original->value)->values[index]);
				((Record*)result->value)->values[index]->variable_data = true;
			}
			break;
		}
		case 10:
		{
			Pointer* ptr = new Pointer;
			memcpy(ptr, original->value, sizeof(Pointer));
			ptr->forward_link = nullptr;
			ptr->backward_link = nullptr;
			result->value = ptr;
			if (ptr->valid and ptr->init) {
				ptr->reference(((Pointer*)original->value)->dereference());
				Pointer::add_link(ptr->node, result);
			}
			break;
		}
		case 12:
			result->value = new Enumerated;
			memcpy(result->value, original->value, sizeof(Enumerated));
			break;
		case 14:
			result->value = new Address(((Address*)original->value)->value);
			break;
		}
		return result;
	}
	inline Data* get_type(Data* data_in) {
		Data* type = new Data{ data_in->type, nullptr };
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
	Data* type_adaptation(Data* data_in, Data* target_type) {
		// original data structure is not being released
		USHORT& type_in = data_in->type;
		USHORT& type_out = target_type->type;
		if (type_in == type_out or type_in == 14 and type_out == 10) {
			return copy(data_in);
		}
		Data* data = nullptr;
		switch ((type_in << 8) + type_out) {
		case 1:
		{
			long long value = ((Integer*)(data_in->value))->value;
			data = new Data{ 1, new Real((long double)value) };
			break;
		}
		case 256:
		{
			long double value = ((Real*)(data_in->value))->value;
			data = new Data{ 0, new Integer((long long)value) };
			break;
		}
		case 515:
		{
			data = new Data{ 3, new String((Char*)data_in->value) };
			break;
		}
		case 770:
		{
			if (((String*)data_in->value)->length != 1) {
				return nullptr;
			}
			else {
				wchar_t value = ((String*)(data_in->value))->string[0];
				data = new Data{ 2, new Char(value) };
			}
			break;
		}
		case 12:
		{
			Enumerated* obj = new Enumerated((EnumeratedType*)target_type->value);
			obj->assign(((Integer*)data_in->value)->value);
			data = new Data{ 12, obj };
			break;
		}
		default:
		{
			return nullptr;
		}
		}
		return data;
	}
	inline void release_data(Data* data) {
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
	bool check_identical(Data* left, Data* right) {
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
	wchar_t* output_data(Data* data, DWORD* count_out) {
		if (not ((DataType::Any*)data->value)->init) {
			*count_out = 6;
			return new wchar_t[] { L"<未初始化>" };
		}
		switch (data->type) {
		case 0:
		{
			long long value = ((DataType::Integer*)data->value)->value;
			bool negative = value < 0;
			value = negative ? -value : value;
			unsigned length = (negative ? 1 : 0) + (unsigned)log10(abs(value)) + 1;
			wchar_t* string = new wchar_t[length * 2];
			if (negative) { string[0] = L'-'; }
			for (unsigned index = 0; index != length - 1 * negative; index++) {
				string[length - index - 1] = value % 10 + 48;
				value /= 10;
			}
			*count_out = (size_t)length;
			return string;
		}
		case 1:
		{
			long double value = ((DataType::Real*)data->value)->value;
			bool negative = value < 0;
			value = negative ? -value : value;
			long long whole_part = (long long)value;
			long double fraction_part = value - whole_part;
			unsigned length = (negative ? 1 : 0) + (unsigned)log10(whole_part) + 1;
			wchar_t* string = new wchar_t[length + 7 + (negative ? 1 : 0)];
			if (negative) { string[0] = 45; }
			for (unsigned index = 0; index != length - 1 * negative; index++) {
				string[length - index - 1] = whole_part % 10 + 48;
				whole_part /= 10;
			}
			string[length] = 46;
			for (USHORT index = 0; index != 6; index++) {
				USHORT this_value = (USHORT)(fraction_part * 10) % 10;
				string[length + index + 1] = (char)(this_value + 48);
				fraction_part = fraction_part * 10 - this_value;
			}
			*count_out = (size_t)length + 7;
			return string;
		}
		case 2:
		{
			wchar_t* buffer = new wchar_t[1];
			memcpy(buffer, &((DataType::Char*)data->value)->value, 2);
			*count_out = 1;
			return buffer;
		}
		case 3:
		{
			wchar_t* buffer = new wchar_t[((DataType::String*)data->value)->length];
			memcpy(buffer, ((DataType::String*)data->value)->string, ((DataType::String*)data->value)->length * 2);
			*count_out = ((DataType::String*)data->value)->length;
			return buffer;
		}
		case 4:
		{
			if (((DataType::Boolean*)data->value)->value) {
				*count_out = 4;
				return new wchar_t[] { L"TRUE" };
			}
			else {
				*count_out = 5;
				return new wchar_t[] { L"FALSE" };
			}
		}
		case 5:
		{
			wchar_t* expr = ((DataType::Date*)data->value)->output();
			*count_out = wcslen(expr);
			return expr;
		}
		default:
		{
			wchar_t* content = new wchar_t[] { L"<该数据仅能以对象形式输出>" };
			*count_out = 14;
			return content;
		}
		}
	}
	wchar_t* output_data_as_object(Data* data, DWORD* count_out) {
		if (not ((DataType::Any*)data->value)->init) {
			*count_out = 6;
			return new wchar_t[] { L"<未初始化>" };
		}
		switch (data->type) {
		case 0:
		{
			DWORD count = 0;
			wchar_t* content = output_data(data, &count);
			wchar_t* result_message = new wchar_t[count + 4];
			static const wchar_t* head = L"整数(";
			memcpy(result_message, head, 6);
			memcpy(result_message + 3, content, (size_t)count * 2);
			static const wchar_t tail = L')';
			result_message[count + 3] = tail;
			delete[] content;
			*count_out = count + 4;
			return result_message;
		}
		case 1:
		{
			DWORD count = 0;
			wchar_t* content = output_data(data, &count);
			wchar_t* result_message = new wchar_t[count + 5];
			static const wchar_t* head = L"浮点数(";
			memcpy(result_message, head, 8);
			memcpy(result_message + 4, content, count * 2);
			static const wchar_t tail = ')';
			result_message[count + 4] = tail;
			delete[] content;
			*count_out = count + 5;
			return result_message;
		}
		case 2:
		{
			DWORD count = 0;
			wchar_t* content = output_data(data, &count);
			wchar_t* result_message = new wchar_t[7];
			static const wchar_t* head = L"字符('";
			memcpy(result_message, head, 8);
			result_message[4] = content[0];
			static const wchar_t* tail = L"')";
			memcpy(result_message + 4 + count, tail, 4);
			delete[] content;
			*count_out = 7;
			return result_message;
		}
		case 3:
		{
			DWORD count = 0;
			wchar_t* content = output_data(data, &count);
			wchar_t* result_message = new wchar_t[count + 7];
			static const wchar_t* head = L"字符串(\"";
			memcpy(result_message, head, 10);
			memcpy(result_message + 5, content, count * 2);
			static const wchar_t* tail = L"\")";
			memcpy(result_message + 5 + count, tail, 4);
			delete[] content;
			*count_out = count + 7;
			return result_message;
		}
		case 4:
		{
			DWORD count = 0;
			wchar_t* content = output_data(data, &count);
			wchar_t* result_message = new wchar_t[count + 5];
			static const wchar_t* head = L"布尔值(";
			memcpy(result_message, head, 8);
			memcpy(result_message + 4, content, count * 2);
			static const wchar_t* tail = L")";
			memcpy(result_message + 4 + count, tail, 2);
			delete[] content;
			*count_out = count + 5;
			return result_message;
		}
		case 5:
		{
			DWORD count = 0;
			wchar_t* content = output_data(data, &count);
			wchar_t* result_message = new wchar_t[count + 7];
			static const wchar_t* head = L"日期与时间(";
			memcpy(result_message, head, 12);
			memcpy(result_message + 6, content, count * 2);
			static const wchar_t* tail = L")";
			memcpy(result_message + 6 + count, tail, 2);
			delete[] content;
			*count_out = count + 7;
			return result_message;
		}
		case 6:
		{
			wchar_t** ptr = new wchar_t*[((DataType::Array*)data->value)->total_size];
			DWORD* counts = new DWORD[((DataType::Array*)data->value)->total_size];
			DWORD total_count = 3;
			for (size_t index = 0; index != ((DataType::Array*)data->value)->total_size; index++) {
				ptr[index] = output_data_as_object(((DataType::Array*)data->value)->memory[index], &counts[index]);
				total_count += counts[index] + 2;
			}
			wchar_t* result_message = new wchar_t[total_count];
			static const wchar_t* head = L"数组[";
			memcpy(result_message, head, 6);
			DWORD offset = 3;
			for (LONG64 index = 0; index != ((DataType::Array*)data->value)->total_size; index++) {
				memcpy(result_message + offset, ptr[index], (size_t)counts[index] * 2);
				static const wchar_t* spliter = L", ";
				memcpy(result_message + offset + counts[index], spliter, 4);
				offset += counts[index] + 2;
				delete[] ptr[index];
			}
			static const wchar_t* tail = L"]";
			memcpy(result_message + offset - 2, tail, 2);
			delete[] ptr;
			*count_out = total_count;
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
			*count_out = total_count;
			return result_message;
		}
		case 8:
		{
			USHORT length = ((DataType::RecordType*)((DataType::Record*)data->value)->source)->number_of_fields;
			wchar_t** value_ptr = new wchar_t* [length];
			DWORD* value_lengths = new DWORD[length];
			size_t total_count = 4;
			for (USHORT index = 0; index != length; index++) {
				Data* value_data = ((DataType::Record*)data->value)->read(index);
				value_ptr[index] = output_data_as_object(value_data, value_lengths + index);
				total_count += wcslen(((DataType::RecordType*)((DataType::Record*)data->value)->source)->fields[index]) + value_lengths[index] + 3;
			}
			wchar_t* result_message = new wchar_t[total_count];
			static const wchar_t* head = L"结构体{";
			memcpy(result_message, head, 8);
			size_t offset = 4;
			for (USHORT index = 0; index != length; index++) {
				wchar_t*& field = ((DataType::RecordType*)((DataType::Record*)data->value)->source)->fields[index];
				size_t field_length = wcslen(field);
				memcpy(result_message + offset, field, field_length * 2);
				result_message[offset + field_length] = L'=';
				memcpy(result_message + offset + field_length + 1, value_ptr[index], value_lengths[index] * 2);
				static const wchar_t* spliter = L", ";
				memcpy(result_message + offset + field_length + 1 + value_lengths[index], spliter, 4);
				offset += field_length + value_lengths[index] + 3;
				delete[] value_ptr[index];
			}
			delete[] value_ptr;
			delete[] value_lengths;
			static const wchar_t tail = '}';
			memcpy(result_message + offset - 2, &tail, 2);
			*count_out = total_count;
			return result_message;
		}
		case 9:
		{
			DWORD count = 0;
			wchar_t* content = nullptr;
			switch (((DataType::PointerType*)data->value)->type->type) {
			case 0:
				count = 4;
				content = const_cast<wchar_t*>(L"整数类型");
				break;
			case 1:
				count = 5;
				content = const_cast<wchar_t*>(L"浮点数类型");
				break;
			case 2:
				count = 4;
				content = const_cast<wchar_t*>(L"字符类型");
				break;
			case 3:
				count = 5;
				content = const_cast<wchar_t*>(L"字符串类型");
				break;
			case 4:
				count = 5;
				content = const_cast<wchar_t*>(L"布尔值类型");
				break;
			case 5:
				count = 7;
				content = const_cast<wchar_t*>(L"日期与时间类型");
				break;
			case 6:
				count = 4;
				content = const_cast<wchar_t*>(L"数组类型");
				break;
			case 8:
				count = 5;
				content = const_cast<wchar_t*>(L"结构体类型");
				break;
			case 10:
				count = 4;
				content = const_cast<wchar_t*>(L"指针类型");
				break;
			case 12:
				count = 4;
				content = const_cast<wchar_t*>(L"枚举类型");
				break;
			}
			wchar_t* message = new wchar_t[count + 10];
			static const wchar_t* head = L"指针类型(目标: ";
			memcpy(message, head, 18);
			memcpy(message + 9, content, count * 2);
			static const wchar_t tail = L')';
			message[count + 9] = tail;
			*count_out = count + 10;
			return message;
		}
		case 10:
		{
			if (not ((Pointer*)data->value)->valid) {
				*count_out = 6;
				return new wchar_t[] { L"<指针无效>" };
			}
			DWORD count = wcslen(((Pointer*)data->value)->node->key);
			wchar_t* buffer = new wchar_t[count + 8];
			static const wchar_t* head = L"指针(目标: ";
			memcpy(buffer, head, 14);
			memcpy(buffer + 7, ((Pointer*)data->value)->node->key, (size_t)count * 2);
			static const wchar_t tail = L')';
			buffer[count + 7] = tail;
			*count_out = count + 8;
			return buffer;
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
				DWORD count = wcslen(terms[index]);
				memcpy(buffer + offset, terms[index], (size_t)count * 2);
				static const wchar_t* spliter = L", ";
				memcpy(buffer + offset + count, spliter, 4);
				offset += count + 2;
			}
			static const wchar_t tail = L')';
			buffer[total_count - 1] = tail;
			*count_out = total_count;
			return buffer;
		}
		case 12:
		{
			USHORT choice = ((DataType::Enumerated*)data->value)->current;
			wchar_t*& term = ((DataType::EnumeratedType*)((DataType::Enumerated*)data->value)->source)->values[choice];
			DWORD count = wcslen(term) + (USHORT)log10(choice) + 16;
			wchar_t* buffer = new wchar_t[count];
			static const wchar_t* head = L"枚举(值: ";
			memcpy(buffer, head, 12);
			size_t enumerated_string_length = (size_t)((USHORT)log10(choice)) + 1;
			if (choice == 0) {
				buffer[6] = L'0';
			}
			else {
				for (size_t index = 0; choice >= 1; index++) {
					buffer[index + 6] = choice % 10 + 48;
					choice /= 10;
				}
			}
			static const wchar_t* middle = L", 枚举常量: ";
			memcpy(buffer + 6 + enumerated_string_length, middle, 16);
			memcpy(buffer + 14 + enumerated_string_length, term, wcslen(term) * 2);
			static const wchar_t tail = L')';
			buffer[count - 1] = tail;
			*count_out = count;
			return buffer;
			break;
		}
		}
		return nullptr;
	}
	char* sequentialise(Data* data) {
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
	Data* desequentialise(char* digest) {
		Data* data = nullptr;
		switch (digest[0]) {
		case 0:
		{
			long long value = 0;
			memcpy(&value, digest + 1, sizeof(long long));
			data = new Data{ 0, new Integer(value) };
			break;
		}
		case 1:
		{
			long double value = 0;
			memcpy(&value, digest + 1, sizeof(long double));
			data = new Data{ 1, new Real(value) };
			break;
		}
		case 2:
		{
			wchar_t value = 0;
			memcpy(&value, digest + 1, sizeof(wchar_t));
			data = new Data{ 2, new Char(value) };
			break;
		}
		case 3:
		{
			size_t length = *(digest + 1);
			wchar_t* string = new wchar_t[length];
			memcpy(string, digest + 9, length * 2);
			data = new Data{ 3, new String(wcslen(string), string)};
			break;
		}
		case 4:
		{
			bool value = 0;
			memcpy(&value, digest + 1, sizeof(bool));
			data = new Data{ 4, new Boolean(value) };
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
			data = new Data{ 5, new Date(second, minute, hour, day, month, year) };
			break;
		}
		}
		return data;
	}
}

namespace Element {
	bool variable(wchar_t* expr) {
		if (not wcslen(expr)) { return false; }
		for (size_t index = 1; expr[index] != 0; index++) {
			if (not (expr[index] >= 65 and expr[index] <= 90 or
				expr[index] >= 97 and expr[index] <= 122 or
				expr[index] == 95 or
				expr[index] >= 48 and expr[index] <= 57)) {
				return false;
			}
		}
		if (not (expr[0] >= 65 and expr[0] <= 90 or
			expr[0] >= 97 and expr[0] <= 122 or
			expr[0] == 95)) {
			return false;
		}
		return true;
	}
	bool integer(wchar_t* expr) {
		if (not wcslen(expr)) { return false; }
		USHORT start = expr[0] == 43 or expr[0] == 45 ? 1 : 0;
		for (size_t index = start; expr[index] != 0; index++) {
			if (not (expr[index] >= 48 and expr[index] <= 57)) {
				return false;
			}
		}
		return true;
	}
	bool float_point_number(wchar_t* expr) {
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
	bool real(wchar_t* expr) {
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
	bool character(wchar_t* expr) {
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
	bool string(wchar_t* expr) {
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
	bool boolean(wchar_t* expr) {
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
	bool date(wchar_t* expr) {
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
						if (result < 0 or result >= 24){
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
	bool expression(wchar_t* expr, RPN_EXP** rpn_out = nullptr); // forward declaration
	bool valid_variable_path(wchar_t* expr); // forward declaration
	bool valid_indexes(wchar_t* expr) {
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
				bool match_result = expression(new_index) or integer(new_index);
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
	bool valid_array_element_access(wchar_t* expr, wchar_t** array_name = nullptr, wchar_t** indexes = nullptr) {
		for (size_t index = 0; expr[index] != 0; index++) {
			if (expr[index] == 91) {
				wchar_t* array_part = new wchar_t[index + 1];
				memcpy(array_part, expr, index * 2);
				array_part[index] = 0;
				if (variable(array_part) and valid_indexes(expr + index)) {
					if (array_name) { *array_name = array_part; }
					else { delete[] array_name; }
					if (indexes) {
						*indexes = new wchar_t[wcslen(expr) - index + 1];
						memcpy(*indexes, expr + index, (wcslen(expr) - index) * 2);
						(*indexes)[wcslen(expr) - index] = 0;
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
	bool valid_variable_path(wchar_t* expr) {
		if (variable(expr) or valid_array_element_access(expr)) {
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
						bool result = valid_variable_path(pointer_path) and valid_variable_path(pointer_path);
						delete[] pointer_path;
						delete[] other_path;
						return result;
					}
				}
				return false;
			}
			else { // in the form of ^x.x
				return valid_variable_path(expr + 1);
			}
		}
		size_t last_sign = 0;
		for (size_t index = 0; expr[index] != 0; index++) {
			if (expr[index] == 46) { // field
				expr[index] = 0;
				if (not (variable(expr + last_sign) or valid_array_element_access(expr + last_sign))) {
					expr[index] = 46;
					return false;
				}
				expr[index] = 46;
				last_sign = index + 1;
			}
		}
		return variable(expr + last_sign) or valid_array_element_access(expr + last_sign); // last field
	}
	bool addressing(wchar_t* expr) {
		if (not wcslen(expr)) { return false; }
		if (expr[0] == 64) {
			expr++;
			strip(expr);
			return variable(expr);
		}
		return false;
	}
	bool valid_operator(wchar_t character, USHORT* precedence_out = nullptr) {
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
	bool expression(wchar_t* expr, RPN_EXP** rpn_out) {
		if (not wcslen(expr)) { return false; }
		RPN_EXP* rpn_exp = new RPN_EXP;
		// phase 1: replacement of symbols, boolean constant
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
		size_t length = wcslen(expr);
		wchar_t* tag_expr = new wchar_t[length + 1];
		memcpy(tag_expr, expr, (length + 1) * 2);
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
			case 60: case 61: case 62:
				if (index < length - 1) { // compound logic operator
					switch ((tag_expr[index] << 8) + tag_expr[index + 1]) {
					case 15933: case 15421: case 15422:
						tag_expr[index] = tag_expr[index] + tag_expr[index + 1] - 120;
						memcpy(tag_expr + index + 1, tag_expr + index + 2, (wcslen(tag_expr) - index - 1) * 2);
						length--;
					}
				}
			}
		}
		// phase two: convert to RPN expression
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
				if (valid_operator(tag_expr[index], &tag_precedence)) {
					if (precedence <= tag_precedence and not bracket and not braces) { // index != 0 is used to identify negative sign before integer
						if (not ((tag_expr[index] == 43 or tag_expr[index] == 45) and index == 0)) {
							precedence = tag_precedence;
							operator_index = index;
						}
					}
				}
			}
		}
		if (bracket or braces) {
			return false; // brackets are not matched
		}
		if (not precedence) {
			if (bracket_exist) {
				if (tag_expr[0] == 40) { // is fully enclosed in brackets
					wchar_t* part_expr = new wchar_t[length - 1];
					memcpy(part_expr, tag_expr + 1, (size_t)(length - 2) * 2);
					part_expr[length - 2] = 0;
					RPN_EXP* part_rpn = new RPN_EXP{};
					bool result = expression(part_expr, &part_rpn);
					delete[] tag_expr;
					delete[] part_expr;
					delete rpn_exp;
					if (result) {
						if (rpn_out) { *rpn_out = part_rpn; }
						return true;
					}
					else {
						return false;
					}
				}
				else { // function calling
					size_t last_spliter = 0;
					USHORT number_of_bracket = 0;
					USHORT number_of_args = 0; // number of arguments passed to the function called
					RPN_EXP* final_rpn = new RPN_EXP; // storing RPN for final result
					size_t total_args_length = 0; // counting of every single character in combined rpn, including \0
					wchar_t* function_name = nullptr; // in the final stage it will be in a form of f/x, where x is the number of args
					for (size_t index = 0; tag_expr[index] != 0; index++) {
						if (tag_expr[index] == 40) {
							last_spliter = index;
							number_of_bracket++;
							if (number_of_bracket != 1) {
								continue;
							}
							function_name = new wchar_t[index + 1];
							memcpy(function_name, tag_expr, index * 2);
							function_name[index] = 0;
							if (not variable(function_name)) {
								delete[] tag_expr;
								delete[] function_name;
								return false;
							}
							if (tag_expr[index + 1] == 41) { // no argument
								wchar_t* final_function_name = new wchar_t[wcslen(function_name) + 3];
								memcpy(final_function_name, function_name, wcslen(function_name) * 2);
								final_function_name[wcslen(function_name)] = 9;
								final_function_name[wcslen(function_name) + 1] = L'0';
								final_function_name[wcslen(function_name) + 2] = 0;
								delete[] function_name;
								final_rpn->rpn = final_function_name;
								final_rpn->number_of_element = 1;
								if (rpn_out) { *rpn_out = final_rpn; }
								else { delete final_rpn; }
								return true;
							}
						}
						else if (tag_expr[index] == 44 or tag_expr[index] == 41) {
							wchar_t* this_arg = new wchar_t[index - last_spliter];
							memcpy(this_arg, tag_expr + last_spliter + 1, (index - last_spliter - 1) * 2);
							this_arg[index - last_spliter - 1] = 0;
							strip(this_arg);
							RPN_EXP* arg_rpn = new RPN_EXP; // storing RPN expression for current argument
							if (not expression(this_arg, &arg_rpn)) {
								delete[] tag_expr;
								delete[] this_arg;
								delete[] function_name;
								delete final_rpn;
								delete arg_rpn;
								return false;
							}
							size_t this_args_length = 0;
							for (USHORT element_index = 0; element_index != arg_rpn->number_of_element; element_index++) {
								this_args_length += wcslen(arg_rpn->rpn + this_args_length) + 1;
							}
							wchar_t* combined_rpn_string = new wchar_t[total_args_length + this_args_length];
							if (final_rpn->rpn) {
								memcpy(combined_rpn_string, final_rpn->rpn, total_args_length * 2);
								delete[] final_rpn->rpn;
							}
							memcpy(combined_rpn_string + total_args_length, arg_rpn->rpn, this_args_length * 2);
							total_args_length += this_args_length;
							final_rpn->rpn = combined_rpn_string;
							final_rpn->number_of_element += arg_rpn->number_of_element;
							delete[] arg_rpn->rpn;
							delete arg_rpn;
							number_of_args++;
							last_spliter = index;
							if (tag_expr[index] == 41) { // end of function calling
								if (number_of_bracket == 1) {
									wchar_t* final_number_of_args = unsigned_to_string(number_of_args);
									wchar_t* final_function_name = new wchar_t[wcslen(function_name) + 1 + wcslen(final_number_of_args) + 1];
									memcpy(final_function_name, function_name, wcslen(function_name) * 2);
									final_function_name[wcslen(function_name)] = 9;
									memcpy(final_function_name + wcslen(function_name) + 1, final_number_of_args, (wcslen(final_number_of_args) + 1) * 2);
									final_function_name[wcslen(function_name) + 1 + wcslen(final_number_of_args)] = 0;
									delete[] function_name;
									wchar_t* final_rpn_string = new wchar_t[total_args_length + wcslen(final_function_name) + 1];
									memcpy(final_rpn_string, final_rpn->rpn, total_args_length * 2);
									memcpy(final_rpn_string + total_args_length, final_function_name, (wcslen(final_function_name) + 1) * 2);
									delete[] final_rpn->rpn;
									final_rpn->rpn = final_rpn_string;
									final_rpn->number_of_element++;
									if (rpn_out) { *rpn_out = final_rpn; }
									else { delete final_rpn; }
									return true;
								}
								else {
									number_of_bracket--;
								}
							}
						}
					}
					return false;
				}
			}
			else { // single element
				if (integer(tag_expr) or real(tag_expr) or character(tag_expr) or string(tag_expr) or
					tag_expr[0] == 7 or tag_expr[0] == 8 or date(tag_expr) or valid_variable_path(tag_expr) or addressing(tag_expr)) {
					rpn_exp->rpn = tag_expr;
					rpn_exp->number_of_element = 1;
					if (rpn_out) { *rpn_out = rpn_exp; }
					else { delete rpn_exp; delete[] tag_expr; }
					return true;
				}
				else {
					return false;
				}
			}
		}
		else {
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
			if (operator_value == 6) { // one arity operator
				if (left_operand[0] != 0) {
					delete[] left_operand;
					delete[] right_operand;
					return false;
				}
			}
			bool result1 = expression(left_operand, &rpn_left);
			bool result2 = expression(right_operand, &rpn_right);
			delete[] tag_expr;
			if (not ((result1 and result2) or (operator_value == 6 and result2))) {
				delete[] left_operand;
				delete[] right_operand;
				if (result1) {
					delete rpn_left;
				}
				if (result2) {
					delete rpn_right;
				}
				return false;
			}
			else {
				size_t length_left = 0;
				if (result1) {
					left_operand = rpn_left->rpn;
					for (unsigned element_index = 0; element_index != rpn_left->number_of_element; element_index++) {
						length_left += wcslen(left_operand + length_left) + 1;
					}
				}
				right_operand = rpn_right->rpn;
				size_t length_right = 0;
				for (unsigned element_index = 0; element_index != rpn_right->number_of_element; element_index++) {
					length_right += wcslen(right_operand + length_right) + 1;
				}
				wchar_t* tag_rpn = new wchar_t[length_left + length_right + 2];
				if (result1) {
					memcpy(tag_rpn, left_operand, length_left * 2);
				}
				memcpy(tag_rpn + length_left, right_operand, length_right * 2);
				tag_rpn[length_left + length_right] = operator_value;
				tag_rpn[length_left + length_right + 1] = 0;
				rpn_exp->rpn = tag_rpn;
				if (result1) {
					rpn_exp->number_of_element = rpn_left->number_of_element + rpn_right->number_of_element + 1;
				}
				else {
					rpn_exp->number_of_element = rpn_right->number_of_element + 1;
				}
				if (rpn_out) { *rpn_out = rpn_exp; }
				else { delete rpn_exp; delete[] tag_rpn; }
				return true;
			}
		}
		return false;
	}
	bool fundamental_type(wchar_t* expr, USHORT* type = nullptr) {
		static const wchar_t* fundamentals[] = {L"INTEGER", L"REAL", L"CHAR", L"STRING", L"boolEAN", L"DATE"};
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
	bool array_type(wchar_t* expr, USHORT** boundaries_out = nullptr, wchar_t** type = nullptr) {
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
	bool pointer_type(wchar_t* expr) {
		if (expr[0] == 94) {
			expr++;
			strip(expr);
			return variable(expr);
		}
		return false;
	}
	bool enumerated_type(wchar_t* expr) {
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
	bool parameter_list(wchar_t* expr, Parameter** param_out = nullptr, USHORT* count_out = nullptr) {
		if (wcslen(expr) == 2) {
			if (expr[0] == 40 and expr[1] == 41) {
				if (param_out) { *param_out = new Parameter[0]; }
				if (count_out) { *count_out = 0; }
				return true;
			}
		}
		wchar_t* tag_expr = new wchar_t[wcslen(expr) + 1];
		memcpy(tag_expr, expr, (wcslen(expr) + 1) * 2);
		USHORT count = 1;
		for (size_t index = 0; tag_expr[index] != 0; index++) {
			if (tag_expr[index] == 44) {
				count++;
				if (tag_expr[index + 1] == 32) {
					memcpy(tag_expr + index + 1, tag_expr + index + 2, (wcslen(tag_expr) - index - 1) * 2);
				}
			}
		}
		size_t last_spliter = 0;
		size_t last_colon = 0;
		Parameter* params = new Parameter[count];
		count = 0;
		for (size_t index = 0; tag_expr[index] != 0; index++) {
			if (tag_expr[index] == 44 or tag_expr[index] == 41) {
				if (last_spliter == last_colon or last_spliter > last_colon) {
					for (USHORT arg_index = 0; arg_index != count; arg_index++) {
						delete[] params[arg_index].name;
						delete[] params[arg_index].type;
					}
					delete[] params;
					return false;
				}
				wchar_t* former_part = new wchar_t[last_colon - last_spliter];
				memcpy(former_part, tag_expr + last_spliter + 1, (last_colon - last_spliter - 1) * 2);
				former_part[last_colon - last_spliter - 1] = 0;
				strip(former_part);
				size_t start = 0; // start index of parameter name
				for (size_t index2 = 0; former_part[index2] != 0; index2++) {
					if (former_part[index2] == 32) {
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
				memcpy(type_part, tag_expr + last_colon + 1, (index - last_colon - 1) * 2);
				type_part[index - last_colon - 1] = 0;
				strip(type_part);
				bool valid = variable(parameter_name) and (variable(type_part) or array_type(type_part));
				static const wchar_t* keyword_ref = L"BYREF";
				static const wchar_t* keyword_value = L"BYVALUE";
				bool byref = false;
				if (keyword) {
					byref = match_keyword(keyword, keyword_ref) == 0 and wcslen(keyword) == wcslen(keyword_ref);
					valid = valid and (byref or match_keyword(keyword, keyword_value) == 0 and wcslen(keyword) == wcslen(keyword_value));
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
				if (tag_expr[index] == 41) { break; }
			}
			else if (tag_expr[index] == 58 and not last_colon) {
				last_colon = index;
			}
		}
		if (param_out) { *param_out = params; }
		if (count_out) { *count_out = count; }
		return true;
	}
}

namespace Construct {
	// enders can not be combined, see sequence_check
	RESULT* empty_line(wchar_t* expr) {
		RESULT* result = new RESULT;
		result->matched = true;
		result->tokens = new TOKEN[]{
			ENDTOKEN,
		};
		if (wcslen(expr) == 0) { return result; }
		for (size_t index = 0; expr[index] != 0; index++) {
			if (expr[index] != 32 and expr[index] != 9) {
				result->matched = false;
			}
		}
		return result;
	}
	RESULT* declaration(wchar_t* expr) {
		RESULT* result = new RESULT;
		if (match_keyword(expr, L"DECLARE") == 0) {
			USHORT count = 1;
			for (size_t index = 7; expr[index] != 0; index++) {
				if (expr[index] == 44) {
					count++;
				}
				else if (expr[index] == 58) {
					break;
				}
			}
			size_t last_spliter = 7;
			wchar_t** variables = new wchar_t* [count];
			TOKEN* variable_tokens = new TOKEN[count * 2];
			count = 0;
			for (size_t index = 7; expr[index] != 0; index++) {
				if (expr[index] == 44 or expr[index] == 58) {
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
							result->matched = true;
							result->args = new void* [] {
								(void*)L"V",
								new USHORT{ count },
								variables,
								type_part,
							};
							result->tokens = new TOKEN[3 + count * 2];
							result->tokens[0] = TOKEN{ 8, TOKENTYPE::Keyword };
							memcpy(result->tokens + 1, variable_tokens, sizeof(TOKEN) * (count * 2));
							result->tokens[1 + count * 2] = TOKEN{ (USHORT)(wcslen(expr) - index), TOKENTYPE::Type };
							result->tokens[2 + count * 2] = ENDTOKEN;
							delete[] variable_tokens;
						}
						else if (Element::array_type(type_part, &boundaries_out, &type_out)) {
							result->matched = true;
							delete[] type_part;
							result->args = new void* [] {
								(void*)L"A",
								new USHORT{ count },
								variables,
								boundaries_out,
								type_out,
							};
							
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
	RESULT* constant(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"CONSTANT ";
		if (match_keyword(expr, keyword) == 0) {
			for (size_t index = 9; expr[index] != 0; index++) {
				if (expr[index] == 8592) {
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
						result->matched = true;
						result->args = new void* [] {variable_part, rpn_out};
						result->tokens = new TOKEN[] {
							{ 8ui16, TOKENTYPE::Keyword },
							{ (USHORT)(index - 9ui16), TOKENTYPE::Variable },
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
	RESULT* type_header(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"TYPE ";
		if (match_keyword(expr, keyword) == 0) {
			wchar_t* type_part = new wchar_t[wcslen(expr) - 4];
			memcpy(type_part, expr + 5, (wcslen(expr) - 5) * 2);
			type_part[wcslen(expr) - 5] = 0;
			strip(type_part);
			if (Element::variable(type_part)) {
				result->matched = true;
				result->args = new void* [] { type_part };
			}
			else {
				delete[] type_part;
			}
		}
		return result;
	}
	RESULT* type_ender(wchar_t* expr) {
		RESULT* result = new RESULT;
		if (wcslen(expr) != 7) { return result; }
		static const wchar_t* keyword = L"ENDTYPE";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
		}
		return result;
	}
	RESULT* pointer_type_header(wchar_t* expr){
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"TYPE ";
		if (match_keyword(expr, keyword) == 0) {
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
						result->matched = true;
						result->args = new void* [] {
							type_part,
							pointer_part,
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
	RESULT* enumerated_type_header(wchar_t* expr){
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"TYPE ";
		if (match_keyword(expr, keyword) == 0) {
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
						result->matched = true;
						result->args = new void* [] {
							type_part,
							value_part,
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
	RESULT* assignment(wchar_t* expr) {
		RESULT* result = new RESULT;
		for (size_t index = 0; expr[index] != 0; index++) {
			if (expr[index] == 8592) {
				wchar_t* variable_part = new wchar_t[index + 1];
				wchar_t* expression_part = new wchar_t[wcslen(expr) - index];
				memcpy(variable_part, expr, index * 2);
				memcpy(expression_part, expr + index + 1, (wcslen(expr) - index - 1) * 2);
				variable_part[index] = 0;
				expression_part[wcslen(expr) - index - 1] = 0;
				strip(expression_part);
				strip(variable_part);
				RPN_EXP* rpn_out = nullptr;
				if (Element::expression(expression_part, &rpn_out) and Element::valid_variable_path(variable_part)) {
					delete[] expression_part;
					result->matched = true;
					result->args = new void* [] {
						variable_part,
						rpn_out,
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
	RESULT* output(wchar_t* expr) {
		RESULT* result = new RESULT;
		if (wcslen(expr) < 7) {
			return result;
		}
		static const wchar_t* keyword = L"OUTPUT";
		for (USHORT index = 0; index != 6; index++) {
			if (keyword[index] != expr[index]) {
				return result;
			}
		}
		wchar_t* expr_part = new wchar_t[wcslen(expr) - 6];
		memcpy(expr_part, expr + 7, (wcslen(expr) - 7) * 2);
		expr_part[wcslen(expr) - 7] = 0;
		RPN_EXP* rpn_out = nullptr;
		if (Element::expression(expr_part, &rpn_out)) {
			delete[] expr_part;
			result->matched = true;
			result->args = new void* [1] { rpn_out };
		}
		else {
			delete[] expr_part;
		}
		return result;
	}
	RESULT* input(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"INPUT ";
		if (match_keyword(expr, keyword) == 0) {
			if (Element::valid_variable_path(expr + 6)) {
				wchar_t* variable_part = new wchar_t[wcslen(expr) - 5];
				memcpy(variable_part, expr + 6, (wcslen(expr) - 6) * 2);
				variable_part[wcslen(expr) - 6] = 0;
				result->matched = true;
				result->args = new void* [1] {
					variable_part,
				};
			}
		}
		return result;
	}
	RESULT* if_header_1(wchar_t* expr) {
		// IF <condition>
		//    THEN ...
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"IF ";
		if (match_keyword(expr, keyword) == 0) {
			RPN_EXP* rpn_out = nullptr;
			if (Element::expression(expr + 3, &rpn_out)) {
				result->matched = true;
				if (nesting_ptr == 128) {
					release_rpn(rpn_out);
					result->error_message = L"嵌套深度达到上限";
				}
				else {
					nesting[nesting_ptr] = new Nesting(Nesting::IF, current_instruction_index);
					result->args = new void* [] {
						nesting[nesting_ptr],
							rpn_out,
					};
					nesting_ptr++;
				}
			}
		}
		return result;
	}
	RESULT* if_header_2(wchar_t* expr) {
		// IF <condition> THEN
		//     ...
		RESULT* result = new RESULT;
		static const wchar_t* keywords[] = { L"IF ", L" THEN" };
		for (USHORT index = 0; index != 3; index++) {
			if (keywords[0][index] != expr[index]) {
				return result;
			}
		}
		for (USHORT index = 0; index != 5; index++) {
			if (keywords[1][index] != expr[wcslen(expr) - 5 + index]) {
				return result;
			}
		}
		wchar_t* condition = new wchar_t[wcslen(expr) - 7];
		memcpy(condition, expr + 3, (wcslen(expr) - 8) * 2);
		condition[wcslen(expr) - 8] = 0;
		RPN_EXP* rpn_out = nullptr;
		bool is_expression = Element::expression(condition, &rpn_out);
		delete[] condition;
		if (is_expression) {
			result->matched = true;
			if (nesting_ptr == 128) {
				release_rpn(rpn_out);
				result->error_message = L"嵌套深度达到上限";
			}
			else {
				nesting[nesting_ptr] = new Nesting(Nesting::IF, current_instruction_index);
				nesting[nesting_ptr]->add_tag(current_instruction_index);
				result->args = new void* [] {
					nesting[nesting_ptr],
						rpn_out,
				};
				nesting_ptr++;
			}
		}
		return result;
	}
	RESULT* then_tag(wchar_t* expr) {
		// THEN
		//     <statements>
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"THEN";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (not nesting_ptr) {
				result->error_message = L"未找到THEN标签对应的语法头";
			}
			if (nesting[nesting_ptr - 1]->nest_type != Nesting::IF) {
				result->error_message = L"未找到THEN标签对应的语法头";
			}
			else if (nesting[nesting_ptr - 1]->tag_number != 1) {
				result->error_message = L"重定义THEN标签是非法的";
			}
			else {
				nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
				result->args = new void* [] {
					nesting[nesting_ptr - 1],
				};
			}
		}
		return result;
	}
	RESULT* else_tag(wchar_t* expr) {
		// ELSE
		//     <statements>
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"ELSE";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (not nesting_ptr) {
				result->error_message = L"未找到ELSE标签对应的IF头";
			}
			else if (nesting[nesting_ptr - 1]->tag_number == 1) {
				result->error_message = L"在定义ELSE标签前请先定义THEN标签";
			}
			else if (nesting[nesting_ptr - 1]->tag_number == 3) {
				result->error_message = L"重定义ELSE标签是非法的";
			}
			else {
				nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
				result->args = new void* [] {
					nesting[nesting_ptr - 1],
				};
			}
		}
		return result;
	}
	RESULT* case_of_header(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"CASE OF ";
		if (match_keyword(expr, keyword) == 0) {
			RPN_EXP* rpn_out = nullptr;
			if (Element::expression(expr + 8, &rpn_out)) {
				if (nesting_ptr == 128) {
					result->error_message = L"嵌套深度达到上限";
				}
				else {
					result->matched = true;
					nesting[nesting_ptr] = new Nesting(Nesting::CASE, current_instruction_index);
					nesting[nesting_ptr]->nest_info->case_of_info.number_of_values = 0;
					result->args = new void* [] {
						nesting[nesting_ptr],
							rpn_out,
					};
					nesting_ptr++;
				}
			}
		}
		return result;
	}
	RESULT* case_tag(wchar_t* expr) {
		RESULT* result = new RESULT;
		if (expr[wcslen(expr) - 1] == 58) {
			expr[wcslen(expr) - 1] = 0;
			RPN_EXP* rpn_out = nullptr;
			bool is_expression = Element::expression(expr, &rpn_out);
			expr[wcslen(expr) - 1] = 58;
			if (is_expression) {
				result->matched = true;
				if (not nesting_ptr) {
					release_rpn(rpn_out);
					result->error_message = L"未找到CASE分支对应的CASE OF头";
				}
				else if (nesting[nesting_ptr - 1]->nest_type != Nesting::CASE) {
					release_rpn(rpn_out);
					result->error_message = L"未找到CASE分支对应的CASE OF头";
				}
				else {
					nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
					RPN_EXP** new_rpn_list = new RPN_EXP * [nesting[nesting_ptr - 1]->tag_number - 1];
					memcpy(new_rpn_list, nesting[nesting_ptr - 1]->nest_info->case_of_info.values, sizeof(RPN_EXP*) * (size_t)(nesting[nesting_ptr - 1]->tag_number - 2));
					new_rpn_list[nesting[nesting_ptr - 1]->tag_number - 2] = rpn_out;
					nesting[nesting_ptr - 1]->nest_info->case_of_info.values = new_rpn_list;
					nesting[nesting_ptr - 1]->nest_info->case_of_info.number_of_values++;
					result->args = new void* [] {
						nesting[nesting_ptr - 1],
					};
				}
			}
		}
		return result;
	}
	RESULT* otherwise_tag(wchar_t* expr) {
		RESULT* result = new RESULT;
		if (wcslen(expr) != 9) { return result; }
		static const wchar_t* keyword = L"OTHERWISE";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (not nesting_ptr) {
				result->error_message = L"未找到OTHERWISE分支对应的CASE OF头";
			}
			else if (nesting[nesting_ptr - 1]->nest_type != Nesting::CASE) {
				result->error_message = L"未找到OTHERWISE分支对应的CASE OF头";
			}
			else if (nesting[nesting_ptr - 1]->tag_number == 1) {
				result->error_message = L"未定义任何分支的情况下定义OTHERWISE标签是非法的";
			}
			else {
				nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
				result->args = new void* [] {
					nesting[nesting_ptr - 1],
				};
			}
		}
		return result;
	}
	RESULT* for_header_1(wchar_t* expr) {
		// FOR <variable> ← <lower_bound> TO <upper_bound>
		RESULT* result = new RESULT;
		static const wchar_t* keywords[] = { L"FOR ", L"←", L" TO " };
		long long positions[3] = {};
		for (USHORT index = 0; index != 3; index++) {
			positions[index] = match_keyword(expr, keywords[index]);
			if (positions[index] == -1) {
				return result;
			}
		}
		if (positions[0] != 0) {
			return result;
		}
		wchar_t* variable_part = new wchar_t[positions[1] - 4];
		wchar_t* lower_bound = new wchar_t[positions[2] - positions[1]];
		wchar_t* upper_bound = new wchar_t[wcslen(expr) - positions[2] - 3];
		memcpy(variable_part, expr + 4, (positions[1] - 5) * 2);
		memcpy(lower_bound, expr + positions[1] + 1, (positions[2] - positions[1] - 1) * 2);
		memcpy(upper_bound, expr + positions[2] + 4, (wcslen(expr) - positions[2] - 4) * 2);
		variable_part[positions[1] - 5] = 0;
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
			if (rpn_lower_bound) { release_rpn(rpn_lower_bound); }
			if (rpn_upper_bound) { release_rpn(rpn_upper_bound); }
		}
		else {
			result->matched = true;
			if (nesting_ptr == 128) {
				result->error_message = L"嵌套深度达到上限";
			}
			else {
				nesting[nesting_ptr] = new Nesting(Nesting::FOR, current_instruction_index);
				nesting[nesting_ptr]->nest_info->for_info.init = false;
				nesting[nesting_ptr]->nest_info->for_info.step = nullptr;
				result->args = new void* [] {
					nesting[nesting_ptr],
					variable_part,
					rpn_lower_bound,
					rpn_upper_bound,
				};
				nesting_ptr++;
			}
		}
		return result;
	}
	RESULT* for_header_2(wchar_t* expr) {
		// FOR <variable> ← <lower_bound> TO <upper_bound> STEP <step>
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L" STEP ";
		long long position = match_keyword(expr, keyword);
		if (position != -1) {
			wchar_t* former_part = new wchar_t[position + 1];
			memcpy(former_part, expr, position * 2);
			former_part[position] = 0;
			RPN_EXP* rpn_out = nullptr;
			if (not Element::expression(expr + position + 6, &rpn_out)) {
				delete[] former_part;
				return result;
			}
			RESULT* former_result = for_header_1(former_part);
			if (not former_result->matched) {
				delete[] former_part;
				delete rpn_out;
				delete former_result;
				return result;
			}
			result->matched = true;
			nesting[nesting_ptr - 1]->nest_info->for_info.step = rpn_out;
			result->args = former_result->args;
			delete former_result;
		}
		return result;
	}
	RESULT* for_ender(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"NEXT ";
		if (match_keyword(expr, keyword) == 0) {
			if (Element::valid_variable_path(expr + 5)) {
				result->matched = true;
				if (not nesting_ptr) {
					result->error_message = L"未找到NEXT标签对应的FOR头";
				}
				else if (nesting[nesting_ptr - 1]->nest_type != Nesting::FOR) {
					result->error_message = L"未找到NEXT标签对应的FOR头";
				}
				else {
					nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
					result->args = new void* [] {
						nesting[nesting_ptr - 1],
						expr + 5,
					};
					nesting_ptr--;
				}
			}
		}
		return result;
	}
	RESULT* while_header(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keywords[] = { L"WHILE ", L" DO" };
		long long positions[2] = {};
		positions[0] = match_keyword(expr, keywords[0]);
		positions[1] = match_keyword(expr, keywords[1]);
		if (positions[0] != 0) { return result; }
		if (positions[1] != (long long)wcslen(expr) - 3) { return result; }
		wchar_t* condition = new wchar_t[wcslen(expr) - 8];
		memcpy(condition, expr + 6, (wcslen(expr) - 9) * 2);
		condition[wcslen(expr) - 9] = 0;
		RPN_EXP* rpn_out = nullptr;
		bool matched_result = Element::expression(condition, &rpn_out);
		delete[] condition;
		if (matched_result) {
			result->matched = true;
			if (nesting_ptr == 128) {
				result->error_message = L"嵌套深度达到上限";
				return result;
			}
			nesting[nesting_ptr] = new Nesting(Nesting::WHILE, current_instruction_index);
			result->args = new void* [] {
				nesting[nesting_ptr],
				rpn_out,
			};
			nesting_ptr++;
		}
		return result;
	}
	RESULT* repeat_header(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"REPEAT";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (nesting_ptr == 128) {
				result->error_message = L"嵌套深度达到上限";
				return result;
			}
			nesting[nesting_ptr] = new Nesting(Nesting::REPEAT, current_instruction_index);
			result->args = new void* [] {
				nesting[nesting_ptr],
			};
			nesting_ptr++;
		}
		return result;
	}
	RESULT* repeat_ender(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"UNTIL ";
		if (match_keyword(expr, keyword) == 0) {
			RPN_EXP* rpn_out = nullptr;
			if (Element::expression(expr + 6, &rpn_out)) {
				result->matched = true;
				if (not nesting_ptr) {
					result->error_message = L"未找到UNTIL标签对应的REPEAT头";
				}
				else if (nesting[nesting_ptr - 1]->nest_type != Nesting::REPEAT) {
					result->error_message = L"未找到UNTIL标签对应的REPEAT头";
				}
				else {
					nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
					result->args = new void* [] {
						nesting[nesting_ptr - 1],
							rpn_out,
					};
					nesting_ptr--;
				}
			}
		}
		return result;
	}
	RESULT* ender(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keywords[] = { L"ENDIF", L"ENDCASE", L"ENDWHILE", L"ENDTRY"};
		for (USHORT index = 0; index != 4; index++) {
			if (match_keyword(expr, keywords[index]) == 0){
				result->matched = true;
				switch (index) {
				case 0:
				{
					if (not nesting_ptr) {
						result->error_message = L"未找到ENDIF标签对应的IF头";
						return result;
					}
					else if (nesting[nesting_ptr - 1]->nest_type != Nesting::IF) {
						result->error_message = L"未找到ENDIF标签对应的IF头";
						return result;
					}
					break;
				}
				case 1:
				{
					if (not nesting_ptr) {
						result->error_message = L"未找到ENDCASE标签对应的CASE OF头";
						return result;
					}
					else if (nesting[nesting_ptr - 1]->nest_type != Nesting::CASE) {
						result->error_message = L"未找到ENDCASE标签对应的CASE OF头";
						return result;
					}
					break;
				}
				case 2:
				{
					if (not nesting_ptr) {
						result->error_message = L"未找到ENDWHILE标签对应的WHILE头";
						return result;
					}
					else if (nesting[nesting_ptr - 1]->nest_type != Nesting::WHILE) {
						result->error_message = L"未找到ENDWHILE标签对应的WHILE头";
						return result;
					}
					break;
				}
				case 3:
				{
					if (not nesting_ptr) {
						result->error_message = L"未找到ENDTRY标签对应的TRY头";
						return result;
					}
					else if (nesting[nesting_ptr - 1]->nest_type != Nesting::TRY) {
						result->error_message = L"未找到ENDTRY标签对应的TRY头";
						return result;
					}
					break;
				}
				}
				nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
				result->args = new void* [] {
					new USHORT{index},
					nesting[nesting_ptr - 1]
				};
				nesting_ptr--;
			}
		}
		return result;
	}
	RESULT* procedure_header(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"PROCEDURE ";
		if (match_keyword(expr, keyword) == 0 and expr[wcslen(expr) - 1] == 41) {
			for (size_t index = 0; expr[index] != 0; index++) {
				if (expr[index] == 40) {
					wchar_t* function_name = new wchar_t[index - 9];
					memcpy(function_name, expr + 10, (index - 10) * 2);
					function_name[index - 10] = 0;
					bool valid_function_name = Element::variable(function_name);
					if (not valid_function_name) {
						delete[] function_name;
						return result;
					}
					wchar_t* parameter_string = new wchar_t[wcslen(expr) - index + 1];
					memcpy(parameter_string, expr + index, (wcslen(expr) - index) * 2);
					parameter_string[wcslen(expr) - index] = 0;
					Parameter* params = nullptr;
					USHORT number_of_args = 0;
					bool valid_parameter_list = Element::parameter_list(parameter_string, &params, &number_of_args);
					delete[] parameter_string;
					if (not valid_parameter_list) {
						delete[] function_name;
						return result;
					}
					result->matched = true;
					if (nesting_ptr == 128) {
						result->error_message = L"嵌套深度达到上限";
						delete[] function_name;
						for (USHORT arg_index = 0; arg_index != number_of_args; arg_index++) {
							delete[] params[arg_index].name;
							delete[] params[arg_index].type;
						}
						return result;
					}
					nesting[nesting_ptr] = new Nesting(Nesting::PROCEDURE, current_instruction_index);
					result->args = new void* [] {
						nesting[nesting_ptr],
						function_name,
						new USHORT{ number_of_args },
						params,
					};
					nesting_ptr++;
					break;
				}
			}
		}
		return result;
	}
	RESULT* procedure_ender(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"ENDPROCEDURE";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (not nesting_ptr) {
				result->error_message = L"未找到ENDPROCEDURE标签对应的PROCEDURE头";
			}
			else if (nesting[nesting_ptr - 1]->nest_type != Nesting::PROCEDURE) {
				result->error_message = L"未找到ENDPROCEDURE标签对应的PROCEDURE头";
			}
			else {
				nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
				result->args = new void* [] {
					nesting[nesting_ptr - 1],
				};
				nesting_ptr--;
			}
		}
		return result;
	}
	RESULT* function_header(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"FUNCTION ";
		if (match_keyword(expr, keyword) == 0) {
			static const wchar_t* return_keyword = L" RETURNS ";
			long long position = match_keyword(expr, return_keyword);
			for (size_t index = 9; expr[index] != 0; index++) {
				if (expr[index] == 40) {
					wchar_t* function_name = new wchar_t[index - 8];
					wchar_t* parameter_part = new wchar_t[position - index + 1];
					wchar_t* return_type = new wchar_t[wcslen(expr) - position - 8];
					memcpy(function_name, expr + 9, (index - 9) * 2);
					memcpy(parameter_part, expr + index, (position - index) * 2);
					memcpy(return_type, expr + position + 9, (wcslen(expr) - position - 9) * 2);
					function_name[index - 9] = 0;
					parameter_part[position - index] = 0;
					return_type[wcslen(expr) - position - 9] = 0;
					Parameter* params = nullptr;
					USHORT number_of_args = 0;
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
						result->matched = true;
						if (nesting_ptr == 128) {
							result->error_message = L"嵌套深度达到上限";
							delete[] function_name;
							for (USHORT arg_index = 0; arg_index != number_of_args; arg_index++) {
								delete[] params[arg_index].name;
								delete[] params[arg_index].type;
							}
							return result;
						}
						nesting[nesting_ptr] = new Nesting(Nesting::FUNCTION, current_instruction_index);
						result->args = new void* [] {
							nesting[nesting_ptr],
							function_name,
							new USHORT{ number_of_args },
							params,
							return_type,
						};
						nesting_ptr++;
					}
					else {
						delete[] function_name;
						delete[] return_type;
					}
					return result;
				}
			}
		}
		return result;
	}
	RESULT* function_ender(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"ENDFUNCTION";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (not nesting_ptr) {
				result->error_message = L"未找到ENDFUNCTION标签对应的FUNCTION头";
			}
			else if (nesting[nesting_ptr - 1]->nest_type != Nesting::FUNCTION) {
				result->error_message = L"未找到ENDFUNCTION标签对应的FUNCTION头";
			}
			else {
				nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
				result->args = new void* [] {
					nesting[nesting_ptr - 1],
				};
				nesting_ptr--;
			}
		}
		return result;
	}
	RESULT* continue_tag(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"CONTINUE";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (not nesting_ptr) {
				result->error_message = L"未找到CONTINUE标签对应的FOR, WHILE或REPEAT头";
			}
			else {
				for (USHORT depth = nesting_ptr - 1;; depth--) {
					if (nesting[depth]->nest_type == Nesting::FOR or
						nesting[depth]->nest_type == Nesting::WHILE or
						nesting[depth]->nest_type == Nesting::REPEAT) {
						result->args = new void* [] {
							nesting[depth]
						};
						return result;
					}
					if (depth == 0) { break; }
				}
				result->error_message = L"未找到CONTINUE标签对应的FOR, WHILE或REPEAT头";
			}
		}
		return result;
	}
	RESULT* break_tag(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"BREAK";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (not nesting_ptr) {
				result->error_message = L"未找到BREAK标签对应的FOR, WHILE或REPEAT头";
			}
			else {
				for (USHORT depth = nesting_ptr - 1;; depth--) {
					if (nesting[depth]->nest_type == Nesting::FOR or
						nesting[depth]->nest_type == Nesting::WHILE or
						nesting[depth]->nest_type == Nesting::REPEAT) {
						result->args = new void* [] {
							nesting[depth]
						};
						return result;
					}
					if (depth == 0) { break; }
				}
				result->error_message = L"未找到BREAK标签对应的FOR, WHILE或REPEAT头";
			}
		}
		return result;
	}
	RESULT* return_statement(wchar_t* expr) {
		RESULT* result = new RESULT;
		if (match_keyword(expr, L"RETURN") == 0) {
			if (wcslen(expr) > 6) {
				if (expr[6] != L' ') {
					return result;
				}
			}
			result->matched = true;
			for (USHORT depth = nesting_ptr - 1;; depth--) {
				if (nesting[depth]->nest_type == Nesting::PROCEDURE or
					nesting[depth]->nest_type == Nesting::FUNCTION) {
					if (wcslen(expr) > 7) {
						RPN_EXP* rpn_out = nullptr;
						if (Element::expression(expr + 7, &rpn_out)) {
							result->args = new void* [] {
								new bool{ true },
								nesting[nesting_ptr - 1],
								rpn_out,
							};
							return result;
						}
					}
					else {
						result->args = new void* [] {
							new bool{ false },
							nesting[nesting_ptr - 1],
						};
					}
				}
				if (depth == 0) { break; }
			}
			result->error_message = L"未找到RETURN标签对应的PROCEDRUE或FUNCTION头";
		}
		return result;
	}
	RESULT* try_header(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"TRY";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (nesting_ptr == 128) {
				result->error_message = L"嵌套深度达到上限";
			}
			else {
				nesting[nesting_ptr] = new Nesting(Nesting::TRY, current_instruction_index);
				result->args = new void* [] {
					nesting[nesting_ptr],
				};
				nesting_ptr++;
			}
		}
		return result;
	}
	RESULT* except_tag(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"EXCEPT";
		if (match_keyword(expr, keyword) == 0) {
			result->matched = true;
			if (not nesting_ptr) {
				result->error_message = L"未找到与EXCEPT标签对应的TRY头";
			}
			else if (nesting[nesting_ptr - 1]->nest_type != Nesting::TRY) {
				result->error_message = L"未找到与EXCEPT标签对应的TRY头";
			}
			else if (nesting[nesting_ptr - 1]->tag_number != 1) {
				result->error_message = L"重定义EXCEPT标签是非法的";
			}
			else {
				nesting[nesting_ptr - 1]->add_tag(current_instruction_index);
				result->args = new void* [] {
					nesting[nesting_ptr - 1],
				};
			}
		}
		return result;
	}
	RESULT* openfile_statement(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword_openfile = L"OPENFILE ";
		if (match_keyword(expr, keyword_openfile) == 0) {
			static const wchar_t* keyword_for = L" FOR ";
			long long position = match_keyword(expr + 9, keyword_for);
			if (position == -1) {
				return result;
			}
			else {
				wchar_t* filename_string = new wchar_t[position + 1];
				memcpy(filename_string, expr + 9, position * 2);
				filename_string[position] = 0;
				RPN_EXP* rpn_out = nullptr;
				bool match_result = Element::expression(filename_string, &rpn_out);
				delete[] filename_string;
				if (match_result) {
					static const wchar_t* modes[] = { L"READ", L"WRITE", L"APPEND" };
					for (USHORT mode_index = 0; mode_index != 3; mode_index++) {
						if (match_keyword(expr + position + 5, modes[mode_index])) {
							result->matched = true;
							result->args = new void* [] {
								rpn_out,
								new USHORT{mode_index},
							};
							return result;
						}
					}
				}
			}
		}
		return result;
	}
	RESULT* readfile_statement(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"READFILE ";
		if (match_keyword(expr, keyword) == 0) {
			for (size_t index = 9; expr[index] != 0; index++) {
				if (expr[index] == 44) {
					wchar_t* filename = new wchar_t[index - 8];
					wchar_t* variable_path = new wchar_t[wcslen(expr) - index];
					memcpy(filename, expr + 9, (index - 9) * 2);
					memcpy(variable_path, expr + index + 1, (wcslen(expr) - index - 1) * 2);
					filename[index - 9] = 0;
					variable_path[wcslen(expr) - index - 1] = 0;
					strip(filename);
					strip(variable_path);
					RPN_EXP* rpn_out = nullptr;
					bool match_result = Element::expression(filename, &rpn_out) and Element::valid_variable_path(variable_path);
					delete[] filename;
					if (match_result) {
						result->matched = true;
						result->args = new void* [] {
							rpn_out,
							variable_path,
						};
					}
					else {
						if (rpn_out) {
							release_rpn(rpn_out);
						}
						delete[] variable_path;
					}
					return result;
				}
			}
		}
		return result;
	}
	RESULT* writefile_statement(wchar_t* expr) {
		RESULT* result = new RESULT;
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
						result->matched = true;
						result->args = new void* [] {
							rpn_out_filename,
							rpn_out_message,
						};
						return result;
					}
					else {
						if (rpn_out_filename) {
							release_rpn(rpn_out_filename);
						}
						if (rpn_out_message) {
							release_rpn(rpn_out_message);
						}
					}
					return result;
				}
			}
		}
		return result;
	}
	RESULT* closefile_statement(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"CLOSEFILE";
		if (match_keyword(expr, keyword) == 0) {
			RPN_EXP* rpn_out = nullptr;
			if (Element::expression(expr + 10, &rpn_out)) {
				result->matched = true;
				result->args = new void* [] {
					rpn_out,
				};
			}
		}
		return result;
	}
	RESULT* seek_statement(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"SEEK ";
		if (match_keyword(expr, keyword) == 0) {
			for (size_t index = 5; expr[index] != 0; index++) {
				if (expr[index] == 44) {
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
					bool match_result = Element::expression(filename_string, &rpn_out_filename) and\
						Element::expression(address_expression, &rpn_out_address);
					delete[] filename_string;
					if (match_result) {
						result->matched = true;
						result->args = new void* [] {
							rpn_out_filename,
							rpn_out_address
						};
						return result;
					}
					else {
						if (rpn_out_filename) {
							release_rpn(rpn_out_filename);
						}
						if (rpn_out_address) {
							release_rpn(rpn_out_address);
						}
					}
				}
			}
		}
		return result;
	}
	RESULT* getrecord_statement(wchar_t* expr) {
		RESULT* result = new RESULT;
		static const wchar_t* keyword = L"GETRECORD ";
		if (match_keyword(expr, keyword) == 0) {
			for (size_t index = 10; expr[index] != 0; index++) {
				if (expr[index] == 44) {
					wchar_t* filename = new wchar_t[index - 8];
					wchar_t* variable_path = new wchar_t[wcslen(expr) - index];
					memcpy(filename, expr + 9, (index - 9) * 2);
					memcpy(variable_path, expr + index + 1, (wcslen(expr) - index - 1) * 2);
					filename[index - 9] = 0;
					variable_path[wcslen(expr) - index - 1] = 0;
					strip(filename);
					strip(variable_path);
					RPN_EXP* rpn_out = nullptr;
					bool match_result = Element::expression(filename, &rpn_out) and Element::valid_variable_path(variable_path);
					delete[] filename;
					if (match_result) {
						result->matched = true;
						result->args = new void* [] {
							rpn_out,
								variable_path,
						};
					}
					else {
						if (rpn_out) {
							release_rpn(rpn_out);
						}
						delete[] variable_path;
					}
					return result;
				}
			}
		}
		return result;
	}
	RESULT* putrecord_statement(wchar_t* expr) {
		RESULT* result = new RESULT;
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
						result->matched = true;
						result->args = new void* [] {
							rpn_out_filename,
								rpn_out_message,
						};
						return result;
					}
					else {
						if (rpn_out_filename) {
							release_rpn(rpn_out_filename);
						}
						if (rpn_out_message) {
							release_rpn(rpn_out_message);
						}
					}
					return result;
				}
			}
		}
		return result;
	}
	RESULT* single_expression(wchar_t* expr) {
		RESULT* result = new RESULT;
		RPN_EXP* rpn_out = nullptr;
		if (Element::expression(expr, &rpn_out)) {
			result->matched = true;
			result->args = new void* [] {
				rpn_out,
			};
		}
		return result;
	}
	void* constructs[] = { empty_line, declaration, constant, type_header, type_ender, pointer_type_header,
		enumerated_type_header, assignment, output, input, if_header_1, if_header_2, then_tag,
		else_tag, case_of_header, case_tag, otherwise_tag, for_header_1, for_header_2, for_ender,
		while_header, repeat_header, repeat_ender, ender, procedure_header, procedure_ender,
		function_header, function_ender, continue_tag, break_tag, return_statement, try_header,
		except_tag, openfile_statement, readfile_statement, writefile_statement, closefile_statement,
		seek_statement, getrecord_statement, putrecord_statement, single_expression };
	USHORT number_of_constructs = sizeof(constructs) / sizeof(void*);
	CONSTRUCT parse(wchar_t* line) {
		for (USHORT index = 0; index != number_of_constructs; index++) {
			RESULT* result = ((RESULT* (*)(wchar_t*))constructs[index])(line);
			if (result->matched) {
				return CONSTRUCT{ index, result };
			}
			else {
				delete result;
			}
		}
		return CONSTRUCT{ NULL, nullptr };
	}
}
