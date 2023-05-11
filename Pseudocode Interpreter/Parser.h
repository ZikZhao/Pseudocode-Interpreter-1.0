#pragma once
#include "IndexedList.h"
#ifdef TRY
#undef TRY
#endif
#define ENDTOKEN { 0, TOKENTYPE::Null }

extern LONGLONG current_instruction_index;

void strip(wchar_t* line); // remove spaces before and after text

long double string_to_real(wchar_t* expr); // convert numerial string into real

wchar_t* unsigned_to_string(unsigned number); // convert unsigned int into string

wchar_t* length_limiting(wchar_t* expr, size_t length_target); // format string into specific length

size_t skip_string(wchar_t* expr); // skip a string in the parsing of an expression

long match_keyword(wchar_t* expr, const wchar_t* keyword); // find keyword in a line of code

// type of token
enum TOKENTYPE {
	Null,
	Punctuation,
	Keyword,
	Variable,
	Type,
	Subroutine,
	Expression,
	Integer,
	String,
	Enumeration,
	Parameter,
	PassingMode,
	Comment,
};

// each single element in a line
struct TOKEN {
	USHORT length;
	TOKENTYPE type;
};

// contains indentation, token lists and comment information
struct ADVANCED_TOKEN {
	USHORT indentation; // number of indentations
	TOKEN* tokens; // array of tokens
	UINT comment; // start of comment in character units
};

// result of a construct matching
struct RESULT {
	bool matched = false; // whether current line matches with the syntax
	void** args = nullptr; // arguments to be passed to execution functions
	TOKEN* tokens = nullptr; // positions of elements used to highlight grammar
};

// structure that represents a data in pseudocode
struct DATA {
	USHORT type = 65535;
	void* value = nullptr;
	bool variable_data = false;
};

// structure that stores a reverse polish notation expression
struct RPN_EXP {
	wchar_t* rpn = nullptr;
	USHORT number_of_element = 0;
};

// structure that represent a parameter of a function
struct PARAMETER {
	wchar_t* name = nullptr;
	union {
		wchar_t* type_string = nullptr;
		DATA* type;
	};
	bool passed_by_ref = false;
};

void release_rpn(RPN_EXP* rpn_exp);

// a whole structure representing a construct
struct CONSTRUCT {
	USHORT syntax_index;
	RESULT result;
	~CONSTRUCT(); // forward declaration
	void release_tokens();
};

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
extern unsigned settings;

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
	Error(ErrorType error_type_in, const wchar_t* error_message_in);
};

// data structure used to store variable data
class BinaryTree {
public:
	// remind:
	// node						pointer of BinaryTree::Node struct
	// node->value				pointer of DATA struct
	// node->value->value		pointer of instance of class in DataType namespace (naturally void pointer)
	struct Node {
		wchar_t* key = nullptr;
		DATA* value = nullptr;
		bool constant = false;
		Node* left = nullptr;
		Node* right = nullptr;
		DATA* last_pointer = nullptr; // see DataType::Pointer
	};
	Node* root = nullptr;
	USHORT number = 0;
	size_t error_handling_indexes[128]{};
	USHORT error_handling_ptr = 0;
	void insert(wchar_t* key, DATA* value, bool constant = false);
	Node* find(wchar_t* key);
	Node* list_nodes(Node* current, USHORT* out_number = nullptr);
protected:
	void clear(Node* current);
public:
	void clear();
};

