#pragma once

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
