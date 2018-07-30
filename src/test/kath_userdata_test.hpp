#pragma once

struct player {
public:

    static int coef;

    int bullets;
    int speed;

    player()
        : player(3, 100) {

    }

    player(int ammo)
        : player(ammo, 100) {

    }

    player(int ammo, int hitpoints)
        : bullets(ammo), hp(hitpoints) {

    }

    void boost() {
        speed += 10;
    }

    bool shoot() {
        if (bullets < 1)
            return false;
        --bullets;
        return true;
    }

    void update_status(int ammo)
    {
        bullets = ammo;
    }

    void update_status(int ammo, int hitpoints)
    {
        bullets = ammo;
        hp = hitpoints;
    }

    void set_hp(int value) {
        hp = value;
    }

    int get_hp() const {
        return hp;
    }

private:
    int hp;
};

int player::coef = 0;

BOOST_AUTO_TEST_CASE(userdata_bisc)
{
    KATH_LUA_LOWLEVEL_BEGIN;

    kath::new_class<player>(L, "player").
        constructors<KATH_ARGS(), KATH_ARGS(int), KATH_ARGS(int, int)>().
        member("boost", &player::boost).
        member("shoot", &player::shoot).
        member("speed", &player::speed).
        member("coef", &player::coef).
        overload("update_statud", 
            static_cast<void(player::*)(int)>(&player::update_status), 
            static_cast<void(player::*)(int, int)>(&player::update_status)).
        property("hs", &player::get_hp, &player::set_hp);

    KATH_LUA_LOWLEVEL_END;
}