namespace DataType {
	class Any {
		// only used to check whether a data object is initialised
	public:
		bool init;
		Any();
	};
	class Integer {
	public:
		bool init;
		long long value;
		Integer();
		Integer(wchar_t* expr);
		Integer(long long value);
	};
	class Real {
	public:
		bool init;
		long double value;
		Real();
		Real(wchar_t* expr);
		Real(long double value);
		Real(Integer* int_obj);
	};
	class Char {
	public:
		bool init;
		wchar_t value;
		Char();
		Char(wchar_t* expr);
		Char(wchar_t value);
	};
	class String {
	public:
		bool init;
		size_t length;
		wchar_t* string;
		static inline USHORT escape_character_in[3] = { L't', L'n', L'\\' };
		static inline const wchar_t escape_character_out[3] = { L'\t', L'\n', L'\\' };
		String();
		String(size_t length, wchar_t* values);
		String(wchar_t* expr);
		String(Char* character);
		~String();
		void assign(unsigned index, Char character);
		wchar_t* read(unsigned index);
		String* combine(String* right_operand);
		bool smaller(String* right_operand);
		bool larger(String* right_operand);
	};
	class Boolean {
	public:
		bool init;
		bool value;
		Boolean();
		Boolean(bool value);
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
		Date();
		Date(wchar_t* expr);
		Date(USHORT second, USHORT minute, USHORT hour,
			USHORT day, USHORT month, USHORT year);
		Date* add(Date* offset);
		wchar_t* output();
	};
	class Array {
	public:
		bool init;
		DATA** memory;
		USHORT* size;
		DATA* type;
		USHORT* start_indexes;
		USHORT dimension;
		size_t total_size;
		Array();
		Array(DATA* type_data, USHORT* boundaries);
		DATA* read(USHORT* indexes);
	};
	class RecordType {
	public:
		bool init = true;
		USHORT number_of_fields;
		wchar_t** fields;
		DATA* types;
		RecordType(BinaryTree* field_info);
	};
	class Record {
	public:
		bool init = true;
		RecordType* source;
		DATA** values;
		Record();
		Record(RecordType* source);
		int find(wchar_t* key);
		DATA* read(USHORT index);
	};
	class PointerType {
	public:
		bool init = true;
		DATA* type;
		PointerType(DATA* type);
		~PointerType();
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
		DATA* forward_link = nullptr;
		DATA* backward_link = nullptr;
		Pointer();
		Pointer(PointerType* source);
		void reference(BinaryTree::Node* target_node);
		BinaryTree::Node* dereference();
		static void add_link(BinaryTree::Node* referenced_node, DATA* pointer);
		static void remove_link(DATA* pointer);
		static void invalidate(BinaryTree::Node* node);
	};
	class EnumeratedType {
	public:
		bool init = true;
		USHORT length;
		wchar_t** values = nullptr;
		EnumeratedType(wchar_t* expr);
		static void add_constants(EnumeratedType* type, BinaryTree* enumerations);
	};
	class Enumerated {
	public:
		bool init = false;
		EnumeratedType* source;
		USHORT current;
		Enumerated();
		Enumerated(EnumeratedType* source_in);
		void assign(USHORT value);
		USHORT read();
	};
	class Function {
	public:
		bool init;
		size_t start_line;
		USHORT number_of_args;
		PARAMETER* params;
		DATA* return_type; // if it does not return, then return_type->type = 65535
		Function();
		Function(size_t start_line_in, USHORT number_of_args_in, PARAMETER* params_in,
			DATA* return_type_in = new DATA{ 65535, nullptr });
		bool push_args(BinaryTree* locals, DATA** args);
	};
	class Address {
	public:
		bool init = true;
		void* value;
		Address(void* target);
	};
	DATA* new_empty_data(DATA* type_data);
	DATA* copy(DATA* original);
	DATA* get_type(DATA* data_in);
	DATA* type_adaptation(DATA* data_in, DATA* target_type);
	void release_data(DATA* data);
	bool check_identical(DATA* left, DATA* right);
	wchar_t* output_data(DATA* data, DWORD* count_out);
	wchar_t* output_data_as_object(DATA* data, DWORD* count_out);
	char* sequentialise(DATA* data);
	DATA* desequentialise(char* digest);
}

namespace Element {
	bool variable(wchar_t* expr);
	bool integer(wchar_t* expr);
	bool float_point_number(wchar_t* expr);
	bool real(wchar_t* expr);
	bool character(wchar_t* expr);
	bool string(wchar_t* expr);
	bool boolean(wchar_t* expr);
	bool date(wchar_t* expr);
	bool valid_indexes(wchar_t* expr);
	bool valid_array_element_access(wchar_t* expr, wchar_t** array_name = nullptr, wchar_t** indexes = nullptr);
	bool valid_variable_path(wchar_t* expr);
	bool addressing(wchar_t* expr);
	bool valid_operator(wchar_t character, USHORT* precedence_out = nullptr);
	bool expression(wchar_t* expr, RPN_EXP** rpn_out = nullptr);
	bool fundamental_type(wchar_t* expr, USHORT* type = nullptr);
	bool array_type(wchar_t* expr, USHORT** boundaries_out = nullptr, wchar_t** type = nullptr);
	bool pointer_type(wchar_t* expr);
	bool enumerated_type(wchar_t* expr);
	bool parameter_list(wchar_t* expr, PARAMETER** param_out = nullptr, USHORT* count_out = nullptr);
}

