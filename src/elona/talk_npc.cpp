#include "ability.hpp"
#include "activity.hpp"
#include "adventurer.hpp"
#include "area.hpp"
#include "audio.hpp"
#include "calc.hpp"
#include "character.hpp"
#include "character_status.hpp"
#include "data/types/type_ability.hpp"
#include "data/types/type_item.hpp"
#include "deferred_event.hpp"
#include "elona.hpp"
#include "food.hpp"
#include "i18n.hpp"
#include "inventory.hpp"
#include "item.hpp"
#include "itemgen.hpp"
#include "macro.hpp"
#include "magic.hpp"
#include "map.hpp"
#include "map_cell.hpp"
#include "menu.hpp"
#include "message.hpp"
#include "quest.hpp"
#include "random.hpp"
#include "shop.hpp"
#include "talk.hpp"
#include "text.hpp"
#include "ui.hpp"
#include "variables.hpp"



namespace elona
{

namespace
{

TalkResult talk_shop_buy(Character& speaker)
{
    invctrl = 11;
    invfile = speaker.shop_store_id;
    shop_sell_item(speaker);
    screenupdate = -1;
    update_screen();
    cs = 0;
    buff = "";
    return TalkResult::talk_npc;
}



TalkResult talk_shop_sell(Character& speaker)
{
    invctrl = 12;
    invfile = speaker.shop_store_id;
    shop_sell_item(speaker);
    screenupdate = -1;
    update_screen();
    cs = 0;
    buff = "";
    return TalkResult::talk_npc;
}



TalkResult talk_inn_eat(Character& speaker)
{
    if (cdata.player().gold < calcmealvalue())
    {
        buff = i18n::s.get("core.ui.no_gold");
        return TalkResult::talk_npc;
    }
    if (cdata.player().nutrition >= 15000)
    {
        buff = i18n::s.get("core.talk.npc.innkeeper.eat.not_hungry", speaker);
        return TalkResult::talk_npc;
    }
    snd("core.paygold1");
    cdata.player().gold -= calcmealvalue();
    snd("core.eat1");
    cdata.player().nutrition = 15000;
    buff = i18n::s.get("core.talk.npc.innkeeper.eat.here_you_are", speaker);
    txt(i18n::s.get("core.talk.npc.innkeeper.eat.results"));
    show_eating_message(cdata.player());
    chara_anorexia(cdata.player());
    return TalkResult::talk_npc;
}



TalkResult talk_wizard_identify(Character& speaker, int chatval_)
{
    if (cdata.player().gold < calcidentifyvalue(chatval_ - 14))
    {
        buff = i18n::s.get("core.ui.no_gold");
        return TalkResult::talk_npc;
    }
    p = 0;
    for (const auto& item : g_inv.pc())
    {
        if (item->identify_state != IdentifyState::completely)
        {
            ++p;
        }
    }
    if (p == 0)
    {
        buff = i18n::s.get("core.talk.npc.wizard.identify.already", speaker);
        return TalkResult::talk_npc;
    }
    if (chatval_ == 15)
    {
        cdata.player().gold -= calcidentifyvalue(1);
        p(0) = 0;
        p(1) = 0;
        p(0) = 0;
        p(1) = 0;
        for (const auto& item : g_inv.pc())
        {
            if (item->identify_state != IdentifyState::completely)
            {
                const auto result = item_identify(item, 250);
                inv_stack(g_inv.pc(), item, true);
                ++p(1);
                if (result >= IdentifyState::completely)
                {
                    ++p;
                }
            }
        }
        txt(i18n::s.get("core.talk.npc.wizard.identify.count", p(0), p(1)));
        buff = i18n::s.get("core.talk.npc.wizard.identify.finished", speaker);
    }
    else
    {
        if (chatval_ == 14)
        {
            efp = 250;
        }
        else
        {
            efp = 1000;
        }
        efid = 411;
        magic(cdata.player(), speaker);
        if (efcancel == 1)
        {
            buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
            return TalkResult::talk_npc;
        }
        if (idtresult == IdentifyState::completely)
        {
            buff =
                i18n::s.get("core.talk.npc.wizard.identify.finished", speaker);
        }
        else
        {
            buff = i18n::s.get(
                "core.talk.npc.wizard.identify.need_investigate", speaker);
        }
        cdata.player().gold -= calcidentifyvalue(chatval_ - 14);
    }
    snd("core.paygold1");
    return TalkResult::talk_npc;
}



TalkResult talk_informer_list_adventurers(Character& speaker)
{
    list_adventurers();
    buff = i18n::s.get("core.talk.npc.informer.show_adventurers", speaker);
    return TalkResult::talk_npc;
}



TalkResult talk_healer_restore_attributes(Character& speaker)
{
    if (cdata.player().gold < calcrestorecost())
    {
        buff = i18n::s.get("core.ui.no_gold");
        return TalkResult::talk_npc;
    }
    snd("core.paygold1");
    cdata.player().gold -= calcrestorecost();
    tcbk = speaker.index;
    for (auto&& chara : cdata.player_and_allies())
    {
        if (chara.state() != Character::State::alive)
        {
            continue;
        }
        efid = 439;
        efp = 100;
        magic(cdata.player(), chara);
        efid = 440;
        efp = 100;
        magic(cdata.player(), chara);
    }
    talk_start();
    buff = i18n::s.get("core.talk.npc.healer.restore_attributes", speaker);
    return TalkResult::talk_npc;
}



TalkResult talk_trade(Character& speaker)
{
    invsubroutine = 1;
    for (const auto& item : g_inv.for_chara(speaker))
    {
        item->identify_state = IdentifyState::completely;
    }
    invctrl(0) = 20;
    invctrl(1) = 0;
    MenuResult result = ctrl_inventory(speaker).menu_result;
    if (!result.succeeded)
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
        return TalkResult::talk_npc;
    }
    buff = i18n::s.get("core.talk.npc.common.thanks", speaker);
    return TalkResult::talk_npc;
}



TalkResult talk_arena_master(Character& speaker, int chatval_)
{
    if (game_data.mount != 0)
    {
        int stat = cell_findspace(
            cdata.player().position.x, cdata.player().position.y, 1);
        if (stat == 0)
        {
            txt(i18n::s.get("core.magic.mount.no_place_to_get_off"));
            return TalkResult::talk_end;
        }
        cell_setchara(cdata[game_data.mount], rtval, rtval(1));
        txt(i18n::s.get("core.magic.mount.dismount", cdata[game_data.mount]));
        ride_end();
    }
    game_data.executing_immediate_quest_fame_gained = calc_gained_fame(
        cdata.player(),
        (220 - game_data.ranks.at(0) / 50) *
                (100 +
                 clamp(
                     area_data[game_data.current_map].winning_streak_in_arena,
                     0,
                     50)) /
                100 +
            2);
    listmax = 0;
    randomize(area_data[game_data.current_map].time_of_next_arena);
    if (chatval_ == 21)
    {
        if (area_data[game_data.current_map].time_of_next_arena >
            game_data.date.hours())
        {
            buff = i18n::s.get(
                "core.talk.npc.arena_master.enter.game_is_over", speaker);
            return TalkResult::talk_npc;
        }
        randomize(area_data[game_data.current_map].arena_random_seed);
        for (int cnt = 0; cnt < 50; ++cnt)
        {
            arenaop(0) = 0;
            arenaop(1) = (100 - game_data.ranks.at(0) / 100) / 3 + 1;
            arenaop(2) = 3;
            if (game_data.ranks.at(0) / 100 < 30)
            {
                arenaop(2) = 4;
            }
            if (game_data.ranks.at(0) / 100 < 10)
            {
                arenaop(2) = 5;
            }
            minlevel = arenaop(1) / 3 * 2;
            flt(arenaop(1));
            fixlv = static_cast<Quality>(arenaop(2));
            const auto chara = chara_create(56, 0, -3, 0);
            if (cmshade)
            {
                continue;
            }
            if (chara->level < minlevel)
            {
                continue;
            }
            if (chara->original_relationship != -3)
            {
                continue;
            }
            arenaop(1) = charaid2int(chara->id);
            buff = i18n::s.get(
                "core.talk.npc.arena_master.enter.target",
                chara->name,
                speaker);
            break;
        }
    }
    else
    {
        if (area_data[game_data.current_map].time_of_next_rumble >
            game_data.date.hours())
        {
            buff = i18n::s.get(
                "core.talk.npc.arena_master.enter.game_is_over", speaker);
            return TalkResult::talk_npc;
        }
        arenaop(0) = 1;
        arenaop(1) = (100 - game_data.ranks.at(0) / 100) / 2 + 1;
        buff = i18n::s.get(
            "core.talk.npc.arena_master.enter.target_group",
            arenaop(1),
            speaker);
    }
    ELONA_APPEND_RESPONSE(
        1, i18n::s.get("core.talk.npc.arena_master.enter.choices.enter"));
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.arena_master.enter.choices.leave"));
    chatesc = 1;

    chatval_ = talk_window_query(speaker);

    if (chatval_ != 1)
    {
        buff = i18n::s.get("core.talk.npc.arena_master.enter.cancel", speaker);
        return TalkResult::talk_npc;
    }
    if (arenaop == 0)
    {
        area_data[game_data.current_map].time_of_next_arena =
            game_data.date.hours() + 24;
    }
    if (arenaop == 1)
    {
        area_data[game_data.current_map].time_of_next_rumble =
            game_data.date.hours() + 24;
    }
    game_data.executing_immediate_quest_type = 1;
    game_data.executing_immediate_quest_show_hunt_remain = 1;
    game_data.executing_immediate_quest = 0;
    game_data.executing_immediate_quest_status = 1;
    map_prepare_for_travel_with_prev(static_cast<int>(mdata_t::MapId::arena));
    chatteleport = 1;
    return TalkResult::talk_end;
}



TalkResult talk_pet_arena_master(Character& speaker, int chatval_)
{
    game_data.executing_immediate_quest_fame_gained = calc_gained_fame(
        cdata.player(),
        (220 - game_data.ranks.at(1) / 50) *
                (50 +
                 clamp(
                     area_data[game_data.current_map]
                         .winning_streak_in_pet_arena,
                     0,
                     50)) /
                100 +
            2);
    listmax = 0;
    if (chatval_ == 40)
    {
        arenaop(0) = 0;
        arenaop(1) = 1;
        arenaop(2) = (100 - game_data.ranks.at(1) / 100) / 3 * 2 + 3;
        buff = i18n::s.get(
            "core.talk.npc.pet_arena_master.register.target",
            arenaop(2),
            speaker);
    }
    if (chatval_ == 41)
    {
        arenaop(0) = 1;
        arenaop(1) = 2;
        arenaop(2) = (100 - game_data.ranks.at(1) / 100) / 2 + 1;
        arenaop(1) = rnd(7) + 2;
        buff = i18n::s.get(
            "core.talk.npc.pet_arena_master.register.target_group",
            arenaop(1),
            arenaop(2),
            speaker);
    }
    ELONA_APPEND_RESPONSE(
        1,
        i18n::s.get("core.talk.npc.pet_arena_master.register.choices.enter"));
    ELONA_APPEND_RESPONSE(
        0,
        i18n::s.get("core.talk.npc.pet_arena_master.register.choices.leave"));
    chatesc = 1;

    chatval_ = talk_window_query(speaker);

    if (chatval_ != 1)
    {
        buff = i18n::s.get("core.talk.npc.arena_master.enter.cancel", speaker);
        return TalkResult::talk_npc;
    }
    DIM2(followerexist, 16);
    for (const auto& chara : cdata.player_and_allies())
    {
        followerexist(chara.index) = static_cast<int>(chara.state());
    }
    int stat = ctrl_ally(ControlAllyOperation::pet_arena);
    if (stat == -1)
    {
        buff = i18n::s.get("core.talk.npc.arena_master.enter.cancel", speaker);
        return TalkResult::talk_npc;
    }
    game_data.executing_immediate_quest_type = 2;
    game_data.executing_immediate_quest_show_hunt_remain = 0;
    game_data.executing_immediate_quest = 0;
    game_data.executing_immediate_quest_status = 1;
    map_prepare_for_travel_with_prev(
        static_cast<int>(mdata_t::MapId::pet_arena));
    chatteleport = 1;
    return TalkResult::talk_end;
}



TalkResult talk_pet_arena_master_score(Character& speaker)
{
    buff = i18n::s.get(
        "core.talk.npc.arena_master.streak",
        area_data[game_data.current_map].winning_streak_in_pet_arena,
        speaker);
    return TalkResult::talk_npc;
}



TalkResult talk_arena_master_score(Character& speaker)
{
    buff = i18n::s.get(
        "core.talk.npc.arena_master.streak",
        area_data[game_data.current_map].winning_streak_in_arena,
        speaker);
    return TalkResult::talk_npc;
}



TalkResult talk_quest_delivery(
    Character& speaker,
    const ItemRef& item_to_deliver)
{
    txt(i18n::s.get("core.talk.npc.common.hand_over", item_to_deliver));
    const auto slot = inv_make_free_slot_force(g_inv.for_chara(speaker));
    const auto handed_over_item = item_separate(item_to_deliver, slot, 1);
    chara_set_ai_item(speaker, handed_over_item);
    rq = deliver;
    quest_set_data(speaker, 3);
    quest_complete();
    refresh_burden_state();
    return TalkResult::talk_npc;
}



TalkResult talk_quest_supply(Character& speaker, const ItemRef& item_to_supply)
{
    txt(i18n::s.get("core.talk.npc.common.hand_over", item_to_supply));
    const auto slot = inv_make_free_slot_force(g_inv.for_chara(speaker));
    const auto handed_over_item = item_separate(item_to_supply, slot, 1);
    speaker.was_passed_item_by_you_just_now() = true;
    chara_set_ai_item(speaker, handed_over_item);
    quest_set_data(speaker, 3);
    quest_complete();
    refresh_burden_state();
    return TalkResult::talk_npc;
}



TalkResult talk_shop_attack(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.shop.attack.dialog", speaker);
    ELONA_APPEND_RESPONSE(
        1, i18n::s.get("core.talk.npc.shop.attack.choices.attack"));
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.shop.attack.choices.go_back"));
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ != 1)
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
        return TalkResult::talk_npc;
    }
    go_hostile();
    return TalkResult::talk_end;
}



