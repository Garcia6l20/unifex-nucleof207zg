/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

// Get the library feature test macros:
#if __has_include(<version>)
#  include <version>
#elif __has_include(<ciso646>)
#  include <ciso646>
#endif

// the configured options and settings for unifex
#define UNIFEX_VERSION_MAJOR 1
#define UNIFEX_VERSION_MINOR 1

#define 	 UNIFEX_NO_MEMORY_RESOURCE
#define UNIFEX_MEMORY_RESOURCE_HEADER <memory_resource>
#define UNIFEX_MEMORY_RESOURCE_NAMESPACE std::pmr

#define UNIFEX_NO_THREADS

#ifndef UNIFEX_ASSERT
#include <cassert>
# define UNIFEX_ASSERT assert
#endif

#define UNIFEX_CLANG_DISABLE_OPTIMIZATION

#if defined(__has_cpp_attribute)
// prior to clang-10, [[no_unique_address]] leads to bad codegen
#if __has_cpp_attribute(no_unique_address) && \
  (!defined(__clang__) || __clang_major__ > 9)
#define UNIFEX_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define UNIFEX_NO_UNIQUE_ADDRESS
#endif
#else
#define UNIFEX_NO_UNIQUE_ADDRESS
#endif

#if !defined(UNIFEX_NO_COROUTINES)
#  if defined(__cpp_impl_coroutine) || defined(__cpp_coroutines)
#    define UNIFEX_NO_COROUTINES 0
#  else
#    define UNIFEX_NO_COROUTINES 1
#  endif
#endif

#if !UNIFEX_NO_COROUTINES
#  if __has_include(<coroutine>) && defined(__cpp_lib_coroutine)
#    define UNIFEX_COROUTINES_HEADER <coroutine>
#    define UNIFEX_COROUTINES_NAMESPACE std
#  elif __has_include(<experimental/coroutine>)
#    define UNIFEX_COROUTINES_HEADER <experimental/coroutine>
#    define UNIFEX_COROUTINES_NAMESPACE std::experimental
#  else
#    undef UNIFEX_NO_COROUTINES
#    define UNIFEX_NO_COROUTINES 1
#  endif
#endif

#if !defined(UNIFEX_NO_EPOLL)
#  if defined(__ANDROID_API__) && __ANDROID_API__ < 19
// Android makes timerfd_create and friend available as of API version 19;
// before that, the epoll API exists but it's insufficient for our purposes.
// https://android.googlesource.com/platform/bionic/+/master/libc/include/sys/timerfd.h#56
#    define UNIFEX_NO_EPOLL 1
#  endif
#endif

#if !defined(UNIFEX_NO_EPOLL)
#define UNIFEX_NO_EPOLL
#endif

#if !defined(UNIFEX_NO_LIBURING)
#define UNIFEX_NO_LIBURING
#endif

// UNIFEX_DECLARE_NON_DEDUCED_TYPE(type)
// UNIFEX_USE_NON_DEDUCED_TYPE(type)
//
// These macros work around a bug in MSVC that causes it to try to specialize
// all found function templates for which it can successfully deduce template
// arguments even if there are parameters that do not participate in template
// parameter deduction for which there is no conversion possible from the argument.
//
// This is most likely related to the core issue CWG1391 [*] which MSVC has not
// yet implemented as of VS 2019.4.
//
// These macros are intended to be used in template functions, typically tag_invoke()
// overloads, as follows.
//
// Where you would normally write:
//
// class foo_sender {
//    template<
//      typename Receiver,
//      std::enable_if_t<is_callable_v<decltype(set_value), Receiver, foo>, int> = 0>
//    friend auto tag_invoke(tag_t<connect>, foo_sender&& s, Receiver&& r) -> foo_operation<Receiver> {
//      return ...;
//    }
// };
//
// You would instead write
//
// class foo_sender {
//   template<
//     typename Receiver,
//     UNIFEX_DECLARE_NON_DEDUCED_TYPE(CPO, tag_t<connect>),
//     UNIFEX_DECLARE_NON_DEDUCED_TYPE(S, foo_sender),
//     std::enable_if_t<is_callable_v<decltype(set_value), Receiver, foo>, int> = 0?
//   friend auto tag_invoke(
//        UNIFEX_USE_NON_DEDUCED_TYPE(CPO, tag_t<connect>),
//        UNIFEX_USE_NON_DEDUCED_TYPE(S, foo_sender)&& s,
//        Receiver&& r) -> foo_operation<Receiver> {
//      return ...;
//    }
// };

#if defined(_MSC_VER)
# define UNIFEX_DECLARE_NON_DEDUCED_TYPE(NAME, ...) \
  typename NAME, \
  std::enable_if_t<std::is_same_v<NAME, __VA_ARGS__>, int> = 0
 # define UNIFEX_USE_NON_DEDUCED_TYPE(NAME, ...) NAME
