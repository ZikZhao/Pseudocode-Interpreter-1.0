#pragma once
// Used as a protocol definition when the program is run in debug mode

// pipe messages
#define PM_CONNECTION 1
#define PM_BREAKPOINT 2
#define PM_EXECUTION_SPEED 3
#define PM_VARIABLE_TABLE 4
#define PM_CALLING_STACK 5
#define PM_FORCE_EXECUTION 6
#define PM_STEP 7

// sub-states
#define CONNECTION_PIPE 1
#define CONNECTION_START 2
#define CONNECTION_EXIT 3

#define BREAKPOINT_ADD 1
#define BREAKPOINT_UPDATE 2
#define BREAKPOINT_DELETE 3
#define BREAKPOINT_CLEAR 4
#define BREAKPOINT_TYPE_NORMAL 1
#define BREAKPOINT_TYPE_CONDITION 2
#define BREAKPOINT_TYPE_TRACE 3
#define BREAKPOINT_TYPE_TEMPORARY 4
#define BREAKPOINT_TYPE_DEPENDENT 5

#define EXECUTION_SPEED_SET 1
#define EXECUTION_SPEED_RESET 2

#define VARIABLE_TABLE_REQUEST 1

#define CALLING_STACK_REQUEST 1

#define FORCE_EXECUTION_SET 1

#define STEP_BY_LINE 1
#define STEP_BY_PROCEDURE 2
#define STEP_OUT 3

// breakpoint struct
struct BREAKPOINT {
	unsigned long long line_index = 0;
	unsigned long type = 0;
	BREAKPOINT* dependency = nullptr;
	bool valid = true;
};