TalkResult talk_guard_return_item(Character& speaker)
{
    listmax = 0;
    auto wallet_opt = itemfind(g_inv.pc(), "core.wallet");
    if (!wallet_opt)
    {
        wallet_opt = itemfind(g_inv.pc(), "core.suitcase");
    }
    const auto wallet = wallet_opt.unwrap();
    wallet->modify_number(-1);
    if (wallet->param1 == 0)
    {
        buff = i18n::s.get("core.talk.npc.guard.lost.empty.dialog", speaker);
        ELONA_APPEND_RESPONSE(
            0, i18n::s.get("core.talk.npc.guard.lost.empty.response"));
        chatesc = 1;
        talk_window_query(speaker);
        modify_karma(cdata.player(), -5);
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.guard.lost.dialog", speaker);
        ELONA_APPEND_RESPONSE(
            0, i18n::s.get("core.talk.npc.guard.lost.response"));
        chatesc = 1;
        talk_window_query(speaker);
        modify_karma(cdata.player(), 5);
        ++game_data.lost_wallet_count;
        if (game_data.lost_wallet_count >= 4)
        {
            listmax = 0;
            buff = i18n::s.get_enum(
                "core.talk.npc.guard.lost.found_often.dialog", 0, speaker);
            ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
            chatesc = 1;
            talk_window_query(speaker);
            if (scenemode)
            {
                if (scene_cut == 1)
                {
                    return TalkResult::talk_end;
                }
            }
            buff = i18n::s.get_enum(
                "core.talk.npc.guard.lost.found_often.dialog", 1);
            ELONA_APPEND_RESPONSE(
                0,
                i18n::s.get("core.talk.npc.guard.lost.found_often.response"));
            chatesc = 1;
            talk_window_query(speaker);
            modify_karma(cdata.player(), -10);
        }
    }
    refresh_burden_state();
    buff = "";
    return TalkResult::talk_npc;
}



