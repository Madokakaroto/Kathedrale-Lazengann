#pragma once

namespace userdata_test
{
    class vector3
    {
    public:
        vector3()
            : vector3(0)
        {}

        explicit vector3(float u) 
            : vector3(u, u, u)
        {}

        vector3(float x, float y, float z)
            : x_(x)
            , y_(y)
            , z_(z)
        {}

        float get_x() const
        {
            return x_;
        }

        void set_x(float x)
        {
            x_ = x;
        }

        float get_y() const
        {
            return y_;
        }

        void set_y(float y)
        {
            y_ = y;
        }

        float get_z() const
        {
            return z_;
        }

        void set_z(float z)
        {
            z_ = z;
        }

        float magnitude() const
        {
            return x_ * x_ + y_ * y_ + z_ * z_;
        }

        vector3 cross(vector3 const& v) const
        {
            return vector3
            {
                y_ * v.z_ - z_ * v.y_,
                z_ * v.x_ - x_ * v.z_,
                x_ * v.y_ - y_ * v.x_
            };
        }

    private:
        float x_;
        float y_;
        float z_;
    };

    class player : public std::enable_shared_from_this<player>
    {
    public:
        player()
        {
        }

    private:
        int     hp = 0;
        int     mp = 0;
        int     sp = 0;
        int     exp = 0;
    };
}

BOOST_AUTO_TEST_CASE(userdata_basic)
{
    KATH_LUA_LOWLEVEL_BEGIN;

    using userdata_test::vector3;

    kath::new_class<vector3>(L, "vector3").
        constructors(KATH_ARGS(), KATH_ARGS(float), KATH_ARGS(float, float, float)).
        member("magnitude", &vector3::magnitude).
        member("cross", &vector3::cross).
        property("X", &vector3::get_x, &vector3::set_x).
        property("Y", &vector3::get_y, &vector3::set_y).
        property("Z", &vector3::get_z, &vector3::set_z);


    BOOST_CHECK(do_script_low_level(L, R"(
        function print_table(table)
            for k,v in pairs(table) do
                print(k, v)
            end
        end

        a = vector3()
        meta_a = getmetatable(a)
        print_table(meta_a)
        --print_table(meta_a.__set)
        --print_table(meta_a.__get)

        b = vector3(1.0)
        magnitude = b:magnitude()
        print(magnitude)
        print(b.X, b.Y, b.Z)
        b.X = 2.0
        b.Y = 2.0
        b.Z = 3.0
        magnitude = b:magnitude()
        print(magnitude)
        print(b.X, b.Y, b.Z)
    )"));

    KATH_LUA_LOWLEVEL_END;
}

BOOST_AUTO_TEST_CASE(userdata_shared_ptr)
{
    KATH_LUA_LOWLEVEL_BEGIN;

    using userdata_test::player;
    kath::new_class<player>(L, "player")
        .constructors(KATH_ARGS());

    auto p1 = std::make_shared<player>();

    kath::stack_push(L, p1.get());
    kath::stack_push(L, p1);
    kath::stack_push(L, std::addressof(p1));
    kath::stack_push(L, *p1);
    KATH_LUA_LOWLEVEL_END
}