namespace Construct {
	// enders can not be combined, see SequenceCheck
	RESULT empty_line(wchar_t* expr);
	RESULT declaration(wchar_t* expr);
	RESULT constant(wchar_t* expr);
	RESULT type_header(wchar_t* expr);
	RESULT type_ender(wchar_t* expr);
	RESULT pointer_type_header(wchar_t* expr);
	RESULT enumerated_type_header(wchar_t* expr);
	RESULT assignment(wchar_t* expr);
	RESULT output(wchar_t* expr);
	RESULT input(wchar_t* expr);
	RESULT if_header_1(wchar_t* expr);
	RESULT if_header_2(wchar_t* expr);
	RESULT then_tag(wchar_t* expr);
	RESULT else_tag(wchar_t* expr);
	RESULT if_ender(wchar_t* expr);
	RESULT case_of_header(wchar_t* expr);
	RESULT case_tag(wchar_t* expr);
	RESULT otherwise_tag(wchar_t* expr);
	RESULT case_of_ender(wchar_t* expr);
	RESULT for_header_1(wchar_t* expr);
	RESULT for_header_2(wchar_t* expr);
	RESULT for_ender(wchar_t* expr);
	RESULT while_header(wchar_t* expr);
	RESULT while_ender(wchar_t* expr);
	RESULT repeat_header(wchar_t* expr);
	RESULT repeat_ender(wchar_t* expr);
	RESULT procedure_header(wchar_t* expr);
	RESULT procedure_ender(wchar_t* expr);
	RESULT function_header(wchar_t* expr);
	RESULT function_ender(wchar_t* expr);
	RESULT continue_tag(wchar_t* expr);
	RESULT break_tag(wchar_t* expr);
	RESULT return_statement(wchar_t* expr);
	RESULT try_header(wchar_t* expr);
	RESULT except_tag(wchar_t* expr);
	RESULT try_ender(wchar_t* expr);
	RESULT openfile_statement(wchar_t* expr);
	RESULT readfile_statement(wchar_t* expr);
	RESULT writefile_statement(wchar_t* expr);
	RESULT closefile_statement(wchar_t* expr);
	RESULT seek_statement(wchar_t* expr);
	RESULT getrecord_statement(wchar_t* expr);
	RESULT putrecord_statement(wchar_t* expr);
	RESULT single_expression(wchar_t* expr);
	// if new constructs are added, add deconstruction to struct CONSTRUCT
	static void* constructs[] = {
		empty_line, declaration, constant, type_header, type_ender, pointer_type_header, enumerated_type_header,
		assignment, output, input, if_header_1, if_header_2, then_tag, else_tag, if_ender, case_of_header,
		case_tag, otherwise_tag, case_of_ender, for_header_1, for_header_2, for_ender, while_header, while_ender,
		repeat_header, repeat_ender, procedure_header, procedure_ender, function_header, function_ender,
		continue_tag, break_tag, return_statement, try_header, except_tag, try_ender, openfile_statement,
		readfile_statement, writefile_statement, closefile_statement, seek_statement, getrecord_statement,
		putrecord_statement, single_expression
	};
	enum construct_index {
		_empty_line, _declaration, _constant, _type_header, _type_ender, _pointer_type_header, _enumerated_type_header,
		_assignment, _output, _input, _if_header_1, _if_header_2, _then_tag, _else_tag, _if_ender, _case_of_header,
		_case_tag, _otherwise_tag, _case_of_ender, _for_header_1, _for_header_2, _for_ender, _while_header, _while_ender,
		_repeat_header, _repeat_ender, _procedure_header, _procedure_ender, _function_header, _function_ender,
		_continue_tag, _break_tag, _return_statement, _try_header, _except_tag, _try_ender, _openfile_statement,
		_readfile_statement, _writefile_statement, _closefile_statement, _seek_statement, _getrecord_statement,
		_putrecord_statement, _single_expression
	};
	static USHORT number_of_constructs = sizeof(constructs) / sizeof(void*);
	CONSTRUCT* parse(wchar_t* line);
}

// describe nesting structure in pseudocode
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
		struct CASE_OF_INFO { // used by CASE OF
			USHORT number_of_values;
			RPN_EXP** values;
			bool otherwise_defined;
		} case_of_info;
		struct FOR_INFO { // used by FOR
			bool init;
			DATA* upper_bound;
			RPN_EXP* step;
		} for_info;
	} *nest_info;
	USHORT tag_number;
	LONGLONG* line_numbers;
	Nesting(NestType type, LONGLONG first_tag_line_number);
	~Nesting();
	void add_tag(LONGLONG new_tag_line_number);
};

// file object
class File {
public:
	HANDLE handle;
	wchar_t* filename;
	USHORT mode;
	bool eof = false;
	DWORD size;
	static inline const size_t size_of_record[] = {
		sizeof(long long), sizeof(long double), sizeof(wchar_t), 0, sizeof(bool), sizeof(size_t) };
	File();
	File(HANDLE handle_in, wchar_t* filename_in, USHORT mode_in);
	wchar_t* readline(bool retain_new_line);
	bool writeline(wchar_t* message, bool new_line, bool flush);
	void putrecord(char* digest);
	char* getrecord();
	void closefile();
	void seek(size_t address);
};

struct BREAKPOINT {
	unsigned long long line_index = 0;
	unsigned long type = 0;
	BREAKPOINT* dependency = nullptr;
	bool valid = true;
};

struct CALLFRAME {
	LONGLONG line_number;
	wchar_t* name;
	BinaryTree* local_variables;
};

struct CALLSTACK {
	CALLFRAME* call;
	USHORT ptr;
};
