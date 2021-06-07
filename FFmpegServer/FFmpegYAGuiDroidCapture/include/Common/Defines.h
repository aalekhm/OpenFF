#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <assert.h>

#define NOT !
#define DEFAULT_WIDTH	480
#define DEFAULT_HEIGHT	320

enum DURATION_TYPE
{
	nanoseconds = 0,
	microseconds,
	milliseconds,
	seconds,
	minutes,
	hours
};

#define TIMER_START_NEW(__identifier__) \
	Timer __identifier__; \
	TIMER_START(__identifier__); \

#define TIMER_START(__identifier__) __identifier__.start();
#define TIMER_STOP(__identifier__) __identifier__.stop();

#define ASSERT(__message__, __iReturnValue__) \
std::cout << ##__message__ << __iReturnValue__ << std::endl; \
assert(__iReturnValue__ > 0);

#define LOG_TAG "[C++]"
#define LOG_CONSOLE(__string__) std::cout << LOG_TAG << " " << __string__ << "\n";

// Object deletion macro
#define SAFE_DELETE(x) if(x != NULL) { delete x; x = NULL; }

// Array deletion macro
#define SAFE_DELETE_ARRAY(x) if(x != NULL) { delete[] x; x = NULL; }

// Ref cleanup macro
#define SAFE_RELEASE(x)	if(x != NULL) { (x)->release(); x = NULL; }

static GLenum __gl_error_code;
typedef GLuint TextureHandle;

#ifdef NDEBUG
#define GP_ASSERT(expression)
#else
#define GP_ASSERT(expression) assert(expression)
#endif

#ifdef NDEBUG
#define GL_ASSERT( gl_code ) gl_code
#else
#define GL_ASSERT( gl_code ) gl_code
//#define GL_ASSERT( gl_code ) do	{ gl_code; __gl_error_code = glGetError(); GP_ASSERT(__gl_error_code == GL_NO_ERROR); } while(0)
#endif

#ifdef GP_ERRORS_AS_WARNINGS
#define GP_ERROR GP_WARN
#else
#define GP_ERROR(...) do { /*LOG HERE...*/ assert(0); std::exit(-1); } while(0)
#endif
