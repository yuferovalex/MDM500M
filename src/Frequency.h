#pragma once

#include <ratio>
#include <chrono>
#include <limits>

/// Библиотека для работы с частотой, по типу chrono

template <class _Type, class _Ratio>
class Frequency;

template <typename>
struct IsFrequency;

template<class _To,
    class _Type,
    class _Ratio> inline
constexpr typename std::enable_if<IsFrequency<_To>::value, _To>::type
frequency_cast(const Frequency<_Type, _Ratio>& freq);

template <class _Type, class _Ratio>
class Frequency
{
public:
    typedef _Type Type;
    typedef _Ratio Ratio;
    typedef Frequency<Type, Ratio> Self;

    constexpr Frequency() = default;

    template <typename Type2,
              typename = typename std::enable_if<
                  std::is_convertible<Type2, Type>::value
                  && (std::is_floating_point<Type>::value
                  || !std::is_floating_point<Type2>::value), void>::type>
    constexpr Frequency(const Type2 &value)
        : m_value(static_cast<Type>(value))
    {}

    template <typename Type2,
              typename Ratio2,
              typename = typename std::enable_if<
                  std::is_floating_point<Type>::value
                  || (std::ratio_divide<Ratio2, Ratio>::den == 1
                  && !std::is_floating_point<Type2>::value), void>::type >
    constexpr Frequency(const Frequency<Type2, Ratio2> &rhs)
        : m_value(frequency_cast<Self>(rhs).count())
    {}

    constexpr Type count() const
    {
        return m_value;
    }

    constexpr Self operator+() const
    {
        return (*this);
    }

    constexpr Self operator-() const
    {
        return Self(0 - m_value);
    }

    Self &operator++()
    {
        ++m_value;
        return (*this);
    }

    Self operator++(int)
    {
        return Self(m_value++);
    }

    Self &operator--()
    {
        --m_value;
        return (*this);
    }

    Self operator--(int)
    {
        return Self(m_value--);
    }

    Self &operator+=(const Self &rhs)
    {
        m_value += rhs.m_value;
        return (*this);
    }

    Self &operator-=(const Self &rhs)
    {
        m_value += rhs.m_value;
        return (*this);
    }

    Self &operator*=(const Type &rhs)
    {
        m_value *= rhs;
        return (*this);
    }

    Self &operator/=(const Type &rhs)
    {
        m_value /= rhs;
        return (*this);
    }

    Self &operator%=(const Type &rhs)
    {
        m_value %= rhs;
        return (*this);
    }

    Self &operator%=(const Self &rhs)
    {
        m_value %= rhs.m_value;
        return (*this);
    }

    constexpr static Self zero()
    {
        return Self();
    }

    constexpr static Self min()
    {
        return Self(std::numeric_limits<Type>::lowest());
    }

    constexpr static Self max()
    {
        return Self(std::numeric_limits<Type>::max());
    }

private:
    Type m_value = Type(0);
};

template <typename T>
struct IsFrequency : std::false_type {};

template <typename Type, typename Ratio>
struct IsFrequency<Frequency<Type, Ratio>> : std::true_type {};

template<class _To,
	class _Type,
	class _Ratio> inline
constexpr typename std::enable_if<IsFrequency<_To>::value, _To>::type
frequency_cast(const Frequency<_Type, _Ratio>& freq)
{
    typedef std::ratio_divide<_Ratio, typename _To::Ratio> _CF;
    typedef typename _To::Type _ToType;
    typedef typename std::common_type<_ToType, _Type, intmax_t>::type _CR;
    return (_CF::num == 1 && _CF::den == 1
                    ? static_cast<_To>(static_cast<_ToType>(freq.count()))
            : _CF::num != 1 && _CF::den == 1
                    ? static_cast<_To>(static_cast<_ToType>(
			static_cast<_CR>(
                            freq.count()) * static_cast<_CR>(_CF::num)))
            : _CF::num == 1 && _CF::den != 1
                    ? static_cast<_To>(static_cast<_ToType>(
                    	static_cast<_CR>(freq.count())
                            / static_cast<_CR>(_CF::den)))
            : static_cast<_To>(static_cast<_ToType>(
                    static_cast<_CR>(freq.count()) * static_cast<_CR>(_CF::num)
                            / static_cast<_CR>(_CF::den))));
}

template<intmax_t _Pn>
  struct _sign
  : std::integral_constant<intmax_t, (_Pn < 0) ? -1 : 1>
  { };

template<intmax_t _Pn>
  struct _abs
  : std::integral_constant<intmax_t, _Pn * _sign<_Pn>::value>
  { };


template<intmax_t _Pn, intmax_t _Qn>
  struct _gcd
  : _gcd<_Qn, (_Pn % _Qn)>
  { };

template<intmax_t _Pn>
  struct _gcd<_Pn, 0>
  : std::integral_constant<intmax_t, _abs<_Pn>::value>
  { };

template<intmax_t _Qn>
  struct _gcd<0, _Qn>
  : std::integral_constant<intmax_t, _abs<_Qn>::value>
  { };

