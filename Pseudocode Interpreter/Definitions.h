#pragma once
// definitions for all the structs and classes used in interpreter and interprocess connection
#ifdef TRY
#undef TRY
#endif
#define ENDTOKEN { 0, PUNCTUATION }

enum TOKENTYPE {
	PUNCTUATION,
	KEYWORD,
	VARIABLE,
	TYPE,
	OPERATOR,
	SUBROUTINE,
	EXPRESSION,
};

struct TOKEN {
	USHORT length;
	TOKENTYPE type;
};

struct RESULT { // result of a construct matching
	bool matched = false;
	void** args = nullptr; // arguments to be passed to execution functions
	TOKEN* tokens = nullptr; // positions of elements used to highlight grammar
	const wchar_t* error_message = nullptr; // when syntax is matched but error may be caused, this field is used
};

struct Data {
	USHORT type = 65535;
	void* value = nullptr;
	bool variable_data = false;
};

struct RPN_EXP {
	wchar_t* rpn = nullptr;
	USHORT number_of_element = 0;
};

struct Parameter {
	wchar_t* name = nullptr;
	union {
		wchar_t* type_string = nullptr;
		Data* type;
	};
	bool passed_by_ref = false;
};

struct CONSTRUCT {
	USHORT syntax_index;
	RESULT* result;
};

class BinaryTree {
public:
	// remind:
	// node						pointer of BinaryTree::Node struct
	// node->value				pointer of Data struct
	// node->value->value		pointer of instance of class in DataType namespace (naturally void pointer)
	struct Node {
		wchar_t* key = nullptr;
		Data* value = nullptr;
		bool constant = false;
		Node* left = nullptr;
		Node* right = nullptr;
		Data* last_pointer = nullptr; // see DataType::Pointer
	};
	Node* root = nullptr;
	USHORT number = 0;
	size_t error_handling_indexes[128]{};
	USHORT error_handling_ptr = 0;
	BinaryTree() {};
	void insert(wchar_t* key, Data* value, bool constant = false) {
		Node* node = new Node{ key, value, constant };
		if (this->root == nullptr) {
			this->root = node;
		}
		else {
			Node* previous = nullptr;
			Node* current = root;
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
	Node* find(wchar_t* key) {
		if (this->root == nullptr or key == nullptr) { return nullptr; }
		Node* current = root;
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
	Node* list_nodes(Node* current, USHORT* out_number = nullptr) {
		USHORT left_number = 0;
		USHORT right_number = 0;
		Node* left_nodes = nullptr;
		Node* right_nodes = nullptr;
		if (current->left) {
			left_nodes = this->list_nodes(current->left, &left_number);
		}
		if (current->right) {
			right_nodes = this->list_nodes(current->right, &right_number);
		}
		Node* result = new Node[left_number + right_number + 1];
		memcpy(result, current, sizeof(Node));
		memcpy(result + 1, left_nodes, sizeof(Node) * left_number);
		memcpy(result + 1 + left_number, right_nodes, sizeof(Node) * right_number);
		if (out_number) {
			*out_number = 1 + left_number + right_number;
		}
		return result;
	}
protected:
	void clear(Node* current) {
		if (current->left) {
			this->clear(current->left);
		}
		if (current->right) {
			this->clear(current->right);
		}
		delete current;
	}
public:
	void clear() {
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
};

class Nesting {
public:
	enum NestType {
		NotInitialised,
		IF,
		CASE,
		WHILE,
		FOR,
		REPEAT,
		TRY,
		PROCEDURE,
		FUNCTION,
	} nest_type;
	union Info {
		struct { // used by CASE OF
			USHORT number_of_values;
			RPN_EXP** values;
		} case_of_info;
		struct FOR_INFO { // used by FOR
			bool init;
			Data* upper_bound;
			RPN_EXP* step;
		} for_info;
	};
	USHORT tag_number;
	size_t* line_numbers;
	Info* nest_info = nullptr;
	Nesting() {
		this->nest_type = NotInitialised;
		this->tag_number = 0;
		this->line_numbers = nullptr;
	}
	Nesting(NestType type, size_t first_tag_line_number) {
		this->nest_type = type;
		this->tag_number = 1;
		this->line_numbers = new size_t[1]{ first_tag_line_number };
		if (this->nest_type == CASE or this->nest_type == FOR or
			this->nest_type == PROCEDURE or this->nest_type == FUNCTION) {
			this->nest_info = new Info;
		}
	}
	inline void add_tag(size_t new_tag_line_number) {
		size_t* temp_line_numbers = new size_t[this->tag_number + 1];
		memcpy(temp_line_numbers, this->line_numbers, sizeof(size_t) * this->tag_number);
		temp_line_numbers[this->tag_number] = new_tag_line_number;
		delete[] this->line_numbers;
		this->line_numbers = temp_line_numbers;
		this->tag_number++;
	}
};

class File {
public:
	HANDLE handle;
	wchar_t* filename;
	USHORT mode;
	bool eof = false;
	DWORD size;
	static inline const size_t size_of_record[] = {
		sizeof(long long), sizeof(long double), sizeof(wchar_t), 0, sizeof(bool), sizeof(size_t) };
	File() {
		this->handle = nullptr;
		this->filename = nullptr;
		this->mode = 0;
		this->size = 0;
	}
	File(HANDLE handle_in, wchar_t* filename_in, USHORT mode_in) {
		this->handle = handle_in;
		this->filename = filename_in;
		this->mode = mode_in;
		GetFileSize(this->handle, &this->size);
	}
	wchar_t* readline(bool retain_new_line) {
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
	bool writeline(wchar_t* message, bool new_line, bool flush) {
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
	inline void putrecord(char* digest) {
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
	char* getrecord() {
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
	inline void closefile() {
		delete[] this->filename;
		CloseHandle(this->handle);
	}
	void seek(size_t address) {
		LARGE_INTEGER offset{};
		offset.QuadPart = address;
		LARGE_INTEGER position{};
		SetFilePointerEx(this->handle, offset, &position, FILE_BEGIN);
		if ((size_t)position.QuadPart == (size_t)this->size - 1) {
			this->eof = true;
		}
	}
};

struct BREAKPOINT {
	unsigned long long line_index = 0;
	unsigned long type = 0;
	BREAKPOINT* dependency = nullptr;
	bool valid = true;
};

struct CALLFRAME {
	size_t line_number;
	wchar_t* name;
	BinaryTree* local_variables;
};

struct CallStack {
	CALLFRAME* call;
	USHORT ptr;
};

enum ErrorType {
	NoError,
	SyntaxError,
	VariableError,
	EvaluationError,
	TypeError,
	ArgumentError,
	ValueError,
};

class Error {
public:
	ErrorType error_type;
	const wchar_t* error_message;
	Error(ErrorType error_type_in, const wchar_t* error_message_in) {
		this->error_type = error_type_in;
		this->error_message = error_message_in;
	}
};