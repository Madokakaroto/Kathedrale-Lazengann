#pragma once

struct player {
public:
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

    void set_hp(int value) {
        hp = value;
    }

    int get_hp() const {
        return hp;
    }

private:
    int hp;
};


BOOST_AUTO_TEST_CASE(userdata_bisc)
{
    KATH_LUA_LOWLEVEL_BEGIN;

    kath::new_class<player>(L, "player").
        //constructors<KATH_ARGS(), KATH_ARGS(int), KATH_ARGS(int, int)>().
        member("boost", &player::boost).
        //member("shoot", &player::shoot).
        member("speed", &player::speed);
        //property("hs", &player::get_hp, &player::set_hp);

    KATH_LUA_LOWLEVEL_END;
}