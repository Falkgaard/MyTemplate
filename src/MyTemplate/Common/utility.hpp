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

bool constexpr kIsDebugMode {
	#if !defined( NDEBUG )
		true
	#else
		false
	#endif
};

bool constexpr kIsVerbose { false }; // TODO: expose to CMake or make a run-time constant set with args

template <typename... T>
bool constexpr always_false_v { false };

#include <limits>
#include <concepts>

template <typename T> requires std::integral<T> or std::floating_point<T>
auto constexpr max { std::numeric_limits<T>::max() };

template <typename T> requires std::integral<T> or std::floating_point<T>
auto constexpr min { std::numeric_limits<T>::min() };

#endif // end-of-header-guard UTILITY_HPP_43ER7VL9
// EOF
