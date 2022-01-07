#pragma once // potentially faster compile-times if supported
#ifndef UTILITY_HPP_43ER7VL9
#define UTILITY_HPP_43ER7VL9

// TODO: replace with `std::unreachable()` when it becomes available.
# ifdef __GNUC__ // GCC 4.8+, Clang, Intel and other compilers compatible with GCC (-std=c++0x or above)
	[[noreturn]] inline __attribute__((always_inline)) void unreachable() { __builtin_unreachable(); }
# elif defined(_MSC_VER) // MSVC
	[[noreturn]] __forceinline void unreachable() { __assume(false); }
# else
#    error Unsupported compiler! //	inline void unreachable() {}
# endif

bool constexpr gIsDebugMode {
	#if !defined( NDEBUG )
		true
	#else
		false
	#endif
};

#endif // end-of-header-guard UTILITY_HPP_43ER7VL9
// EOF
