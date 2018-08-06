#pragma once

namespace kath { namespace detail
{

#if defined(_MSC_VER)
    inline static void demangle_impl(std::string& pretty_name)
    {
        constexpr std::array<std::string_view, 3> filter = {
            "`anonymous namespace'::", "class ", "struct ",
        };

        auto pos = std::accumulate(filter.begin(), filter.end(), std::string::npos, 
            [&](size_t pos, std::string_view keyword) 
        {
            auto find_pos = pretty_name.find(keyword);
            if (find_pos != std::string::npos && (find_pos > pos || std::string::npos == pos))
                return find_pos + keyword.length();
            return pos;
        });

        if (pos != std::string::npos)
        {
            pretty_name.erase(0, pos);
        }
    }
#elif defined(__clang__)
    inline static void demangle_impl(std::string& pretty_name)
    {
    }
#else
#error Demangle not implemented!
#endif

    template <typename T>
    inline static std::string demangle()
    {
        auto result = boost::typeindex::type_id<T>().pretty_name();
        demangle_impl(result);
        return result;
    }

} }