TalkResult talk_bartender_call_ally(Character& speaker)
{
    int stat = ctrl_ally(ControlAllyOperation::call_back);
    if (stat != -1)
    {
        if (cdata[stat].state() == Character::State::alive)
        {
            buff = i18n::s.get(
                "core.talk.npc.bartender.call_ally.no_need", speaker);
            return TalkResult::talk_npc;
        }
        listmax = 0;
        buff = i18n::s.get(
            "core.talk.npc.bartender.call_ally.cost",
            calc_resurrection_value(cdata[stat]),
            speaker);
        if (cdata.player().gold >= calc_resurrection_value(cdata[stat]))
        {
            ELONA_APPEND_RESPONSE(
                1,
                i18n::s.get("core.talk.npc.bartender.call_ally.choices.pay"));
        }
        ELONA_APPEND_RESPONSE(
            0,
            i18n::s.get("core.talk.npc.bartender.call_ally.choices.go_back"));
        chatesc = 1;
        int chatval_ = talk_window_query(speaker);
        if (chatval_ == 1)
        {
            snd("core.paygold1");
            cdata.player().gold -= calc_resurrection_value(cdata[stat]);
            buff = i18n::s.get(
                "core.talk.npc.bartender.call_ally.brings_back",
                speaker,
                cdata[stat]);
            revive_character(cdata[stat]);
        }
        else
        {
            buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
        }
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_ally_order_wait(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.ally.wait_at_town", speaker);
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    cell_data.at(speaker.position.x, speaker.position.y).chara_index_plus_one =
        0;
    speaker.set_state(Character::State::pet_waiting);
    speaker.current_map = 0;
    return TalkResult::talk_end;
}



TalkResult talk_ally_abandon(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.ally.abandon.prompt", speaker);
    ELONA_APPEND_RESPONSE(
        1, i18n::s.get("core.talk.npc.ally.abandon.choices.yes"));
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.ally.abandon.choices.no"));
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        txt(i18n::s.get("core.talk.npc.ally.abandon.you_abandoned", speaker));
        cell_data.at(speaker.position.x, speaker.position.y)
            .chara_index_plus_one = 0;
        chara_delete(speaker);
        return TalkResult::talk_end;
    }
    buff = "";
    return TalkResult::talk_npc;
}



TalkResult talk_slave_buy(Character& speaker, int chatval_)
{
    for (int cnt = 0; cnt < 10; ++cnt)
    {
        flt(cdata.player().level / 2 + 5);
        fixlv = Quality::good;
        if (chatval_ == 36)
        {
            fltn(u8"man"s);
        }
        else
        {
            fltn(u8"horse"s);
        }
        chara_create(56, 0, -3, 0);
        if (cdata.tmp().level == 0)
        {
            chara_vanquish(cdata.tmp());
            continue;
        }
        break;
    }
    listmax = 0;
    buff = i18n::s.get(
        "core.talk.npc.slave_trader.buy.cost",
        cnven(cdata[56].name),
        calc_slave_value(cdata.tmp()),
        speaker);
    if (cdata.player().gold >= calc_slave_value(cdata.tmp()))
    {
        ELONA_APPEND_RESPONSE(
            1, i18n::s.get("core.talk.npc.slave_trader.buy.choices.pay"));
    }
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.slave_trader.buy.choices.go_back"));
    chatesc = 1;

    chatval_ = talk_window_query(speaker);

    if (chatval_ == 1)
    {
        txt(i18n::s.get(
            "core.talk.npc.slave_trader.buy.you_buy", cnven(cdata[56].name)));
        snd("core.paygold1");
        cdata.player().gold -= calc_slave_value(cdata.tmp());
        new_ally_joins(cdata.tmp());
        buff = i18n::s.get("core.talk.npc.common.thanks", speaker);
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_slave_sell(Character& speaker)
{
    int stat = ctrl_ally(ControlAllyOperation::sell);
    if (stat != -1)
    {
        listmax = 0;
        buff = i18n::s.get(
            "core.talk.npc.slave_trader.sell.price",
            (calc_slave_value(cdata[stat]) * 2 / 3),
            speaker);
        ELONA_APPEND_RESPONSE(
            1, i18n::s.get("core.talk.npc.slave_trader.sell.choices.deal"));
        ELONA_APPEND_RESPONSE(
            0, i18n::s.get("core.talk.npc.slave_trader.sell.choices.go_back"));
        chatesc = 1;
        int chatval_ = talk_window_query(speaker);
        if (chatval_ == 1)
        {
            txt(i18n::s.get(
                "core.talk.npc.slave_trader.sell.you_sell_off",
                cnven(cdata[stat].name)));
            snd("core.getgold1");
            earn_gold(cdata.player(), calc_slave_value(cdata[stat]) * 2 / 3);
            if (cdata[stat].state() == Character::State::alive)
            {
                cell_data.at(cdata[stat].position.x, cdata[stat].position.y)
                    .chara_index_plus_one = 0;
            }
            if (cdata[stat].is_escorted() == 1)
            {
                event_add(15, charaid2int(cdata[stat].id));
            }
            chara_delete(cdata[stat]);
            buff = i18n::s.get("core.talk.npc.common.thanks", speaker);
        }
        else
        {
            buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
        }
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_ally_marriage(Character& speaker)
{
    if (speaker.impression < 200)
    {
        buff = i18n::s.get("core.talk.npc.ally.marriage.refuses", speaker);
        return TalkResult::talk_npc;
    }
    speaker.is_married() = true;
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.ally.marriage.accepts");
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    marry = speaker.index;
    event_add(13);
    return TalkResult::talk_end;
}



TalkResult talk_ally_gene(Character& speaker)
{
    /*
    if (game_data.current_map == mdata_t::MapId::shelter_)
    {
        listmax = 0;
        buff = i18n::s.get("core.talk.npc.ally.make_gene.refuses");
        ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
        chatesc = 1;
        talk_window_query(speaker);
        if (scenemode)
        {
            if (scene_cut == 1)
            {
                return TalkResult::talk_end;
            }
        }
        return TalkResult::talk_end;
    }
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.ally.make_gene.accepts");
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    speaker.has_made_gene() = true;
    game_data.character_and_status_for_gene = speaker.index;
    return TalkResult::talk_end;
    */

    listmax = 0;
    buff =
        "Making a nege is disabled in this version. Please wait for a future release.";
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
    chatesc = 1;
    talk_window_query(speaker);
    return TalkResult::talk_end;
}



TalkResult talk_innkeeper_shelter(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.innkeeper.go_to_shelter", speaker);
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    map_prepare_for_travel_with_prev(
        static_cast<int>(mdata_t::MapId::shelter_));
    chatteleport = 1;
    snd("core.exitmap1");
    return TalkResult::talk_end;
}



TalkResult talk_servant_fire(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.servant.fire.prompt", speaker);
    ELONA_APPEND_RESPONSE(
        1, i18n::s.get("core.talk.npc.servant.fire.choices.yes"));
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.servant.fire.choices.no"));
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        txt(i18n::s.get("core.talk.npc.servant.fire.you_dismiss", speaker));
        chara_vanquish(speaker);
        calccosthire();
        return TalkResult::talk_end;
    }
    buff = "";
    return TalkResult::talk_npc;
}



TalkResult talk_maid_think_of_house_name(Character& speaker)
{
    mdatan(0) = random_title(RandomTitleType::character);
    if (rnd(5))
    {
        mdatan(0) = i18n::s.get(
            "core.talk.npc.maid.think_of_house_name.suffixes", mdatan(0));
    }
    screenupdate = -1;
    update_entire_screen();
    buff = i18n::s.get(
        "core.talk.npc.maid.think_of_house_name.come_up_with",
        mdatan(0),
        speaker);
    return TalkResult::talk_npc;
}



TalkResult talk_sister_buy_indulgence(Character& speaker)
{
    if (cdata.player().karma >= -30)
    {
        buff =
            i18n::s.get("core.talk.npc.sister.buy_indulgence.karma_is_not_low");
        return TalkResult::talk_npc;
    }
    listmax = 0;
    buff = i18n::s.get(
        "core.talk.npc.sister.buy_indulgence.cost", calcguiltvalue(), speaker);
    if (cdata.player().gold >= calcguiltvalue())
    {
        ELONA_APPEND_RESPONSE(
            1, i18n::s.get("core.talk.npc.sister.buy_indulgence.choices.buy"));
    }
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.sister.buy_indulgence.choices.go_back"));
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        snd("core.paygold1");
        cdata.player().gold -= calcguiltvalue();
        modify_karma(cdata.player(), (cdata.player().karma - -30) * -1 + 1);
        buff = i18n::s.get("core.talk.npc.common.thanks", speaker);
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_informer_investigate_ally(Character& speaker)
{
    int stat = ctrl_ally(ControlAllyOperation::investigate);
    if (stat != -1)
    {
        listmax = 0;
        buff = i18n::s.get(
            "core.talk.npc.informer.investigate_ally.cost", speaker);
        if (cdata.player().gold >= 10000)
        {
            ELONA_APPEND_RESPONSE(
                1,
                i18n::s.get(
                    "core.talk.npc.informer.investigate_ally.choices.pay"));
        }
        ELONA_APPEND_RESPONSE(
            0,
            i18n::s.get(
                "core.talk.npc.informer.investigate_ally.choices.go_back"));
        chatesc = 1;
        int chatval_ = talk_window_query(speaker);
        if (chatval_ == 1)
        {
            snd("core.paygold1");
            cdata.player().gold -= 10000;
            snd("core.pop2");
            menu_character_sheet_investigate(cdata[stat]);
            talk_start();
            buff = "";
        }
        else
        {
            buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
        }
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_ally_silence(Character& speaker)
{
    if (speaker.is_silent() == 0)
    {
        speaker.is_silent() = true;
        buff = i18n::s.get("core.talk.npc.ally.silence.start", speaker);
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.ally.silence.stop", speaker);
        speaker.is_silent() = false;
    }
    return TalkResult::talk_npc;
}



TalkResult talk_adventurer_hire(Character& speaker)
{
    buff = i18n::s.get(
        "core.talk.npc.adventurer.hire.cost",
        calc_adventurer_hire_cost(speaker),
        speaker);
    if (cdata.player().gold >= calc_adventurer_hire_cost(speaker))
    {
        ELONA_APPEND_RESPONSE(
            1, i18n::s.get("core.talk.npc.adventurer.hire.choices.pay"));
    }
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.adventurer.hire.choices.go_back"));
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        snd("core.paygold1");
        cdata.player().gold -= calc_adventurer_hire_cost(speaker);
        speaker.relationship = 10;
        speaker.is_contracting() = true;
        speaker.period_of_contract = game_data.date.hours() + 168;
        ++speaker.hire_count;
        snd("core.pray1");
        txt(i18n::s.get("core.talk.npc.adventurer.hire.you_hired", speaker),
            Message::color{ColorIndex::orange});
        buff = i18n::s.get("core.talk.npc.common.thanks", speaker);
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_adventurer_join(Character& speaker)
{
    if (cdata.player().level * 3 / 2 + 10 < speaker.level)
    {
        buff = i18n::s.get("core.talk.npc.adventurer.join.too_weak", speaker);
        return TalkResult::talk_npc;
    }
    if (speaker.impression >= 200 && speaker.hire_count > 2)
    {
        listmax = 0;
        buff = i18n::s.get("core.talk.npc.adventurer.join.accept", speaker);
        ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
        chatesc = 1;
        talk_window_query(speaker);
        if (scenemode)
        {
            if (scene_cut == 1)
            {
                return TalkResult::talk_end;
            }
        }
        f = chara_get_free_slot_ally();
        if (f == 0)
        {
            buff = i18n::s.get(
                "core.talk.npc.adventurer.join.party_full", speaker);
            return TalkResult::talk_npc;
        }
        const auto ally = new_ally_joins(speaker);
        ally->role = Role::none;
        ally->current_map = 0;
        speaker.impression = 100;
        create_adventurer(speaker);
        return TalkResult::talk_end;
    }
    buff = i18n::s.get("core.talk.npc.adventurer.join.not_known", speaker);
    return TalkResult::talk_npc;
}



TalkResult talk_moyer_sell_paels_mom(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.moyer.sell_paels_mom.prompt");
    ELONA_APPEND_RESPONSE(
        1, i18n::s.get("core.talk.npc.moyer.sell_paels_mom.choices.sell"));
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.moyer.sell_paels_mom.choices.go_back"));
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        txt(i18n::s.get("core.talk.npc.moyer.sell_paels_mom.you_sell"));
        modify_karma(cdata.player(), -20);
        snd("core.getgold1");
        earn_gold(cdata.player(), 50000);
        game_data.quest_flags.pael_and_her_mom = 1002;
        const auto lily = chara_find("core.lily");
        assert(lily);
        lily->ai_calm = 3;
        lily->relationship = 0;
        lily->initial_position.x = 48;
        lily->initial_position.y = 18;
        cell_movechara(*lily, 48, 18);
        buff = i18n::s.get("core.talk.npc.common.thanks", speaker);
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_wizard_return(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.wizard.return", speaker);
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    try_to_return();
    return TalkResult::talk_end;
}



TalkResult talk_shop_reload_ammo(Character& speaker)
{
    if (calc_ammo_reloading_cost(cdata.player()) == 0)
    {
        buff = i18n::s.get("core.talk.npc.shop.ammo.no_ammo", speaker);
        return TalkResult::talk_npc;
    }
    buff = i18n::s.get(
        "core.talk.npc.shop.ammo.cost",
        calc_ammo_reloading_cost(cdata.player()),
        speaker);
    if (cdata.player().gold >= calc_ammo_reloading_cost(cdata.player()))
    {
        ELONA_APPEND_RESPONSE(
            1, i18n::s.get("core.talk.npc.shop.ammo.choices.pay"));
    }
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.shop.ammo.choices.go_back"));
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        snd("core.paygold1");
        cdata.player().gold -= calc_ammo_reloading_cost(cdata.player());
        p = calc_ammo_reloading_cost(cdata.player(), true);
        buff = i18n::s.get("core.talk.npc.common.thanks", speaker);
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_spell_writer_reserve(Character& speaker)
{
    (void)speaker;

    screenupdate = -1;
    update_screen();
    invctrl = 0;
    show_spell_writer_menu();
    buff = "";
    return TalkResult::talk_npc;
}



TalkResult talk_sex(Character& speaker)
{
    ELONA_APPEND_RESPONSE(
        1, i18n::s.get("core.talk.npc.common.sex.choices.accept"));
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.common.sex.choices.go_back"));
    buff = i18n::s.get("core.talk.npc.common.sex.prompt", speaker);
    int chatval_ = talk_window_query(speaker);
    if (chatval_ != 1)
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
        return TalkResult::talk_npc;
    }
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.common.sex.start", speaker);
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.talk.npc.common.sex.response"));
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    activity_sex(cdata.player(), speaker);
    return TalkResult::talk_end;
}



TalkResult talk_result_maid_chase_out(Character& speaker)
{
    --game_data.number_of_waiting_guests;
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.maid.do_not_meet", speaker);
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    buff = "";
    return TalkResult::talk_npc;
}



TalkResult talk_prostitute_buy(Character& speaker)
{
    int sexvalue =
        speaker.get_skill(17).level * 25 + 100 + cdata.player().fame / 10;
    if (cdata.player().gold >= sexvalue)
    {
        ELONA_APPEND_RESPONSE(
            1, i18n::s.get("core.talk.npc.common.sex.choices.accept"));
    }
    ELONA_APPEND_RESPONSE(
        0, i18n::s.get("core.talk.npc.common.sex.choices.go_back"));
    buff = i18n::s.get("core.talk.npc.prostitute.buy", sexvalue, speaker);
    int chatval_ = talk_window_query(speaker);
    if (chatval_ != 1)
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
        return TalkResult::talk_npc;
    }
    snd("core.paygold1");
    cdata.player().gold -= sexvalue;
    earn_gold(speaker, sexvalue);
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.common.sex.start", speaker);
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.talk.npc.common.sex.response"));
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    activity_sex(speaker, cdata.player());
    return TalkResult::talk_end;
}



TalkResult talk_caravan_master_hire(Character& speaker)
{
    map_set_caravan_destinations();

    for (int cnt = 0; cnt < 10; ++cnt)
    {
        if (p(cnt) == 0)
        {
            ELONA_APPEND_RESPONSE(
                0,
                i18n::s.get(
                    "core.talk.npc.caravan_master.hire.choices.go_back"));
            break;
        }
        ELONA_APPEND_RESPONSE(p(cnt), mapname(p(cnt)));
    }
    buff = i18n::s.get("core.talk.npc.caravan_master.hire.tset");
    int chatval_ = talk_window_query(speaker);
    if (chatval_ <= 0)
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
        return TalkResult::talk_npc;
    }
    game_data.destination_map = area_data[chatval_].outer_map;
    game_data.destination_dungeon_level = 1;
    levelexitby = 4;
    game_data.reset_world_map_in_diastrophism_flag = 1;
    game_data.destination_outer_map = area_data[chatval_].outer_map;
    game_data.pc_x_in_world_map = area_data[chatval_].position.x;
    game_data.pc_y_in_world_map = area_data[chatval_].position.y;
    fixtransfermap = 1;
    chatteleport = 1;
    return TalkResult::talk_end;
}



TalkResult talk_guard_where_is(Character& speaker, int chatval_)
{
    talk_guide_quest_client();
    const auto& chara_you_ask = cdata[rtval(chatval_ - 10000)];
    p = direction(
        cdata.player().position.x,
        cdata.player().position.y,
        chara_you_ask.position.x,
        chara_you_ask.position.y);
    if (p == 1)
    {
        s = i18n::s.get("core.talk.npc.guard.where_is.direction.west");
    }
    else if (p == 2)
    {
        s = i18n::s.get("core.talk.npc.guard.where_is.direction.east");
    }
    else if (p == 3)
    {
        s = i18n::s.get("core.talk.npc.guard.where_is.direction.north");
    }
    else
    {
        s = i18n::s.get("core.talk.npc.guard.where_is.direction.south");
    }
    p = dist(cdata.player().position, chara_you_ask.position);

    if (chara_you_ask == speaker)
    {
        s = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    else if (chara_you_ask.state() != Character::State::alive)
    {
        s = i18n::s.get("core.talk.npc.guard.where_is.dead", speaker);
    }
    else if (p < 6)
    {
        s = i18n::s.get(
            "core.talk.npc.guard.where_is.very_close",
            s(0),
            chara_you_ask,
            speaker);
    }
    else if (p < 12)
    {
        s = i18n::s.get(
            "core.talk.npc.guard.where_is.close", s(0), chara_you_ask, speaker);
    }
    else if (p < 20)
    {
        s = i18n::s.get(
            "core.talk.npc.guard.where_is.moderate",
            s(0),
            chara_you_ask,
            speaker);
    }
    else if (p < 35)
    {
        s = i18n::s.get(
            "core.talk.npc.guard.where_is.far", s(0), chara_you_ask, speaker);
    }
    else
    {
        s = i18n::s.get(
            "core.talk.npc.guard.where_is.very_far",
            s(0),
            chara_you_ask,
            speaker);
    }
    buff = s;
    return TalkResult::talk_npc;
}



TalkResult talk_accepted_quest(Character& speaker)
{
    if (quest_data[rq].id == 1001 || quest_data[rq].id == 1010)
    {
        listmax = 0;
        buff = i18n::s.get("core.talk.npc.quest_giver.accept.hunt", speaker);
        list(0, listmax) = 0;
        listn(0, listmax) = i18n::s.get("core.ui.more");
        ++listmax;
        chatesc = 1;
        talk_window_query(speaker);
        if (scenemode)
        {
            if (scene_cut == 1)
            {
                return TalkResult::talk_end;
            }
        }
    }
    if (quest_data[rq].id == 1006)
    {
        listmax = 0;
        buff = i18n::s.get("core.talk.npc.quest_giver.accept.harvest", speaker);
        list(0, listmax) = 0;
        listn(0, listmax) = i18n::s.get("core.ui.more");
        ++listmax;
        chatesc = 1;
        talk_window_query(speaker);
        if (scenemode)
        {
            if (scene_cut == 1)
            {
                return TalkResult::talk_end;
            }
        }
    }
    if (quest_data[rq].id == 1009)
    {
        listmax = 0;
        buff = i18n::s.get("core.talk.npc.quest_giver.accept.party", speaker);
        list(0, listmax) = 0;
        listn(0, listmax) = i18n::s.get("core.ui.more");
        ++listmax;
        chatesc = 1;
        talk_window_query(speaker);
        if (scenemode)
        {
            if (scene_cut == 1)
            {
                return TalkResult::talk_end;
            }
        }
    }
    game_data.executing_immediate_quest_type = quest_data[rq].id;
    game_data.executing_immediate_quest_show_hunt_remain =
        quest_data[rq].client_chara_type;
    game_data.executing_immediate_quest = rq;
    game_data.executing_immediate_quest_status = 1;
    map_prepare_for_travel_with_prev(static_cast<int>(mdata_t::MapId::quest));
    chatteleport = 1;
    return TalkResult::talk_end;
}



TalkResult talk_trainer(Character& speaker, bool is_training)
{
    tcbk = speaker.index;
    menucycle = 0;
    optional<int> selected_skill_opt =
        menu_character_sheet_trainer(is_training);
    talk_start();
    if (!selected_skill_opt)
    {
        buff = i18n::s.get("core.talk.npc.trainer.leave", speaker);
        return TalkResult::talk_npc;
    }
    int selected_skill = *selected_skill_opt;
    listmax = 0;
    if (is_training)
    {
        buff = i18n::s.get(
            "core.talk.npc.trainer.cost.training",
            the_ability_db.get_text(selected_skill, "name"),
            calc_skill_training_cost(selected_skill, cdata.player()),
            speaker);
        if (cdata.player().platinum_coin >=
            calc_skill_training_cost(selected_skill, cdata.player()))
        {
            list(0, listmax) = 1;
            listn(0, listmax) =
                i18n::s.get("core.talk.npc.trainer.choices.train.accept");
            ++listmax;
        }
    }
    else
    {
        buff = i18n::s.get(
            "core.talk.npc.trainer.cost.learning",
            the_ability_db.get_text(selected_skill, "name"),
            calc_skill_learning_cost(selected_skill, cdata.player()),
            speaker);
        if (cdata.player().platinum_coin >=
            calc_skill_learning_cost(selected_skill, cdata.player()))
        {
            list(0, listmax) = 1;
            listn(0, listmax) =
                i18n::s.get("core.talk.npc.trainer.choices.learn.accept");
            ++listmax;
        }
    }
    list(0, listmax) = 0;
    listn(0, listmax) = i18n::s.get("core.talk.npc.trainer.choices.go_back");
    ++listmax;
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        snd("core.paygold1");
        if (is_training)
        {
            cdata.player().platinum_coin -=
                calc_skill_training_cost(selected_skill, cdata.player());
            modify_potential(
                cdata.player(),
                selected_skill,
                clamp(
                    15 -
                        cdata.player().get_skill(selected_skill).potential / 15,
                    2,
                    15));
            buff =
                i18n::s.get("core.talk.npc.trainer.finish.training", speaker);
        }
        else
        {
            cdata.player().platinum_coin -=
                calc_skill_learning_cost(selected_skill, cdata.player());
            chara_gain_skill(cdata.player(), selected_skill);
            ++game_data.number_of_learned_skills_by_trainer;
            buff =
                i18n::s.get("core.talk.npc.trainer.finish.learning", speaker);
        }
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.trainer.leave", speaker);
    }
    return TalkResult::talk_npc;
}



TalkResult talk_trainer_train_skill(Character& speaker)
{
    return talk_trainer(speaker, true);
}



TalkResult talk_trainer_learn_skill(Character& speaker)
{
    return talk_trainer(speaker, false);
}



TalkResult talk_invest(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get(
        "core.talk.npc.shop.invest.ask", calcinvestvalue(speaker), speaker);
    if (cdata.player().gold >= calcinvestvalue(speaker))
    {
        list(0, listmax) = 1;
        listn(0, listmax) =
            i18n::s.get("core.talk.npc.shop.invest.choices.invest");
        ++listmax;
    }
    list(0, listmax) = 0;
    listn(0, listmax) = i18n::s.get("core.talk.npc.shop.invest.choices.reject");
    ++listmax;
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        snd("core.paygold1");
        cdata.player().gold -= calcinvestvalue(speaker);
        chara_gain_exp_investing(cdata.player());
        speaker.shop_rank += rnd(2) + 2;
        buff = i18n::s.get("core.talk.npc.common.thanks", speaker);
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    if (game_data.current_map == mdata_t::MapId::your_home)
    {
        calccosthire();
    }
    return TalkResult::talk_npc;
}



TalkResult talk_finish_escort(Character& speaker)
{
    listmax = 0;
    buff = i18n::s.get("core.talk.npc.quest_giver.finish.escort", speaker);
    list(0, listmax) = 0;
    listn(0, listmax) = i18n::s.get("core.ui.more");
    ++listmax;
    chatesc = 1;
    talk_window_query(speaker);
    if (scenemode)
    {
        if (scene_cut == 1)
        {
            return TalkResult::talk_end;
        }
    }
    return TalkResult::talk_end;
}



TalkResult talk_quest_giver(Character& speaker)
{
    if (quest_data[rq].progress == 1)
    {
        buff = i18n::s.get("core.talk.npc.quest_giver.about.during", speaker);
        return TalkResult::talk_npc;
    }
    quest_set_data(speaker, 1);
    listmax = 0;
    list(0, listmax) = 1;
    listn(0, listmax) =
        i18n::s.get("core.talk.npc.quest_giver.about.choices.take");
    ++listmax;
    list(0, listmax) = 0;
    listn(0, listmax) =
        i18n::s.get("core.talk.npc.quest_giver.about.choices.leave");
    ++listmax;
    chatesc = 1;
    int chatval_ = talk_window_query(speaker);
    if (chatval_ == 1)
    {
        p = 0;
        for (int cnt = 0, cnt_end = (game_data.number_of_existing_quests);
             cnt < cnt_end;
             ++cnt)
        {
            if (quest_data[cnt].id == 0)
            {
                continue;
            }
            if (quest_data[cnt].progress != 0)
            {
                ++p;
            }
        }
        if (p >= 5)
        {
            buff = i18n::s.get(
                "core.talk.npc.quest_giver.about.too_many_unfinished", speaker);
            return TalkResult::talk_npc;
        }
        for (int cnt = 0; cnt < 5; ++cnt)
        {
            p = game_data.taken_quests.at(cnt);
            f = 0;
            for (int cnt = 0; cnt < 5; ++cnt)
            {
                if (game_data.taken_quests.at(cnt) == p)
                {
                    ++f;
                }
            }
            if (quest_data[p].progress == 0 || f > 1)
            {
                game_data.taken_quests.at(cnt) = rq;
                break;
            }
        }
        if (quest_data[rq].id == 1002)
        {
            if (!g_inv.pc().has_free_slot())
            {
                buff = i18n::s.get(
                    "core.talk.npc.quest_giver.about.backpack_full", speaker);
                return TalkResult::talk_npc;
            }
        }
        if (quest_data[rq].id == 1007)
        {
            f = chara_get_free_slot_ally();
            if (f == 0)
            {
                buff = i18n::s.get(
                    "core.talk.npc.quest_giver.about.party_full", speaker);
                return TalkResult::talk_npc;
            }
            for (int cnt = 0;; ++cnt)
            {
                int chara_id;
                if (cnt == 99)
                {
                    chara_id = 35;
                }
                else
                {
                    chara_id = 0;
                }
                flt(quest_data[rq].difficulty + cnt, Quality::bad);
                fltn(u8"man"s);
                const auto chara = chara_create(56, chara_id, -3, 0);
                f = !!chara;
                if (f == 1)
                {
                    for (const auto& ally : cdata.player_and_allies())
                    {
                        if (ally.state() == Character::State::empty)
                        {
                            continue;
                        }
                        if (ally.id == chara->id && ally.is_escorted())
                        {
                            f = 0;
                            break;
                        }
                    }
                }
                if (f == 1)
                {
                    break;
                }
            }
            const auto ally = new_ally_joins(cdata.tmp());
            ally->is_escorted() = true;
            quest_data[rq].extra_info_2 = charaid2int(ally->id);
        }
        quest_data[rq].progress = 1;
        if (quest_data[rq].deadline_days == -1)
        {
            return talk_accepted_quest(speaker);
        }
        buff = i18n::s.get("core.talk.npc.quest_giver.about.thanks", speaker);
        if (quest_data[rq].id == 1002)
        {
            ++quest_data[quest_data[rq].target_chara_index]
                  .delivery_has_package_flag;
            flt();
            if (const auto item =
                    itemcreate_player_inv(quest_data[rq].target_item_id, 0))
            {
                txt(i18n::s.get(
                    "core.common.you_put_in_your_backpack", item.unwrap()));
                snd("core.inv");
                refresh_burden_state();
                buff = i18n::s.get(
                    "core.talk.npc.quest_giver.about.here_is_package", speaker);
            }
        }
    }
    else
    {
        buff = i18n::s.get("core.talk.npc.common.you_kidding", speaker);
    }
    return TalkResult::talk_npc;
}



/// Check whether character at `chara_index` is one of the targets of the
/// trading quests you are taking now.
bool _talk_check_trade(int chara_index)
{
    for (const auto& quest_index : game_data.taken_quests)
    {
        const auto& quest = quest_data[quest_index];
        if (quest.progress == 1 &&
            quest.originating_map_id == game_data.current_map &&
            game_data.current_dungeon_level == 1 &&
            chara_index == quest.target_chara_index)
        {
            return true;
        }
    }

    return false;
}

} // namespace



void talk_wrapper(Character& speaker, TalkResult initial)
{
    TalkResult result = initial;
    bool finished = false;
    while (!finished)
    {
        switch (result)
        {
        case TalkResult::talk_npc: result = talk_npc(speaker); break;
        case TalkResult::talk_unique: result = talk_unique(speaker); break;
        case TalkResult::talk_quest_giver:
            result = talk_quest_giver(speaker);
            break;
        case TalkResult::talk_house_visitor:
            result = talk_house_visitor(speaker);
            break;
        case TalkResult::talk_sleeping: result = talk_sleeping(speaker); break;
        case TalkResult::talk_busy: result = talk_busy(speaker); break;
        case TalkResult::talk_ignored: result = talk_ignored(speaker); break;
        case TalkResult::talk_finish_escort:
            result = talk_finish_escort(speaker);
            break;
        case TalkResult::talk_game_begin:
            result = talk_game_begin(speaker);
            break;
        case TalkResult::talk_more: result = talk_more(speaker); break;
        case TalkResult::talk_end:
            talk_end();
            finished = true;
            break;
        default: assert(0); break;
        }
    }
}



TalkResult talk_npc(Character& speaker)
{
    OptionalItemRef item_to_deliver;
    OptionalItemRef item_to_supply;

    listmax = 0;
    if (buff == ""s)
    {
        get_npc_talk(speaker);
        bool did_speak = chara_custom_talk(speaker, 106);
        if (did_speak)
        {
            text_replace_tags_in_quest_board(speaker);
        }
        if (speaker.interest > 0)
        {
            if (speaker.relationship != 10)
            {
                if (!speaker.is_player_or_ally())
                {
                    if (rnd(3) == 0)
                    {
                        if (speaker.impression < 100)
                        {
                            if (rnd_capped(
                                    cdata.player().get_skill(17).level + 1) >
                                10)
                            {
                                chara_modify_impression(speaker, rnd(3));
                            }
                            else
                            {
                                chara_modify_impression(speaker, rnd(3) * -1);
                            }
                        }
                    }
                    speaker.interest -= rnd(30);
                    speaker.time_interest_revive = game_data.date.hours() + 8;
                }
            }
        }
    }
    if (speaker.role == Role::maid)
    {
        if (game_data.number_of_waiting_guests > 0)
        {
            ELONA_APPEND_RESPONSE(
                58, i18n::s.get("core.talk.npc.maid.choices.meet_guest"));
            ELONA_APPEND_RESPONSE(
                59, i18n::s.get("core.talk.npc.maid.choices.do_not_meet"));
        }
    }
    if (speaker.interest > 0 && !chatval_unique_chara_id)
    {
        ELONA_APPEND_RESPONSE(
            1, i18n::s.get("core.talk.npc.common.choices.talk"));
    }
    if (is_shopkeeper(speaker.role))
    {
        ELONA_APPEND_RESPONSE(
            10, i18n::s.get("core.talk.npc.shop.choices.buy"));
        ELONA_APPEND_RESPONSE(
            11, i18n::s.get("core.talk.npc.shop.choices.sell"));
        if (speaker.role == Role::wandering_vendor)
        {
            ELONA_APPEND_RESPONSE(
                31, i18n::s.get("core.talk.npc.shop.choices.attack"));
        }
        if (speaker.role != Role::wandering_vendor &&
            speaker.role != Role::trader)
        {
            ELONA_APPEND_RESPONSE(
                12, i18n::s.get("core.talk.npc.shop.choices.invest"));
        }
    }
    if (speaker.role == Role::bartender)
    {
        ELONA_APPEND_RESPONSE(
            33, i18n::s.get("core.talk.npc.bartender.choices.call_ally"));
    }
    if (speaker.role == Role::slave_master)
    {
        if (chara_get_free_slot_ally() != 0)
        {
            ELONA_APPEND_RESPONSE(
                36, i18n::s.get("core.talk.npc.slave_trader.choices.buy"));
        }
        ELONA_APPEND_RESPONSE(
            37, i18n::s.get("core.talk.npc.slave_trader.choices.sell"));
    }
    if (speaker.role == Role::horse_master)
    {
        if (chara_get_free_slot_ally() != 0)
        {
            ELONA_APPEND_RESPONSE(
                57, i18n::s.get("core.talk.npc.horse_keeper.choices.buy"));
        }
    }
    if (speaker.is_player_or_ally())
    {
        if (speaker.is_escorted() == 0)
        {
            if (speaker.is_escorted_in_sub_quest() == 0)
            {
                ELONA_APPEND_RESPONSE(
                    34, i18n::s.get("core.talk.npc.ally.choices.wait_at_town"));
                if (speaker.is_married() == 0)
                {
                    ELONA_APPEND_RESPONSE(
                        38,
                        i18n::s.get(
                            "core.talk.npc.ally.choices.ask_for_marriage"));
                }
                else if (game_data.continuous_active_hours >= 15)
                {
                    ELONA_APPEND_RESPONSE(
                        39,
                        i18n::s.get("core.talk.npc.ally.choices.make_gene"));
                }
                if (speaker.can_talk != 0)
                {
                    if (speaker.is_silent() == 0)
                    {
                        ELONA_APPEND_RESPONSE(
                            48,
                            i18n::s.get(
                                "core.talk.npc.ally.choices.silence.start"));
                    }
                    else
                    {
                        ELONA_APPEND_RESPONSE(
                            48,
                            i18n::s.get(
                                "core.talk.npc.ally.choices.silence.stop"));
                    }
                }
                ELONA_APPEND_RESPONSE(
                    35, i18n::s.get("core.talk.npc.ally.choices.abandon"));
            }
        }
    }
    if (speaker.role == Role::blacksmith)
    {
        ELONA_APPEND_RESPONSE(
            54, i18n::s.get("core.talk.npc.shop.choices.ammo"));
    }
    if (speaker.role == Role::innkeeper)
    {
        ELONA_APPEND_RESPONSE(
            13,
            i18n::s.get("core.talk.npc.innkeeper.choices.eat") + u8" ("s +
                calcmealvalue() + i18n::s.get("core.ui.gold") + u8")"s);
        if (game_data.weather == 1 || game_data.weather == 4 ||
            game_data.weather == 2)
        {
            ELONA_APPEND_RESPONSE(
                43,
                i18n::s.get("core.talk.npc.innkeeper.choices.go_to_shelter"));
        }
    }
    if (speaker.role == Role::appraiser)
    {
        ELONA_APPEND_RESPONSE(
            14,
            i18n::s.get("core.talk.npc.wizard.choices.identify") + u8" ("s +
                calcidentifyvalue(0) + i18n::s.get("core.ui.gold") + u8")"s);
        ELONA_APPEND_RESPONSE(
            15,
            i18n::s.get("core.talk.npc.wizard.choices.identify_all") + u8" ("s +
                calcidentifyvalue(1) + i18n::s.get("core.ui.gold") + u8")"s);
        ELONA_APPEND_RESPONSE(
            16,
            i18n::s.get("core.talk.npc.wizard.choices.investigate") + u8" ("s +
                calcidentifyvalue(2) + i18n::s.get("core.ui.gold") + u8")"s);
    }
    if (speaker.role == Role::trainer)
    {
        ELONA_APPEND_RESPONSE(
            17, i18n::s.get("core.talk.npc.trainer.choices.train.ask"));
        ELONA_APPEND_RESPONSE(
            30, i18n::s.get("core.talk.npc.trainer.choices.learn.ask"));
    }
    if (speaker.role == Role::informer)
    {
        ELONA_APPEND_RESPONSE(
            18, i18n::s.get("core.talk.npc.informer.choices.show_adventurers"));
        ELONA_APPEND_RESPONSE(
            47, i18n::s.get("core.talk.npc.informer.choices.investigate_ally"));
    }
    if (speaker.role == Role::healer)
    {
        ELONA_APPEND_RESPONSE(
            19,
            i18n::s.get("core.talk.npc.healer.choices.restore_attributes") +
                u8"("s + calcrestorecost() + i18n::s.get("core.ui.gold") +
                u8")"s);
    }
    if (speaker.role == Role::adventurer)
    {
        ELONA_APPEND_RESPONSE(
            20, i18n::s.get("core.talk.npc.common.choices.trade"));
        if (speaker.is_contracting() == 0)
        {
            ELONA_APPEND_RESPONSE(
                50, i18n::s.get("core.talk.npc.adventurer.choices.hire"));
            ELONA_APPEND_RESPONSE(
                51, i18n::s.get("core.talk.npc.adventurer.choices.join"));
        }
    }
    if (speaker.role == Role::arena_master)
    {
        ELONA_APPEND_RESPONSE(
            21, i18n::s.get("core.talk.npc.arena_master.choices.enter_duel"));
        ELONA_APPEND_RESPONSE(
            22, i18n::s.get("core.talk.npc.arena_master.choices.enter_rumble"));
        ELONA_APPEND_RESPONSE(
            23, i18n::s.get("core.talk.npc.arena_master.choices.score"));
    }
    if (speaker.role == Role::pet_arena_master)
    {
        ELONA_APPEND_RESPONSE(
            40,
            i18n::s.get(
                "core.talk.npc.pet_arena_master.choices.register_duel"));
        ELONA_APPEND_RESPONSE(
            41,
            i18n::s.get(
                "core.talk.npc.pet_arena_master.choices.register_team"));
        ELONA_APPEND_RESPONSE(
            42, i18n::s.get("core.talk.npc.arena_master.choices.score"));
    }
    if (speaker.role == Role::maid)
    {
        ELONA_APPEND_RESPONSE(
            45, i18n::s.get("core.talk.npc.maid.choices.think_of_house_name"));
    }
    if (speaker.role == Role::sister)
    {
        ELONA_APPEND_RESPONSE(
            46, i18n::s.get("core.talk.npc.sister.choices.buy_indulgence"));
    }
    if (_talk_check_trade(speaker.index))
    {
        ELONA_APPEND_RESPONSE(
            20, i18n::s.get("core.talk.npc.common.choices.trade"));
    }
    if (speaker.role == Role::guard)
    {
        int stat = talk_guide_quest_client();
        if (stat != 0)
        {
            for (int cnt = 0, cnt_end = (stat); cnt < cnt_end; ++cnt)
            {
                ELONA_APPEND_RESPONSE(
                    10000 + cnt,
                    i18n::s.get(
                        "core.talk.npc.guard.choices.where_is",
                        cdata[rtval(cnt)]));
            }
        }
        if (itemfind(g_inv.pc(), "core.wallet"))
        {
            ELONA_APPEND_RESPONSE(
                32, i18n::s.get("core.talk.npc.guard.choices.lost_wallet"));
        }
        else if (itemfind(g_inv.pc(), "core.suitcase"))
        {
            ELONA_APPEND_RESPONSE(
                32, i18n::s.get("core.talk.npc.guard.choices.lost_suitcase"));
        }
    }
    if (speaker.role == Role::returner)
    {
        ELONA_APPEND_RESPONSE(
            53, i18n::s.get("core.talk.npc.wizard.choices.return"));
    }
    if (speaker.role == Role::spell_writer)
    {
        if (game_data.guild.belongs_to_mages_guild != 0)
        {
            ELONA_APPEND_RESPONSE(
                55, i18n::s.get("core.talk.npc.spell_writer.choices.reserve"));
        }
    }
    if (speaker.drunk != 0 || 0)
    {
        if (game_data.current_map != mdata_t::MapId::show_house)
        {
            if (!speaker.is_player_or_ally())
            {
                if (!event_has_pending_events())
                {
                    ELONA_APPEND_RESPONSE(
                        56, i18n::s.get("core.talk.npc.common.choices.sex"));
                }
            }
        }
    }
    if (speaker.id == CharaId::prostitute)
    {
        if (!event_has_pending_events())
        {
            ELONA_APPEND_RESPONSE(
                60, i18n::s.get("core.talk.npc.prostitute.choices.buy"));
        }
    }
    if (speaker.role == Role::caravan_master)
    {
        ELONA_APPEND_RESPONSE(
            61, i18n::s.get("core.talk.npc.caravan_master.choices.hire"));
    }
    f = 0;
    deliver = -1;
    if (game_data.current_dungeon_level == 1)
    {
        for (int cnt = 0, cnt_end = (game_data.number_of_existing_quests);
             cnt < cnt_end;
             ++cnt)
        {
            if (quest_data[cnt].originating_map_id == game_data.current_map)
            {
                if (quest_data[cnt].client_chara_index == speaker.index)
                {
                    rq = cnt;
                    f = 1;
                    break;
                }
            }
        }
    }
    if (f == 1)
    {
        for (int cnt = 0, cnt_end = (game_data.number_of_existing_quests);
             cnt < cnt_end;
             ++cnt)
        {
            if (quest_data[cnt].id == 0)
            {
                continue;
            }
            if (quest_data[cnt].progress != 1)
            {
                continue;
            }
            if (quest_data[cnt].client_chara_type == 2)
            {
                if (quest_data[cnt].target_chara_index == rq)
                {
                    p = quest_data[cnt].target_item_id;
                    deliver = cnt;
                    for (const auto& item : g_inv.pc())
                    {
                        if (the_item_db[item->id]->legacy_id == p)
                        {
                            item_to_deliver = item;
                            break;
                        }
                    }
                }
            }
        }
        if (quest_data[rq].progress == 3)
        {
            quest_set_data(speaker, 3);
            quest_complete();
        }
        else if (
            quest_data[rq].client_chara_type == 3 &&
            quest_data[rq].progress == 1)
        {
            for (const auto& item : g_inv.pc())
            {
                if (item->is_no_drop)
                {
                    continue;
                }
                if (quest_data[rq].id == 1003)
                {
                    if (the_item_db[item->id]->category == ItemCategory::food &&
                        item->param1 / 1000 == quest_data[rq].extra_info_1 &&
                        item->param2 == quest_data[rq].extra_info_2)
                    {
                        item_to_supply = item;
                        break;
                    }
                }
                if (quest_data[rq].id == 1004 || quest_data[rq].id == 1011)
                {
                    if (the_item_db[item->id]->legacy_id ==
                        quest_data[rq].target_item_id)
                    {
                        item_to_supply = item;
                        break;
                    }
                }
            }
            if (item_to_supply)
            {
                ELONA_APPEND_RESPONSE(
                    26,
                    i18n::s.get(
                        "core.talk.npc.quest_giver.choices.here_is_item",
                        item_to_supply.unwrap()));
            }
            else
            {
                ELONA_APPEND_RESPONSE(
                    24,
                    i18n::s.get(
                        "core.talk.npc.quest_giver.choices.about_the_work"));
            }
        }
        else if (quest_data[rq].id != 0)
        {
            ELONA_APPEND_RESPONSE(
                24,
                i18n::s.get(
                    "core.talk.npc.quest_giver.choices.about_the_work"));
        }
    }
    if (deliver != -1 && item_to_deliver)
    {
        ELONA_APPEND_RESPONSE(
            25,
            i18n::s.get("core.talk.npc.quest_giver.choices.here_is_delivery"));
    }
    if (game_data.current_map == mdata_t::MapId::your_home)
    {
        if (speaker.is_map_local())
        {
            if (speaker.role != Role::none)
            {
                if (!is_guest(speaker.role) && !event_has_pending_events())
                {
                    ELONA_APPEND_RESPONSE(
                        44, i18n::s.get("core.talk.npc.servant.choices.fire"));
                }
            }
        }
    }
    if (speaker.role == Role::moyer)
    {
        if (game_data.quest_flags.pael_and_her_mom == 1000)
        {
            if (const auto lily = chara_find("core.lily"))
            {
                if (lily->state() == Character::State::alive)
                {
                    ELONA_APPEND_RESPONSE(
                        52,
                        i18n::s.get(
                            "core.talk.npc.moyer.choices.sell_paels_mom"));
                }
            }
        }
    }
    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.bye"));
    chatesc = 1;

    int chatval_ = talk_window_query(speaker);

    if (chatval_ == 10 || chatval_ == 11)
    {
        if (is_shopkeeper(speaker.role))
        {
            if (cdata.player().karma < -30 &&
                cdata.player().is_incognito() == 0)
            {
                if (game_data.current_map != mdata_t::MapId::derphy &&
                    game_data.current_map != mdata_t::MapId::your_home)
                {
                    listmax = 0;
                    if (chatval_ == 10)
                    {
                        buff = i18n::s.get(
                            "core.talk.npc.shop.criminal.buy", speaker);
                    }
                    else
                    {
                        buff = i18n::s.get(
                            "core.talk.npc.shop.criminal.sell", speaker);
                    }
                    ELONA_APPEND_RESPONSE(0, i18n::s.get("core.ui.more"));
                    chatesc = 1;
                    talk_window_query(speaker);
                    if (scenemode)
                    {
                        if (scene_cut == 1)
                        {
                            return TalkResult::talk_end;
                        }
                    }
                    return TalkResult::talk_npc;
                }
            }
        }
    }

    switch (chatval_)
    {
    case 1: buff = ""; return TalkResult::talk_npc;
    case 10: return talk_shop_buy(speaker);
    case 11: return talk_shop_sell(speaker);
    case 12: return talk_invest(speaker);
    case 13: return talk_inn_eat(speaker);
    case 14:
    case 15:
    case 16: return talk_wizard_identify(speaker, chatval_);
    case 17: return talk_trainer_train_skill(speaker);
    case 18: return talk_informer_list_adventurers(speaker);
    case 19: return talk_healer_restore_attributes(speaker);
    case 20: return talk_trade(speaker);
    case 21:
    case 22: return talk_arena_master(speaker, chatval_);
    case 40:
    case 41: return talk_pet_arena_master(speaker, chatval_);
    case 42: return talk_pet_arena_master_score(speaker);
    case 23: return talk_arena_master_score(speaker);
    case 24: return TalkResult::talk_quest_giver;
    case 25:
        assert(item_to_deliver);
        return talk_quest_delivery(speaker, item_to_deliver.unwrap());
    case 26:
        assert(item_to_supply);
        return talk_quest_supply(speaker, item_to_supply.unwrap());
    case 30: return talk_trainer_learn_skill(speaker);
    case 31: return talk_shop_attack(speaker);
    case 32: return talk_guard_return_item(speaker);
    case 33: return talk_bartender_call_ally(speaker);
    case 34: return talk_ally_order_wait(speaker);
    case 35: return talk_ally_abandon(speaker);
    case 36:
    case 57: return talk_slave_buy(speaker, chatval_);
    case 37: return talk_slave_sell(speaker);
    case 38: return talk_ally_marriage(speaker);
    case 39: return talk_ally_gene(speaker);
    case 43: return talk_innkeeper_shelter(speaker);
    case 44: return talk_servant_fire(speaker);
    case 45: return talk_maid_think_of_house_name(speaker);
    case 46: return talk_sister_buy_indulgence(speaker);
    case 47: return talk_informer_investigate_ally(speaker);
    case 48: return talk_ally_silence(speaker);
    case 50: return talk_adventurer_hire(speaker);
    case 51: return talk_adventurer_join(speaker);
    case 52: return talk_moyer_sell_paels_mom(speaker);
    case 53: return talk_wizard_return(speaker);
    case 54: return talk_shop_reload_ammo(speaker);
    case 55: return talk_spell_writer_reserve(speaker);
    case 56: return talk_sex(speaker);
    case 58: {
        if (game_data.left_turns_of_timestop == 0)
        {
            event_add(25);
        }
        return TalkResult::talk_end;
    }
    case 59: return talk_result_maid_chase_out(speaker);
    case 60: return talk_prostitute_buy(speaker);
    case 61: return talk_caravan_master_hire(speaker);
    }

    if (chatval_ >= 10000)
    {
        return talk_guard_where_is(speaker, chatval_);
    }

    if (event_processing_event() == 11)
    {
        levelexitby = 4;
        chatteleport = 1;
        snd("core.exitmap1");
    }

    return TalkResult::talk_end;
}

} // namespace elona
