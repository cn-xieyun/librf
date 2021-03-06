//===----------------------------- coroutine -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_EXPERIMENTAL_COROUTINE
#define _LIBCPP_EXPERIMENTAL_COROUTINE

#define _EXPERIMENTAL_COROUTINE_

/**
	experimental/coroutine synopsis

// C++next

namespace std {
namespace experimental {
inline namespace coroutines_v1 {

  // 18.11.1 coroutine traits
template <typename R, typename... ArgTypes>
class coroutine_traits;
// 18.11.2 coroutine handle
template <typename Promise = void>
class coroutine_handle;
// 18.11.2.7 comparison operators:
bool operator==(coroutine_handle<> x, coroutine_handle<> y) noexcept;
bool operator!=(coroutine_handle<> x, coroutine_handle<> y) noexcept;
bool operator<(coroutine_handle<> x, coroutine_handle<> y) noexcept;
bool operator<=(coroutine_handle<> x, coroutine_handle<> y) noexcept;
bool operator>=(coroutine_handle<> x, coroutine_handle<> y) noexcept;
bool operator>(coroutine_handle<> x, coroutine_handle<> y) noexcept;
// 18.11.3 trivial awaitables
struct suspend_never;
struct suspend_always;
// 18.11.2.8 hash support:
template <class T> struct hash;
template <class P> struct hash<coroutine_handle<P>>;

} // namespace coroutines_v1
} // namespace experimental
} // namespace std

 */

#include <new>
#include <type_traits>
#include <functional>
#include <memory> // for hash<T*>
#include <cstddef>
#include <cassert>

#if defined(__clang__)
#include "clang_builtin.h"
#elif defined(__GNUC__)
#include "gcc_builtin.h"
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#pragma GCC system_header
#endif

#ifndef _LIBCPP_HAS_NO_COROUTINES

namespace std {
#if defined(__GNUC__)
	inline
#endif
	namespace experimental {

		template <class _Tp, class = void>
		struct __coroutine_traits_sfinae {};

		template <class _Tp>
		struct __coroutine_traits_sfinae<_Tp, void_t<typename _Tp::promise_type>>
		{
			using promise_type = typename _Tp::promise_type;
		};

		template <typename _Ret, typename... _Args>
		struct coroutine_traits : public __coroutine_traits_sfinae<_Ret>
		{
		};

		template <typename _Promise = void>
		class coroutine_handle;

		template <>
		class coroutine_handle<void> {
		public:
			constexpr coroutine_handle() noexcept : __handle_(nullptr) {}
			constexpr coroutine_handle(nullptr_t) noexcept : __handle_(nullptr) {}
			coroutine_handle& operator=(nullptr_t) noexcept {
				__handle_ = nullptr;
				return *this;
			}

			constexpr void* address() const noexcept { return __handle_; }
			constexpr explicit operator bool() const noexcept { return __handle_; }

			void operator()() { resume(); }
			void resume() {
				__builtin_coro_resume(__handle_);
			}
			void destroy() {
				__builtin_coro_destroy(__handle_);
			}
			bool done() const {
				return __builtin_coro_done(__handle_);
			}
		public:
			static coroutine_handle from_address(void* __addr) noexcept {
				coroutine_handle __tmp;
				__tmp.__handle_ = __addr;
				return __tmp;
			}

			// FIXME: Should from_address(nullptr) be allowed?
			static coroutine_handle from_address(nullptr_t) noexcept {
				return coroutine_handle(nullptr);
			}

			template <class _Tp, bool _CallIsValid = false>
			static coroutine_handle from_address(_Tp*) {
				static_assert(_CallIsValid,
					"coroutine_handle<void>::from_address cannot be called with "
					"non-void pointers");
			}

		private:
			bool __is_suspended() const noexcept {
				// FIXME actually implement a check for if the coro is suspended.
				return __handle_;
			}

			template <class _PromiseT>
			friend class coroutine_handle;

			void* __handle_;
		};

		// 18.11.2.7 comparison operators:
		inline bool operator==(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return __x.address() == __y.address();
		}
		inline bool operator!=(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return !(__x == __y);
		}
		inline bool operator<(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return less<void*>()(__x.address(), __y.address());
		}
		inline bool operator>(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return __y < __x;
		}
		inline bool operator<=(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return !(__x > __y);
		}
		inline bool operator>=(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return !(__x < __y);
		}

