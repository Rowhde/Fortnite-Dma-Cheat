#ifndef JM_XORSTR_HPP
#define JM_XORSTR_HPP
#define JM_XORSTR_DISABLE_AVX_INTRINSICS
#include <immintrin.h>
#include <cstdint>
#include <cstddef>

#define xorstr(str) ::jm::xor_string<XORSTR_STR(str)>()
#define xs(str) xorstr(str).crypt_get()

#ifdef _MSC_VER
#define XORSTR_FORCEINLINE __forceinline
#else
#define XORSTR_FORCEINLINE __attribute__((always_inline))
#endif

#if defined(__clang__)
#define XORSTR_CLANG_VOLATILE volatile
#define XORSTR_VOLATILE volatile
#elif defined(__GNUC__)
#define XORSTR_CLANG_VOLATILE
#define XORSTR_VOLATILE volatile
#else
#define XORSTR_CLANG_VOLATILE
#define XORSTR_VOLATILE
#endif

#define XORSTR_STRING_EXPAND_10(n, x)                                                    \
    jm::detail::c_at<n##0>(x), jm::detail::c_at<n##1>(x), jm::detail::c_at<n##2>(x),     \
        jm::detail::c_at<n##3>(x), jm::detail::c_at<n##4>(x), jm::detail::c_at<n##5>(x), \
        jm::detail::c_at<n##6>(x), jm::detail::c_at<n##7>(x), jm::detail::c_at<n##8>(x), \
        jm::detail::c_at<n##9>(x)

#define XORSTR_STRING_EXPAND_100(x)                                   \
    XORSTR_STRING_EXPAND_10(, x), XORSTR_STRING_EXPAND_10(1, x),      \
        XORSTR_STRING_EXPAND_10(2, x), XORSTR_STRING_EXPAND_10(3, x), \
        XORSTR_STRING_EXPAND_10(4, x), XORSTR_STRING_EXPAND_10(5, x), \
        XORSTR_STRING_EXPAND_10(6, x), XORSTR_STRING_EXPAND_10(7, x), \
        XORSTR_STRING_EXPAND_10(8, x), XORSTR_STRING_EXPAND_10(9, x)

#define XORSTR_STR(s)                                                      \
    ::jm::detail::string_builder<                                          \
        typename ::jm::detail::decay_array_deref<decltype(*s)>::type,      \
        jm::detail::tstring_<                                              \
            typename ::jm::detail::decay_array_deref<decltype(*s)>::type>, \
        XORSTR_STRING_EXPAND_100(s)>::type

namespace jm {

	namespace detail {

		template<class T>
		struct decay_array_deref;

		template<class T>
		struct decay_array_deref<T&> {
			using type = T;
		};

		template<class T>
		struct decay_array_deref<const T&> {
			using type = T;
		};

		template<bool Cond>
		struct conditional {
			template<class, class False>
			using type = False;
		};

		template<>
		struct conditional<true> {
			template<class True, class>
			using type = True;
		};

		template<class T>
		struct as_unsigned {
			using type = typename conditional<sizeof(T) == 1>::template type<
				std::uint8_t,
				typename conditional<sizeof(T) == 2>::template type<unsigned short,
				unsigned int>>;
		};

		template<std::size_t I, std::size_t M, class T>
		constexpr T c_at(const T(&str)[M]) noexcept
		{
			return (I < M) ? str[I] : 0;
		}

		template<class T, class B, T...>
		struct string_builder;

		template<class T, class B>
		struct string_builder<T, B> {
			using type = B;
		};

		template<class T, template<class, T...> class S, T... Hs, T C, T... Cs>
		struct string_builder<T, S<T, Hs...>, C, Cs...>
			: conditional<C ==
			T(0)>::template type<string_builder<T, S<T, Hs...>>,
			string_builder<T, S<T, Hs..., C>, Cs...>> {
		};

		template<class T, T... Cs>
		struct tstring_ {
			using value_type = T;
			constexpr static std::size_t size = sizeof...(Cs);
			constexpr static value_type  str[size + 1] = { Cs..., '\0' };

			constexpr static std::size_t size_in_bytes() noexcept
			{
				return (size + 1) * sizeof(value_type);
			}
		};

		constexpr static void hash_single(std::uint32_t& value, char c) noexcept
		{
			value = static_cast<std::uint32_t>((value ^ c) * 16777619ull);
		}