#else
# define UNIFEX_DECLARE_NON_DEDUCED_TYPE(NAME, ...) typename NAME = __VA_ARGS__
# define UNIFEX_USE_NON_DEDUCED_TYPE(NAME, ...) __VA_ARGS__
#endif

#if !defined(UNIFEX_CXX_CONCEPTS)
#ifdef UNIFEX_DOXYGEN_INVOKED
#define UNIFEX_CXX_CONCEPTS 201800L
#elif defined(__cpp_concepts) && __cpp_concepts > 0
#define UNIFEX_CXX_CONCEPTS __cpp_concepts
#else
#define UNIFEX_CXX_CONCEPTS 0L
#endif
#endif

#if __cpp_rtti >= 199711
#  define UNIFEX_NO_RTTI 0
#else
#  define UNIFEX_NO_RTTI 1
#endif

#if __cpp_exceptions >= 199711
#  define UNIFEX_NO_EXCEPTIONS 0
#  define UNIFEX_TRY try
#  define UNIFEX_CATCH(...) catch (__VA_ARGS__)
#  define UNIFEX_RETHROW() throw
#else
#  define UNIFEX_NO_EXCEPTIONS 1
#  define UNIFEX_TRY
#  define UNIFEX_CATCH(...) if constexpr (true) {} else
#  define UNIFEX_RETHROW() ((void)0)
#endif

#if defined(_MSC_VER) && !defined(__clang__)
  #define UNIFEX_DIAGNOSTIC_PUSH __pragma(warning(push))
  #define UNIFEX_DIAGNOSTIC_POP __pragma(warning(pop))
  #define UNIFEX_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME
  #define UNIFEX_DIAGNOSTIC_IGNORE_FLOAT_EQUAL
  #define UNIFEX_DIAGNOSTIC_IGNORE_CPP2A_COMPAT
#else // ^^^ defined(_MSC_VER) ^^^ / vvv !defined(_MSC_VER) vvv
  #if defined(__GNUC__) || defined(__clang__)
    #define UNIFEX_PRAGMA(X) _Pragma(#X)
    #define UNIFEX_DIAGNOSTIC_PUSH UNIFEX_PRAGMA(GCC diagnostic push)
    #define UNIFEX_DIAGNOSTIC_POP UNIFEX_PRAGMA(GCC diagnostic pop)
    #define UNIFEX_DIAGNOSTIC_IGNORE_PRAGMAS \
      UNIFEX_PRAGMA(GCC diagnostic ignored "-Wpragmas")
    #define UNIFEX_DIAGNOSTIC_IGNORE(X) \
      UNIFEX_DIAGNOSTIC_IGNORE_PRAGMAS \
      UNIFEX_PRAGMA(GCC diagnostic ignored "-Wunknown-pragmas") \
      UNIFEX_PRAGMA(GCC diagnostic ignored X)
    #define UNIFEX_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME \
      UNIFEX_DIAGNOSTIC_IGNORE("-Wunknown-warning-option") \
      UNIFEX_DIAGNOSTIC_IGNORE("-Winit-list-lifetime")
    #define UNIFEX_DIAGNOSTIC_IGNORE_FLOAT_EQUAL \
      UNIFEX_DIAGNOSTIC_IGNORE("-Wfloat-equal")
    #define UNIFEX_DIAGNOSTIC_IGNORE_CPP2A_COMPAT \
      UNIFEX_DIAGNOSTIC_IGNORE("-Wc++2a-compat")
  #else
    #define UNIFEX_DIAGNOSTIC_PUSH
    #define UNIFEX_DIAGNOSTIC_POP
    #define UNIFEX_DIAGNOSTIC_IGNORE_INIT_LIST_LIFETIME
    #define UNIFEX_DIAGNOSTIC_IGNORE_FLOAT_EQUAL
    #define UNIFEX_DIAGNOSTIC_IGNORE_CPP2A_COMPAT
  #endif
#endif // MSVC/Generic configuration switch

#if defined(__GNUC__) || defined(__clang__)
  #define UNIFEX_ALWAYS_INLINE \
    __attribute__((__always_inline__)) __attribute__((__visibility__("hidden"))) inline
#elif defined(_MSC_VER)
  #define UNIFEX_ALWAYS_INLINE __forceinline
#else
  #define UNIFEX_ALWAYS_INLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__)
  #define UNIFEX_ASSUME_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
  #define UNIFEX_ASSUME_UNREACHABLE __assume(0)
#else
  #define UNIFEX_ASSUME_UNREACHABLE std::terminate()
#endif