template<intmax_t _Ax,
	intmax_t _Bx>
    struct _lcm
	{	/* compute least common multiple of _Ax and _Bx */
    static constexpr intmax_t _Gx = _gcd<_Ax, _Bx>::value;
	static constexpr intmax_t value = (_Ax / _Gx) * _Bx;
	};

namespace std {

template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
	struct common_type<
		Frequency<Type1, Ratio1>,
		Frequency<Type2, Ratio2> >
        {
	typedef Frequency<typename std::common_type<Type1, Type2>::type,
        std::ratio<_gcd<Ratio1::num, Ratio2::num>::value,
            _lcm<Ratio1::den, Ratio2::den>::value> > type;
	};

} // namespace std

template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
inline constexpr bool operator <
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    typedef typename std::common_type<Frequency<Type1, Ratio1>,
                                      Frequency<Type2, Ratio2> >::type CT;
    return CT(lhs).count() < CT(rhs).count();
}

template<class Type1,
    class Ratio1,
    class Type2,
    class Ratio2>
inline constexpr bool operator <=
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    typedef typename std::common_type<Frequency<Type1, Ratio1>,
                                      Frequency<Type2, Ratio2> >::type CT;
    return CT(lhs).count() <= CT(rhs).count();
}

template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
inline constexpr bool operator ==
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    typedef typename std::common_type<Frequency<Type1, Ratio1>,
                                      Frequency<Type2, Ratio2> >::type CT;
    return CT(lhs).count() == CT(rhs).count();
}

template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
inline constexpr bool operator >=
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    return !(lhs < rhs);
}

template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
inline constexpr bool operator !=
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    return !(lhs == rhs);
}

template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
inline constexpr bool operator >
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    return !(lhs < rhs) && lhs != rhs;
}

template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
inline
constexpr
typename std::common_type<Frequency<Type1, Ratio1>, Frequency<Type2, Ratio2> >::type
operator +
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    typedef typename std::common_type<
            Frequency<Type1, Ratio1>,
            Frequency<Type2, Ratio2>
            >::type CT;
    return CT(lhs).count() + CT(rhs).count();
}

template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
inline
constexpr
typename std::common_type<Frequency<Type1, Ratio1>, Frequency<Type2, Ratio2> >::type
operator -
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    typedef typename std::common_type<
            Frequency<Type1, Ratio1>,
            Frequency<Type2, Ratio2>
            >::type CT;
    return CT(lhs).count() - CT(rhs).count();
}


template<class Type1,
	class Ratio1,
	class Type2,
	class Ratio2>
inline
constexpr
typename std::common_type<Frequency<Type1, Ratio1>, Frequency<Type2, Ratio2> >::type
operator *
(
    const Frequency<Type1, Ratio1> &lhs,
    const Frequency<Type2, Ratio2> &rhs
)
{
    typedef typename std::common_type<
            Frequency<Type1, Ratio1>,
            Frequency<Type2, Ratio2>
            >::type CT;
    return CT(lhs).count() * CT(rhs).count();
}

template<class Type1,
	class Ratio,
	class Type2>
inline
constexpr
Frequency<Type1, Ratio>
operator *
(
    const Frequency<Type1, Ratio> &lhs,
    const Type2 &rhs
)
{
    return Frequency<Type1, Ratio>(lhs.count() * rhs);
}

template<class Type1,
	class Ratio,
	class Type2>
inline
constexpr
Frequency<Type1, Ratio>
operator *
(
    const Type2 &lhs,
    const Frequency<Type1, Ratio> &rhs
)
{
    return Frequency<Type1, Ratio>(rhs.count() * lhs);
}

template<class Type1,
	class Ratio,
	class Type2>
inline
constexpr
Frequency<Type1, Ratio>
operator /
(
    const Frequency<Type1, Ratio> &lhs,
    const Type2 &rhs
)
{
    return Frequency<Type1, Ratio>(lhs.count() / rhs);
}

template<class Type1,
	class Ratio,
	class Type2>
inline
constexpr
Frequency<Type1, Ratio>
operator %
(
    const Frequency<Type1, Ratio> &lhs,
    const Type2 &rhs
)
{
    return Frequency<Type1, Ratio>(lhs.count() % rhs);
}

using KiloHertz = Frequency<unsigned int, std::kilo>;
using MegaHertz = Frequency<unsigned int, std::mega>;

using KiloHertzReal = Frequency<double, std::kilo>;
using MegaHertzReal = Frequency<double, std::mega>;

inline constexpr MegaHertz operator "" _MHz(unsigned long long value)
{
    return MegaHertz(value);
}

inline constexpr KiloHertz operator "" _KHz(unsigned long long value)
{
    return KiloHertz(value);
}

inline constexpr MegaHertzReal operator "" _MHz(long double value)
{
    return MegaHertzReal(value);
}

inline constexpr KiloHertzReal operator "" _KHz(long double value)
{
    return KiloHertzReal(value);
}

