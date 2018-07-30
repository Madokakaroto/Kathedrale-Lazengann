#pragma once

#include <numeric>
#include <complex>

struct foo
{
    template <typename T>
    int operator()(T) { return true; }
};

struct bar
{
    using value_type = int;
};

BOOST_AUTO_TEST_CASE(BooleanMetaFunction)
{
    BOOST_CHECK((std::is_same_v<kath::safe_value_type_t<bar>, int>));
    BOOST_CHECK((std::is_same_v<kath::safe_value_type_t<std::string>, char>));
    BOOST_CHECK((std::is_same_v<kath::safe_value_type_t<foo>, void>));
    BOOST_CHECK((std::is_same_v<std::invoke_result_t<foo, void*>, int>));
}

//BOOST_AUTO_TEST_CASE(testest)
//{
//    using namespace std::complex_literals;
//
//    std::complex<int32_t> a, b;
//
//    auto quotient = (a * std::conj(b)) / std::norm(b);
//    auto remainder = a - b * quotient;
//}