#
# Compiler/stdlib feature checks for ctwm
#


# Expect and try to enforce a C99 capable compiler.  There doesn't seem
# an obvious way to be sure in a fully portable way, but we probably
# don't work well in places that compile with something other than a
# program called like 'cc', and a cc that supports C99 should accept -std
# calls, so that's good enough.  Lacking it is not (yet) a fatal error,
# but is a sign that it's a compiler or platform we're moving further
# away from.
include(CheckCCompilerFlag)
set(C99_FLAG -std=c99)
check_c_compiler_flag(${C99_FLAG} COMPILER_HAS_STD_C99)
if(COMPILER_HAS_STD_C99)
	message(STATUS "Enabling ${C99_FLAG}")
	add_definitions(${C99_FLAG})
else()
	message(WARNING "Compiler doesn't support ${C99_FLAG}, "
			"building without it.")
endif(COMPILER_HAS_STD_C99)


# With -std=c99, GNU libc's includes get strict about what they export.
# Particularly, a lot of POSIX stuff doesn't get defined unless we
# explicitly ask for it.  Do our best at checking for what's there...
set(USE_GLIBC_FEATURES_H OFF)
check_include_files(features.h HAS_FEATURES_H)
if(HAS_FEATURES_H)
	# Check if including it with our args sets __USE_ISOC99; that's a
	# sign it's what we're looking for here.
	check_symbol_exists(__USE_ISOC99 features.h SETS_USE_ISOC99)
	if(SETS_USE_ISOC99)
		# OK, it does.  Assume that's a good enough test that things are
		# acting as we expect, and set the flag for our ctwm_config.h.
		set(USE_GLIBC_FEATURES_H ON)
		message(STATUS "Enabling glibc features.h settings.")
	endif()
endif(HAS_FEATURES_H)