		template<std::uint32_t Seed>
		constexpr std::uint32_t key4() noexcept
		{
			std::uint32_t value = Seed;
			for (auto str = __FILE__; *str; ++str)
				hash_single(value, *str);
			return value;
		}

		template<std::size_t S>
		constexpr std::uint64_t key8()
		{
			constexpr auto first_part = key4<2166136261 + S>();
			constexpr auto second_part = key4<first_part>();
			return (static_cast<std::uint64_t>(first_part) << 32) | second_part;
		}

		template<class T>
		constexpr std::size_t buffer_size()
		{
			constexpr auto x = T::size_in_bytes() / 16;
			return x * 2 + ((T::size_in_bytes() - x * 16) % 16 != 0) * 2;
		}

		template<class T>
		constexpr std::size_t buffer_align()
		{
#ifndef JM_XORSTR_DISABLE_AVX_INTRINSICS
			return ((T::size_in_bytes() > 16) ? 32 : 16);
#else
			return 16;
#endif
		}

		template<class T>
		struct string_storage {
			std::uint64_t storage[buffer_size<T>()];

			template<std::size_t N = 0>
			XORSTR_FORCEINLINE constexpr void _xor()
			{
				if constexpr (N != detail::buffer_size<T>()) {
					constexpr auto key = key8<N>();
					storage[N] ^= key;
					_xor<N + 1>();
				}
			}

			XORSTR_FORCEINLINE constexpr string_storage() : storage{ 0 }
			{
				using cast_type = typename as_unsigned<typename T::value_type>::type;

				for (std::size_t i = 0; i < T::size; ++i)
					storage[i / (8 / sizeof(typename T::value_type))] |=
					(std::uint64_t{ static_cast<cast_type>(T::str[i]) }
						<< ((i % (8 / sizeof(typename T::value_type))) * 8 *
							sizeof(typename T::value_type)));
				_xor<0>();
			}
		};

	}

	template<class T>
	struct xor_string {
		alignas(detail::buffer_align<T>())
			XORSTR_VOLATILE std::uint64_t _storage[detail::buffer_size<T>()];

		template<std::size_t N>
		XORSTR_FORCEINLINE void _crypt() noexcept
		{
			if constexpr (detail::buffer_size<T>() > N) {
#ifndef JM_XORSTR_DISABLE_AVX_INTRINSICS
				if constexpr ((detail::buffer_size<T>() - N) >= 4) {
					alignas(32) XORSTR_CLANG_VOLATILE std::uint64_t keys[4];
					keys[0] = detail::key8<N + 0>();
					keys[1] = detail::key8<N + 1>();
					keys[2] = detail::key8<N + 2>();
					keys[3] = detail::key8<N + 3>();

					*(__m256i*)(&_storage[N]) = _mm256_xor_si256(
						*(__m256i*)(&_storage[N]), *(const __m256i*)(&keys));
					_crypt<N + 4>();
				}
				else
#endif
				{
					alignas(16) XORSTR_VOLATILE std::uint64_t keys[2];
					keys[0] = detail::key8<N + 0>();
					keys[1] = detail::key8<N + 1>();

					*(__m128i*)(&_storage[N]) = _mm_xor_si128(*(__m128i*)(&_storage[N]),
						*(const __m128i*)(&keys));
					_crypt<N + 2>();
				}
			}
		}

		template<std::size_t N>
		XORSTR_FORCEINLINE constexpr static std::uint64_t _at()
		{
			constexpr std::uint64_t val = detail::string_storage<T>{}.storage[N];
			return val;
		}

		template<std::size_t N>
		void _copy() noexcept
		{
			if constexpr (detail::buffer_size<T>() > N) {
				_storage[N] = _at<N>();
				_storage[N + 1] = _at<N + 1>();
				_copy<N + 2>();
			}
		}

	public:
		using value_type = typename T::value_type;
		using size_type = std::size_t;
		using pointer = value_type*;
		using const_pointer = const pointer;
		XORSTR_FORCEINLINE xor_string() noexcept { _copy<0>(); }
		constexpr size_type size() const noexcept { return T::size - 1; }
		XORSTR_FORCEINLINE void crypt() noexcept { _crypt<0>(); }
		XORSTR_FORCEINLINE const_pointer get() const noexcept { return _storage; }
		XORSTR_FORCEINLINE const_pointer crypt_get() noexcept
		{
			crypt();
			return (const_pointer)(_storage);
		}
	};

}

#endif

#ifdef _DEBUG
#undef xorstr_
#define xorstr_(x) (x)
#endif