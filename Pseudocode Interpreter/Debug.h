#pragma once
// used as a protocol definition when the program is run in debug mode

// signal types
#define SIGNAL_CONNECTION 1 // used to establish a connection
#define SIGNAL_BREAKPOINT 2 // used to manipulate breakpoints
#define SIGNAL_EXECUTION 4 // used to control execution (by single step, or force executing to specific line)
#define SIGNAL_INFORMATION 8 // used to transfer information of execution

// sub-states
#define CONNECTION_PIPE 1 // LPARAM: handle to send signal back to parent process
#define CONNECTION_START 2
#define CONNECTION_EXIT 1

#define BREAKPOINT_ADD 1 // LPARAM: line index
#define BREAKPOINT_DELETE 2 // LPARAM: line index
#define BREAKPOINT_HIT 4 // LPARAM: line index
#define BREAKPOINT_TYPE_NORMAL 1
#define BREAKPOINT_TYPE_CONDITION 2
#define BREAKPOINT_TYPE_TRACE 4
#define BREAKPOINT_TYPE_TEMPORARY 8
#define BREAKPOINT_TYPE_DEPENDENT 16

#define EXECUTION_CONTINUE 1
#define EXECUTION_STEPIN 2
#define EXECUTION_STEPOVER 4
#define EXECUTION_STEPOUT 8
#define EXECUTION_FORCE_SET 16
#define EXECUTION_STEPPED 1 // LPARAM: line index

#define INFORMATION_VARIABLE_TABLE_REQUEST 1 // returns pointer to global variable binary tree
#define INFORMATION_CALLING_STACK_REQUEST 2
#define INFORMATION_VARIABLE_TABLE_RESPONSE 1 // LPARAM: pointer to the binary tree representing variable table
#define INFORMATION_CALLING_STACK_RESPONSE 2 // LPARAM: pointer to the calling stack struct