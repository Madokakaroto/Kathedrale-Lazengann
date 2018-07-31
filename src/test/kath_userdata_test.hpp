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

        float set_x() const
        {
            return x_;
        }

        float get_y() const
        {
            return y_;
        }

        float set_y() const
        {
            return y_;
        }

        float get_z() const
        {
            return z_;
        }

        float set_z() const
        {
            return z_;
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
}

BOOST_AUTO_TEST_CASE(userdata_bisc)
{
    KATH_LUA_LOWLEVEL_BEGIN;

    using userdata_test::vector3;

    kath::new_class<vector3>(L, "vector3").
        constructors(KATH_ARGS(float), KATH_ARGS(float, float, float)).
        member("magnitude", &vector3::magnitude).
        member("cross", &vector3::cross).
        property("X", &vector3::get_x, &vector3::set_x).
        property("Y", &vector3::get_y, &vector3::set_y).
        property("Z", &vector3::get_z, &vector3::set_z);


    BOOST_CHECK(do_script_low_level(L, R"(
        a = vector3(3.0)

        for k,v in pairs(getmetatable(a)) do
            print(k, v)
        end
    )"));


    KATH_LUA_LOWLEVEL_END;
}