		template <typename _Promise>
		class coroutine_handle : public coroutine_handle<> {
			using _Base = coroutine_handle<>;
		public:
#ifndef _LIBCPP_CXX03_LANG
			// 18.11.2.1 construct/reset
			using coroutine_handle<>::coroutine_handle;
#else
			coroutine_handle() noexcept : _Base() {}
			coroutine_handle(nullptr_t) noexcept : _Base(nullptr) {}
#endif

			coroutine_handle& operator=(nullptr_t) noexcept {
				_Base::operator=(nullptr);
				return *this;
			}

			_Promise& promise() const {
				return *static_cast<_Promise*>(
					__builtin_coro_promise(this->__handle_, alignof(_Promise), false));
			}

		public:
			static coroutine_handle from_address(void* __addr) noexcept {
				coroutine_handle __tmp;
				__tmp.__handle_ = __addr;
				return __tmp;
			}

			// NOTE: this overload isn't required by the standard but is needed so
			// the deleted _Promise* overload doesn't make from_address(nullptr)
			// ambiguous.
			// FIXME: should from_address work with nullptr?
			static coroutine_handle from_address(nullptr_t) noexcept {
				return coroutine_handle(nullptr);
			}

			template <class _Tp, bool _CallIsValid = false>
			static coroutine_handle from_address(_Tp*) {
				static_assert(_CallIsValid,
					"coroutine_handle<promise_type>::from_address cannot be called with "
					"non-void pointers");
			}

			template <bool _CallIsValid = false>
			static coroutine_handle from_address(_Promise*) {
				static_assert(_CallIsValid,
					"coroutine_handle<promise_type>::from_address cannot be used with "
					"pointers to the coroutine's promise type; use 'from_promise' instead");
			}

			static coroutine_handle from_promise(_Promise& __promise) noexcept {
				typedef typename remove_cv<_Promise>::type _RawPromise;
				coroutine_handle __tmp;
				__tmp.__handle_ = __builtin_coro_promise(
					std::addressof(const_cast<_RawPromise&>(__promise)),
					alignof(_Promise), true);
				return __tmp;
			}
		};

#if __has_builtin(__builtin_coro_noop)
		struct noop_coroutine_promise {};

		template <>
		class coroutine_handle<noop_coroutine_promise>
			: public coroutine_handle<> {
			using _Base = coroutine_handle<>;
			using _Promise = noop_coroutine_promise;
		public:
			_Promise& promise() const {
				return *static_cast<_Promise*>(
					__builtin_coro_promise(this->__handle_, alignof(_Promise), false));
			}

			constexpr explicit operator bool() const noexcept { return true; }
			constexpr bool done() const noexcept { return false; }

			constexpr void operator()() const noexcept {}
			constexpr void resume() const noexcept {}
			constexpr void destroy() const noexcept {}
		private:
			friend coroutine_handle<noop_coroutine_promise> noop_coroutine() noexcept;
			coroutine_handle() noexcept {
				this->__handle_ = __builtin_coro_noop();
			}
		};

		using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;

		inline noop_coroutine_handle noop_coroutine() noexcept {
			return noop_coroutine_handle();
		}
#endif // __has_builtin(__builtin_coro_noop)

		struct suspend_never {
			bool await_ready() const noexcept { return true; }
			void await_suspend(coroutine_handle<>) const noexcept {}
			void await_resume() const noexcept {}
		};

		struct suspend_always {
			bool await_ready() const noexcept { return false; }
			void await_suspend(coroutine_handle<>) const noexcept {}
			void await_resume() const noexcept {}
		};

	}

	template <class _Tp>
	struct hash<experimental::coroutine_handle<_Tp> > {
		using __arg_type = experimental::coroutine_handle<_Tp>;
		size_t operator()(__arg_type const& __v) const noexcept
		{
			return hash<void*>()(__v.address());
		}
	};
}

#endif // !defined(_LIBCPP_HAS_NO_COROUTINES)

#endif /* _LIBCPP_EXPERIMENTAL_COROUTINE */