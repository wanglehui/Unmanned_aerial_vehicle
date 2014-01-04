// common.h

#ifndef _COMMON_H_
#define _COMMON_H_

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <errno.h>
	#include <math.h>
	#include <sys/types.h>
	#include <unistd.h>

	typedef unsigned char uint8_t;
	typedef short unsigned int uint16_t;
	typedef unsigned int uint32_t;
	typedef unsigned long uint64_t;

	#define return_enum_string(ENUM_CODE) case ENUM_CODE: return #ENUM_CODE
	#define return_custom_enum_string(ENUM_CODE, STRING) case ENUM_CODE: return STRING
	
	#define LENGTH_MISURE_UNIT	"meters"
	#define ANGLE_MISURE_UNIT	"degrees"
	#define TIME_MISURE_UNIT	"seconds"
	#define METERS_TO_FEETS_FACTOR 3.2808399
	
	//#define INTERACTIVE
	#define DEBUG
	//#define DO_NOT_COMUNICATE //to be removed
	
#endif
