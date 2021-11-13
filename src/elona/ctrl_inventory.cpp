#include "../util/strutil.hpp"
#include "ability.hpp"
#include "activity.hpp"
#include "attack.hpp"
#include "audio.hpp"
#include "calc.hpp"
#include "character.hpp"
#include "character_status.hpp"
#include "command.hpp"
#include "config.hpp"
#include "data/types/type_item.hpp"
#include "dmgheal.hpp"
#include "draw.hpp"
#include "elona.hpp"
#include "enums.hpp"
#include "equipment.hpp"
#include "globals.hpp"
#include "i18n.hpp"
#include "input.hpp"
#include "inventory.hpp"
#include "item.hpp"
#include "keybind/keybind.hpp"
#include "map.hpp"
#include "menu.hpp"
#include "message.hpp"
#include "quest.hpp"
#include "shop.hpp"
#include "text.hpp"
#include "ui.hpp"
#include "variables.hpp"



#if 0
_1 = "どのアイテムを調べる？ "
_2 = "どのアイテムを置く？ "
_3 = "どのアイテムを拾う？ "

_5 = "何を食べよう？ "
_6 = "何を装備する？"
_7 = "どれを読む？ "
_8 = "どれを飲む？ "
_9 = "どれを振る？ "
_10 = "どれを渡す？ "
_11 = "どれを購入する？ "
_12 = "どれを売却する？ "
_13 = "どのアイテムを鑑定する？ "
_14 = "どのアイテムを使用する？ "
_15 = "どれを開ける？ "
_16 = "何を料理する？ "
_17 = "何を混ぜる？ "
_18 = "何に混ぜる？(${_1}の効果を適用するアイテムを選択) "
_19 = "何を神に捧げる？ "
_20 = "何を交換する？ "
_21 = "${_1}の代わりに何を提示する？ "
_22 = "何を取る？ "
_23 = "何を対象にする？ "
_24 = "何を入れる？ "
_25 = "何をもらう？ "
_26 = "何を投げる？ "
_27 = "何を盗む？ "
_28 = "何と交換する？ "
#endif



namespace elona
{

namespace
{

struct OnEnterResult
{
    int type;
    MenuResult menu_result;
    OptionalItemRef selected_item;


    OnEnterResult(int type)
        : type(type)
    {
    }


    OnEnterResult(
        const MenuResult& menu_result,
        OptionalItemRef selected_item = nullptr)
        : type(0)
        , menu_result(menu_result)
        , selected_item(std::move(selected_item))
    {
    }
};

OnEnterResult on_enter(
    optional_ref<Character> inventory_owner,
    int selected_item_index,
    OptionalItemRef& citrade,
    OptionalItemRef& cidip,
    bool dropcontinue);
optional<MenuResult> on_cancel(bool dropcontinue);



bool reset_mru_cursor(int invctrl)
{
    return invctrl == 11 || invctrl == 12;
}



bool exclude_ground_items(int invctrl)
{
    return invctrl == 2 || invctrl == 6 || invctrl == 10 || invctrl == 12 ||
        invctrl == 16 || invctrl == 20 || invctrl == 21 || invctrl == 23 ||
        invctrl == 24 || invctrl == 25;
}



bool refers_to_tmp_inventory(int invctrl)
{
    return invctrl == 11 || invctrl == 22 || invctrl == 28;
}



bool exclude_character_items(int invctrl)
{
    return invctrl == 3 || invctrl == 11 || invctrl == 22 || invctrl == 28;
}



bool exclude_equipped_items(int invctrl)
{
    return invctrl != 1 && invctrl != 5 && invctrl != 13 && invctrl != 14 &&
        invctrl != 18 && invctrl != 20 && invctrl != 23 && invctrl != 25 &&
        invctrl != 27;
}



bool show_inventory_owners_money(int invctrl)
{
    return invctrl == 11 || invctrl == 12 || invctrl == 25 || invctrl == 27;
}



bool can_assign_shortcut(int invctrl)
{
    return invctrl == 5 || invctrl == 7 || invctrl == 8 || invctrl == 9 ||
        invctrl == 14 || invctrl == 15 || invctrl == 26;
}



int get_menu_cycle_type(bool dropcontinue)
{
    if (dropcontinue)
    {
        return 4;
    }
    else if (invally == 1)
    {
        return 2;
    }
    else if (invcontainer)
    {
        return 3;
    }
    else if (map_data.type == mdata_t::MapType::world_map)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



void remove_card_and_figure_from_heir_trunk()
{
    if (invctrl == 22 && invctrl(1) == 1)
    {
        for (const auto& item : g_inv.tmp())
        {
            if (item->id == "core.card" || item->id == "core.figurine")
            {
                item->remove();
            }
        }
    }
}



void fallback_to_default_command_if_unavailable()
{
    if (menucycle)
    {
        if (map_data.type == mdata_t::MapType::world_map)
        {
            p = 0;
            for (int cnt = 0; cnt < 12; ++cnt)
            {
                if (cycle(cnt, 1) == -1)
                {
                    break;
                }
                if (cycle(cnt, 1) == invctrl)
                {
                    p = 1;
                    break;
                }
            }
            if (p == 0)
            {
                invctrl = cycle(0, 1);
            }
        }
    }
}



void restore_cursor()
{
    if (reset_mru_cursor(invctrl(0)))
    {
        invmark(invctrl) = 0;
    }
    page = 0;
    pagesize = 16;
    listmax = 0;
    cs = invmark(invctrl) % 1000;
    page = invmark(invctrl) / 1000;
    cs_bk = -1;
    page_load();
}



void make_item_list(
    optional_ref<Character> inventory_owner,
    OptionalItemRef& mainweapon,
    const OptionalItemRef& citrade,
    const OptionalItemRef& cidip)
{
    // cnt = 0 => extra
    // cnt = 1 => PC/NPC
    for (int cnt = 0; cnt < 2; ++cnt)
    {
        Inventory* inv;
        if (cnt == 0)
        {
            if (exclude_ground_items(invctrl(0)))
            {
                continue;
            }
            if (refers_to_tmp_inventory(invctrl(0)))
            {
                inv = &g_inv.tmp();
            }
            else
            {
                inv = &g_inv.ground();
            }
        }
        if (cnt == 1)
        {
            inv = &g_inv.pc();
            if (invctrl == 20 || invctrl == 25)
            {
                assert(inventory_owner);
                inv = &g_inv.for_chara(*inventory_owner);
            }
            if (invctrl == 27)
            {
                const auto target_chara_index =
                    cell_data.at(tlocx, tlocy).chara_index_plus_one - 1;
                if (target_chara_index == 0 || target_chara_index == -1)
                {
                    continue;
                }
                inv = &g_inv.for_chara(cdata[target_chara_index]);
            }
            if (exclude_character_items(invctrl(0)))
            {
                break;
            }
        }
        int cnt2 = cnt;

        for (const auto& item : *inv)
        {
            // compatibility?
            if (item->id == "core.training_machine")
            {
                item->function = 9;
            }

            // compatibility?
            if (the_item_db[item->id]->legacy_id >= maxitemid ||
                the_item_db[item->id]->legacy_id < 0)
            {
                dialog(i18n::s.get(
                    "core.ui.inv.common.invalid",
                    the_item_db[item->id]->legacy_id));
                item->remove();
                item->id = "core.none";
                continue;
            }

            if (map_data.type == mdata_t::MapType::world_map)
            {
                if (invctrl == 7)
                {
                    if (the_item_db[item->id]->subcategory != 53100 &&
                        item->id != "core.treasure_map")
                    {
                        // ワールドマップでは権利書か宝の地図しか読めない
                        continue;
                    }
                }
            }
            if (cnt2 == 0)
            {
                if (invctrl == 27)
                {
                    if (item->position().x != tlocx ||
                        item->position().y != tlocy)
                    {
                        // その座標にあるものしか盗めない
                        continue;
                    }
                }
                else if (invctrl != 11 && invctrl != 22 && invctrl != 28)
                {
                    if (item->position().x != cdata.player().position.x ||
                        item->position().y != cdata.player().position.y)
                    {
                        // キャラと同じ座標にあるものしか対象に取れない
                        continue;
                    }
                }
            }

            // ここで呼び出す?
            item_checkknown(item);

            reftype = (int)the_item_db[item->id]->category;

            if (item->own_state == OwnState::town_special)
            {
                // ショウルームのアイテムで、[調べる]でなく、showroom_onlyのものを
                // 使おうとしているのでもないならリストから除外
                if (!item->is_showroom_only || invctrl != 14)
                {
                    if (invctrl != 1)
                    {
                        continue;
                    }
                }
            }

            if (exclude_equipped_items(invctrl(0)))
            {
                if (item->body_part != 0) // `item` is worn.
                {
                    continue;
                }
            }

            // (利き腕)表示用
            if (item->body_part != 0)
            {
                if (reftype == 10000)
                {
                    if (!mainweapon || item->body_part < mainweapon->body_part)
                    {
                        mainweapon = item;
                    }
                }
            }

            // 各行動
            if (invctrl == 5)
            {
                if (reftype != 57000 && reftype != 91000 &&
                    item->material != "core.raw")
                {
                    continue;
                }
            }
            if (invctrl == 6)
            {
                if (iequiploc(item) !=
                    cdata.player().equipment_slots[body - 100].type)
                {
                    continue;
                }
            }
            if (invctrl == 7)
            {
                if (!the_item_db[item->id]->is_readable)
                {
                    continue;
                }
            }
            if (invctrl == 8)
            {
                if (!the_item_db[item->id]->is_drinkable)
                {
                    continue;
                }
            }
            if (invctrl == 9)
            {
                if (!the_item_db[item->id]->is_zappable)
                {
                    continue;
                }
            }
            if (invctrl == 11)
            {
                if (item->id == "core.gold_piece" ||
                    item->id == "core.platinum_coin")
                {
                    continue;
                }
            }
            if (invctrl == 11 || invctrl == 12)
            {
                if (shoptrade)
                {
                    if (item->weight >= 0)
                    {
                        continue;
                    }
                    else if (reftype != 92000)
                    {
                        continue;
                    }
                }
                else if (item->weight < 0)
                {
                    if (reftype == 92000)
                    {
                        continue;
                    }
                }
                if (item->value <= 1)
                {
                    continue;
                }
                if (item->is_precious)
                {
                    continue;
                }
                if (item->param3 < 0)
                {
                    continue;
                }
                if (item->quality == Quality::special)
                {
                    continue;
                }
            }
            if (invctrl == 13)
            {
                if (item->identify_state == IdentifyState::completely)
                {
                    continue;
                }
            }
            if (invctrl == 14)
            {
                if (item->function == 0 && !the_item_db[item->id]->is_usable &&
                    !item->is_alive)
                {
                    continue;
                }
            }
            if (invctrl == 15)
            {
                if (reftype != 72000)
                {
                    continue;
                }
            }
            if (invctrl == 16)
            {
                if (reftype != 57000)
                {
                    continue;
                }
                else if (item->param2 != 0)
                {
                    continue;
                }
            }
            if (invctrl == 17)
            {
                if (reftype != 52000 && item->id != "core.bait")
                {
                    continue;
                }
            }
            if (invctrl == 18)
            {
                if (cidip->id == "core.bait")
                {
                    if (item->id != "core.fishing_pole")
                    {
                        continue;
                    }
                }
                if (cidip == item || item->id == "core.bottle_of_water")
                {
                    continue;
                }
            }
            if (invctrl == 19)
            {
                if (!god_is_offerable(item, cdata.player()))
                {
                    continue;
                }
            }
            if (invctrl == 20)
            {
                if (item->id == "core.gold_piece" ||
                    item->id == "core.platinum_coin")
                {
                    continue;
                }
            }
            if (invctrl == 21)
            {
                if (calcitemvalue(item, 0) * item->number() <
                    calcitemvalue(citrade.unwrap(), 0) * citrade->number() / 2 *
                        3)
                {
                    continue;
                }
                if (item->is_stolen)
                {
                    continue;
                }
            }
            if (invctrl == 23)
            {
                if (invctrl(1) == 0)
                {
                    if (reftype >= 50000 && reftype != 60000)
                    {
                        continue;
                    }
                }
                if (invctrl(1) == 1)
                {
                    if (reftype != 10000 && reftype != 24000)
                    {
                        continue;
                    }
                }
                if (invctrl(1) == 2)
                {
                    if (reftype < 12000 || reftype >= 24000)
                    {
                        if (reftype < 30000 || reftype >= 50000)
                        {
                            continue;
                        }
                    }
                }
                if (invctrl(1) == 3)
                {
                    if (!item->has_charges)
                    {
                        continue;
                    }
                }
                if (invctrl(1) == 4)
                {
                    if (item->body_part != 0)
                    {
                        continue;
                    }
                }
                if (invctrl(1) == 5)
                {
                    if (reftype != 56000)
                    {
                        continue;
                    }
                }
                if (invctrl(1) == 6)
                {
                    if (item->weight <= 0 || item->id == "core.cooler_box")
                    {
                        continue;
                    }
                }
                if (invctrl(1) == 7)
                {
                    if (item->quality >= Quality::miracle || reftype >= 50000)
                    {
                        continue;
                    }
                }
            }
            if (invctrl == 24)
            {
                if (invctrl(1) == 0)
                {
                    if (game_data.current_map == mdata_t::MapId::lumiest)
                    {
                        if (item->id != "core.ancient_book" ||
                            item->param2 == 0)
                        {
                            continue;
                        }
                    }
                    else if (item->own_state != OwnState::crop)
                    {
                        continue;
                    }
                }
                else if (item->own_state == OwnState::crop)
                {
                    continue;
                }
                if (invctrl(1) == 2)
                {
                    if (item->id != "core.bill")
                    {
                        continue;
                    }
                }
                if (reftype == 72000)
                {
                    continue;
                }
                if (invctrl(1) == 3)
                {
                    if (reftype != 57000)
                    {
                        continue;
                    }
                }
            }
            else if (item->own_state == OwnState::crop)
            {
                // 調べる、置く、拾う、食べる、納入する
                // 以外は禁止(収穫依頼の野菜)
                if (invctrl != 1 && invctrl != 2 && invctrl != 3 &&
                    invctrl != 5)
                {
                    continue;
                }
            }
            if (invctrl == 26)
            {
                if (reftype != 52000 && item->id != "core.kitty_bank" &&
                    item->id != "core.monster_ball" &&
                    item->id != "core.little_ball" && item->id != "core.tomato")
                {
                    continue;
                }
                if (item->id == "core.monster_ball")
                {
                    if (item->subname != 0)
                    {
                        continue;
                    }
                }
            }
            if (invctrl == 27)
            {
                if (cnt2 == 0)
                {
                    if (item->own_state != OwnState::town)
                    {
                        continue;
                    }
                }
            }

            // リスト追加
            list(0, listmax) = item->global_index();

            // ソート情報
            list(1, listmax) =
                reftype * 1000 + the_item_db[item->id]->legacy_id;
            if (item->id == "core.disc")
            {
                list(1, listmax) += item->param1 + 900;
            }
            if (invctrl == 1 || invctrl == 13)
            {
                if (item->body_part != 0)
                {
                    list(1, listmax) -= 99999000;
                }
            }
            if (cnt2 == 0)
            {
                list(1, listmax) -= 199998000;
            }
            if (invctrl == 28)
            {
                list(1, listmax) = calcmedalvalue(item);
            }

            ++listmax;
        }
    }
}



// 不可能な行動を制限
optional<MenuResult> check_command(
    optional_ref<Character> inventory_owner,
    const OptionalItemRef& citrade)
{
    MenuResult result = {false, false, TurnResult::none};

    f = 0;
    if (listmax == 0)
    {
        if (invctrl == 21)
        {
            txt(i18n::s.get(
                "core.ui.inv.trade.too_low_value", citrade.unwrap()));
            f = 1;
        }
        if (invctrl == 27)
        {
            if (inventory_owner)
            {
                txt(i18n::s.get(
                    "core.ui.inv.steal.has_nothing", *inventory_owner));
                f = 1;
            }
            else
            {
                txt(i18n::s.get("core.ui.inv.steal.there_is_nothing"));
                f = 1;
            }
        }
    }
    if (invctrl == 19)
    {
        if (!item_find(ItemCategory::altar))
        {
            txt(i18n::s.get("core.ui.inv.offer.no_altar"),
                Message::only_once{true});
            f = 1;
        }
    }
    if (invctrl == 27)
    {
        if (inventory_owner)
        {
            if (inventory_owner->relationship == 10)
            {
                txt(i18n::s.get("core.ui.inv.steal.do_not_rob_ally"));
                f = 1;
            }
        }
    }
    if (invctrl == 24)
    {
        if (invctrl(1) == 0)
        {
            if (game_data.current_map == mdata_t::MapId::lumiest)
            {
                if (game_data.guild.mages_guild_quota <= 0)
                {
                    txt(i18n::s.get("core.ui.inv.put.guild.have_no_quota"));
                    f = 1;
                }
            }
        }
    }

    if (f == 1)
    {
        if (invsubroutine == 1)
        {
            invsubroutine = 0;
            result.succeeded = false;
            return result;
        }
        update_screen();
        result.turn_result = TurnResult::pc_turn_user_error;
        return result;
    }

    return none;
}



optional<MenuResult> check_pick_up()
{
    MenuResult result = {false, false, TurnResult::none};

    if (invctrl == 3)
    {
        if (listmax == 0)
        {
            result.turn_result = TurnResult::turn_end;
            return result;
        }
    }

    return none;
}



void show_message(const OptionalItemRef& citrade, const OptionalItemRef& cidip)
{
    if (returnfromidentify == 0)
    {
        std::string valn;
        if (invctrl == 18)
        {
            valn = itemname(cidip.unwrap(), 1);
        }
        else if (invctrl == 21)
        {
            valn = itemname(citrade.unwrap());
        }

        for (int cnt = 0; cnt < 30; cnt++)
        {
            if (auto text =
                    i18n::s.get_enum_optional("core.ui.inv.title", cnt, valn))
            {
                s(cnt) = *text;
            }
            else
            {
                s(cnt) = ""s;
            }
        }

        if (s(invctrl) != ""s)
        {
            Message::instance().linebreak();
        }
        if (invsc == 0)
        {
            txt(s(invctrl));
        }
        if (invctrl == 28)
        {
            if (const auto small_medals = item_find(
                    "core.small_medal", ItemFindLocation::player_inventory))
            {
                p = small_medals->number();
            }
            else
            {
                p = 0;
            }
            txt(i18n::s.get("core.ui.inv.trade_medals.medals", p(0)));
        }
        if (invctrl == 24 && invctrl(1) == 0)
        {
            if (game_data.current_map == mdata_t::MapId::lumiest)
            {
                txt(i18n::s.get(
                    "core.ui.inv.put.guild.remaining",
                    game_data.guild.mages_guild_quota));
            }
        }
    }

    asset_load("core.deco_inv");
    gsel(0);
    if (returnfromidentify == 0)
    {
        windowshadow = 1;
    }
    returnfromidentify = 0;
}



// ショートカット経由
optional<OnEnterResult>
on_shortcut(OptionalItemRef& citrade, OptionalItemRef& cidip, bool dropcontinue)
{
    MenuResult result = {false, false, TurnResult::none};

    if (invsc != 0)
    {
        f = 0;
        for (int cnt = 0, cnt_end = (listmax); cnt < cnt_end; ++cnt)
        {
            p = list(0, cnt);
            if (the_item_db[g_inv[p]->id]->legacy_id == invsc)
            {
                f = 1;
                if (g_inv[p]->has_charges)
                {
                    if (g_inv[p]->charges <= 0)
                    {
                        continue;
                    }
                }
                break;
            }
        }
        if (f == 0)
        {
            if (itemfind(g_inv.pc(), *the_item_db.get_id_from_legacy(invsc)))
            {
                Message::instance().linebreak();
                txt(i18n::s.get("core.action.cannot_do_in_global"));
            }
            else
            {
                txt(i18n::s.get("core.ui.inv.common.does_not_exist"));
            }
            invsc = 0;
            update_screen();
            result.turn_result = TurnResult::pc_turn_user_error;
            return OnEnterResult{result};
        }
        invsc = 0;
        if (map_data.type == mdata_t::MapType::world_map)
        {
            if (invctrl == 9 || invctrl == 15 || invctrl == 26)
            {
                Message::instance().linebreak();
                txt(i18n::s.get("core.action.cannot_do_in_global"));
                update_screen();
                result.turn_result = TurnResult::pc_turn_user_error;
                return OnEnterResult{result};
            }
        }
        if (!cargocheck(g_inv[p]))
        {
            result.turn_result = TurnResult::pc_turn_user_error;
            return OnEnterResult{result};
        }
        return on_enter(none, p(0), citrade, cidip, dropcontinue);
    }

    return none;
}



void set_cursor()
{
    cs_bk = -1;
    pagemax = (listmax - 1) / pagesize;
    if (page < 0)
    {
        page = pagemax;
    }
    else if (page > pagemax)
    {
        page = 0;
    }
}



void draw_menu(bool dropcontinue)
{
    if (!menucycle)
        return;

    font(12 + sizefix - en * 2);
    y = 34;
    x = windoww - 650 + 156;
    window2(x, y, 475, 22, 5, 5);
    draw("core.radar_deco", x - 28, y - 8);
    if (dropcontinue)
    {
        i = 4;
    }
    else if (invally == 1)
    {
        i = 2;
    }
    else if (invcontainer)
    {
        i = 3;
    }
    else if (map_data.type == mdata_t::MapType::world_map)
    {
        i = 1;
    }
    else
    {
        i = 0;
    }
    for (int cnt = 0; cnt < 12; ++cnt)
    {
        if (cycle(cnt, i) == -1)
        {
            break;
        }
        p = cycle(cnt, i);
        draw_indexed(
            "core.inventory_icon", x + cnt * 44 + 20, y - 24, invicon(p));
        if (invctrl == p)
        {
            gmode(5, 70);
            draw_indexed(
                "core.inventory_icon", x + cnt * 44 + 20, y - 24, invicon(p));
            gmode(2);
        }
        std::string inv_command_txt =
            i18n::s.get_enum("core.ui.inventory_command", p);
        bmes(
            inv_command_txt,
            x + cnt * 44 + 46 - strlen_u(inv_command_txt) * 3,
            y + 7,
            invctrl == p ? snail::Color{255, 255, 255}
                         : snail::Color{165, 165, 165});
        if (invkey(p) != ""s)
        {
            bmes(
                u8"("s + invkey(p) + u8")"s,
                x + cnt * 44 + 46,
                y + 18,
                {235, 235, 235});
        }
    }
    bmes(
        keybind_get_bound_key_name("northwest") + "," +
            keybind_get_bound_key_name("northeast") + u8",Tab,Ctrl+Tab "s +
            "[" + i18n::s.get("core.ui.inv.window.change") + "]",
        x + 260,
        y + 32);
}



void draw_window(optional_ref<Character> inventory_owner, bool dropcontinue)
{
    auto key_help = strhint2 + strhint5 + strhint5b + strhint3;
    if (invctrl == 5 || invctrl == 7 || invctrl == 8 || invctrl == 9 ||
        invctrl == 14 || invctrl == 15 || invctrl == 26)
    {
        key_help += strhint7;
    }
    if (invctrl == 1)
    {
        key_help += keybind_get_bound_key_name("switch_mode_2") + " [" +
            i18n::s.get("core.ui.inv.window.tag.no_drop") + "]";
    }
    if (invctrl == 2)
    {
        if (!dropcontinue)
        {
            key_help += keybind_get_bound_key_name("switch_mode_2") + " [" +
                i18n::s.get("core.ui.inv.window.tag.multi_drop") + "]";
        }
    }
    ui_display_window(
        i18n::s.get(
            "core.ui.inv.window.select_item",
            i18n::s.get_enum("core.ui.inventory_command", invctrl)),
        key_help,
        (windoww - 640) / 2 + inf_screenx,
        winposy(432),
        640,
        432);

    if (invicon(invctrl) != -1)
    {
        draw_indexed("core.inventory_icon", wx + 46, wy - 14, invicon(invctrl));
    }
    s = i18n::s.get("core.ui.inv.window.weight");
    if (invctrl == 11 || invctrl == 12)
    {
        s = i18n::s.get("core.ui.inv.buy.window.price");
    }
    if (invctrl == 28)
    {
        s = i18n::s.get("core.ui.inv.trade_medals.window.medal");
    }
    display_topic(i18n::s.get("core.ui.inv.window.name"), wx + 28, wy + 30);
    display_topic(s, wx + 526, wy + 30);

    draw_additional_item_info_label(wx + 300, wy + 40);

    draw("core.deco_inv_a", wx + ww - 136, wy - 6);
    if (g_show_additional_item_info == AdditionalItemInfo::none)
    {
        draw("core.deco_inv_b", wx + ww - 186, wy - 6);
    }
    draw("core.deco_inv_c", wx + ww - 246, wy - 6);
    draw("core.deco_inv_d", wx - 6, wy - 6);
    s = ""s + listmax + u8" items"s;
    s += "  ("s +
        i18n::s.get(
            "core.ui.inv.window.total_weight",
            cnvweight(cdata.player().inventory_weight),
            cnvweight(cdata.player().max_inventory_weight),
            cnvweight(game_data.cargo_weight)) +
        ")"s;
    if (invctrl == 25)
    {
        s = ""s;
    }
    display_note(s);
    if (invctrl == 25)
    {
        assert(inventory_owner);
        x = (windoww - 640) / 2 + inf_screenx + 455;
        y = winposy(432) - 32;
        int w = 200;
        int h = 102;
        window(x + 4, y + 4, w, h - h % 8, true);
        window(x, y, w, h - h % 8);
        font(12 + en - en * 2);
        mes(x + 16,
            y + 17,
            u8"DV:"s + inventory_owner->dv + u8" PV:"s + inventory_owner->pv);
        mes(x + 16,
            y + 35,
            i18n::s.get("core.ui.inv.take_ally.window.equip_weight") + ":" +
                cnvweight(inventory_owner->sum_of_equipment_weight) + ""s +
                get_armor_class_name(*inventory_owner));
        x = wx + 40;
        y = wy + wh - 65 - wh % 8;
        mes(x, y, i18n::s.get("core.ui.inv.take_ally.window.equip"));
        x += 60;

        for (const auto& equipment_slot : inventory_owner->equipment_slots)
        {
            if (!equipment_slot)
            {
                continue;
            }
            std::string body_part_desc =
                i18n::s.get_enum("core.ui.body_part", equipment_slot.type);
            const auto text_color = equipment_slot.equipment
                ? snail::Color{50, 50, 200}
                : snail::Color{100, 100, 100};
            mes(x, y, body_part_desc, text_color);
            x += (strlen_u(body_part_desc) + 1) * 6;
        }
    }
}



void update_key_list()
{
    keyrange = 0;
    for (int cnt = 0, cnt_end = (pagesize); cnt < cnt_end; ++cnt)
    {
        p = pagesize * page + cnt;
        if (p >= listmax)
        {
            break;
        }
        key_list(cnt) = key_select(cnt);
        ++keyrange;
        if (cnt % 2 == 0)
        {
            boxf(wx + 70, wy + 60 + cnt * 19, 540, 18, {12, 14, 16, 16});
        }
    }
    font(14 - en * 2);
    cs_listbk();
}



void draw_item_list(const OptionalItemRef& mainweapon)
{
    for (int cnt = 0, cnt_end = (pagesize); cnt < cnt_end; ++cnt)
    {
        p = pagesize * page + cnt;
        if (p >= listmax)
        {
            break;
        }
        p = list(0, p);
        s(0) = itemname(g_inv[p]);
        s(1) = cnvweight(g_inv[p]->weight * g_inv[p]->number());
        if (invctrl == 11)
        {
            s += u8" "s + cnvweight(g_inv[p]->weight);
            s(1) = ""s + calcitemvalue(g_inv[p], 0) + u8" gp"s;
        }
        if (invctrl == 12)
        {
            s += u8" "s + cnvweight(g_inv[p]->weight);
            s(1) = ""s + calcitemvalue(g_inv[p], 1) + u8" gp"s;
        }
        if (invctrl == 28)
        {
            s(1) = i18n::s.get(
                "core.ui.inv.trade_medals.medal_value",
                calcmedalvalue(g_inv[p]));
        }
        if (invctrl != 3 && invctrl != 11 && invctrl != 22 && invctrl != 27 &&
            invctrl != 28)
        {
            if (item_is_on_ground(g_inv[p]))
            {
                s += i18n::space_if_needed() + "(" +
                    i18n::s.get("core.ui.inv.window.ground") + ")";
            }
        }
        for (int cnt = 0; cnt < 20; ++cnt)
        {
            if (game_data.skill_shortcuts.at(cnt) ==
                the_item_db[g_inv[p]->id]->legacy_id + invctrl * 10000)
            {
                s +=
                    u8"{"s + get_bound_shortcut_key_name_by_index(cnt) + u8"}"s;
            }
        }
        display_key(wx + 58, wy + 60 + cnt * 19 - 2, cnt);

        draw_item_with_portrait_scale_height(
            g_inv[p], wx + 37, wy + 69 + cnt * 19);

        if (g_inv[p]->body_part != 0)
        {
            draw("core.equipped", wx + 46, wy + 72 + cnt * 18 - 3);
            if (g_inv[p] == mainweapon)
            {
                s += i18n::space_if_needed() + "(" +
                    i18n::s.get("core.ui.inv.window.main_hand") + ")";
            }
        }
        draw_additional_item_info(g_inv[p], wx + 300, wy + 60 + cnt * 19 + 2);
        if (g_show_additional_item_info != AdditionalItemInfo::none)
        {
            s = cut_item_name_for_additional_info(s);
        }
        const auto text_color = cs_list_get_item_color(g_inv[p]);
        cs_list(cs == cnt, s, wx + 84, wy + 60 + cnt * 19 - 1, 0, text_color);
        mes(wx + 600 - strlen_u(s(1)) * 7,
            wy + 60 + cnt * 19 + 2,
            s(1),
            text_color);
    }
}



void save_csbk()
{
    if (keyrange != 0)
    {
        cs_bk = cs;
    }
}



void show_money(optional_ref<Character> inventory_owner)
{
    if (show_inventory_owners_money(invctrl(0)))
    {
        if (inventory_owner &&
            g_show_additional_item_info == AdditionalItemInfo::none)
        {
            font(13 - en * 2);
            gmode(2);
            draw("core.gold_coin", wx + 340, wy + 32);
            mes(wx + 368,
                wy + 37 - en * 2,
                ""s + inventory_owner->gold + u8" gp"s);
        }
    }
}



std::string get_action()
{
    auto action = get_selected_item(p(0));
    invmark(invctrl) = page * 1000 + cs;
    if (mode == 9)
    {
        if (listmax == 0)
        {
            p = -1;
            action = "cancel";
        }
    }
    return action;
}



OnEnterResult on_enter_examine(const ItemRef& selected_item)
{
    item_show_description(selected_item);
    return OnEnterResult{1};
}



OnEnterResult on_enter_drop(
    const ItemRef& selected_item,
    MenuResult& result,
    bool dropcontinue)
{
    if (selected_item->is_no_drop)
    {
        snd("core.fail1");
        txt(i18n::s.get("core.ui.inv.common.set_as_no_drop"));
        return OnEnterResult{2};
    }
    if (!g_inv.ground().has_free_slot())
    {
        txt(i18n::s.get("core.ui.inv.drop.cannot_anymore"));
        snd("core.fail1");
        return OnEnterResult{2};
    }
    if (map_data.max_item_count != 0)
    {
        if (inv_count(g_inv.ground()) >= map_data.max_item_count)
        {
            if (the_item_db[selected_item->id]->category !=
                ItemCategory::furniture)
            {
                txt(i18n::s.get("core.ui.inv.drop.cannot_anymore"));
                snd("core.fail1");
                return OnEnterResult{2};
            }
        }
    }
    if (selected_item->number() > 1)
    {
        txt(i18n::s.get(
            "core.ui.inv.drop.how_many",
            selected_item->number(),
            selected_item));
        input_number_dialog(
            (windoww - 200) / 2 + inf_screenx,
            winposy(60),
            selected_item->number());
        in = elona::stoi(inputlog(0));
        if (in > selected_item->number())
        {
            in = selected_item->number();
        }
        if (in == 0 || rtval == -1)
        {
            return OnEnterResult{2};
        }
    }
    else
    {
        in = 1;
    }
    savecycle();
    item_drop(selected_item, in);
    if (dropcontinue)
    {
        menucycle = true;
        return OnEnterResult{1};
    }
    result.turn_result = TurnResult::turn_end;
    return OnEnterResult{result};
}



OnEnterResult on_enter_external_inventory(
    const ItemRef& selected_item,
    MenuResult& result,
    bool dropcontinue,
    optional_ref<Character> inventory_owner)
{
    if (invctrl != 3 && invctrl != 22)
    {
        if (selected_item->is_no_drop)
        {
            snd("core.fail1");
            txt(i18n::s.get("core.ui.inv.common.set_as_no_drop"));
            return OnEnterResult{2};
        }
    }
    if (invctrl == 24)
    {
        if (invctrl(1) == 3 || invctrl(1) == 5)
        {
            if (inv_count(g_inv.tmp()) >= invcontainer)
            {
                snd("core.fail1");
                txt(i18n::s.get("core.ui.inv.put.container.full"));
                return OnEnterResult{2};
            }
        }
        if (invctrl(1) == 5)
        {
            if (selected_item->weight >= efp * 100)
            {
                snd("core.fail1");
                txt(i18n::s.get(
                    "core.ui.inv.put.container.too_heavy",
                    cnvweight(efp * 100)));
                return OnEnterResult{2};
            }
            if (selected_item->weight <= 0)
            {
                snd("core.fail1");
                txt(i18n::s.get("core.ui.inv.put.container.cannot_hold_cargo"));
                return OnEnterResult{2};
            }
        }
        if (invctrl(1) == 5)
        {
            if (!action_sp(cdata.player(), 10))
            {
                txt(i18n::s.get("core.magic.common.too_exhausted"));
                return OnEnterResult{*on_cancel(dropcontinue)};
            }
        }
    }
    if (invctrl == 22)
    {
        if (invctrl(1) == 1)
        {
            if (game_data.rights_to_succeed_to < 1)
            {
                txt(i18n::s.get("core.ui.inv.take.no_claim"));
                return OnEnterResult{2};
            }
        }
        if (invctrl(1) == 5)
        {
            if (!action_sp(cdata.player(), 10))
            {
                txt(i18n::s.get("core.magic.common.too_exhausted"));
                return OnEnterResult{*on_cancel(dropcontinue)};
            }
        }
    }
    if (selected_item->own_state > OwnState::none &&
        selected_item->own_state < OwnState::shelter)
    {
        snd("core.fail1");
        if (selected_item->own_state == OwnState::shop)
        {
            txt(i18n::s.get("core.action.get.cannot_carry"),
                Message::only_once{true});
        }
        if (selected_item->own_state == OwnState::town)
        {
            txt(i18n::s.get("core.action.get.not_owned"),
                Message::only_once{true});
        }
        update_screen();
        result.turn_result = TurnResult::pc_turn_user_error;
        return OnEnterResult{result};
    }
    page_save();
    if (mode == 6 && selected_item->number() > 1 && invctrl != 22)
    {
        if (invctrl == 11)
        {
            txt(i18n::s.get(
                "core.ui.inv.buy.how_many",
                selected_item->number(),
                selected_item));
        }
        if (invctrl == 12)
        {
            txt(i18n::s.get(
                "core.ui.inv.sell.how_many",
                selected_item->number(),
                selected_item));
        }
        input_number_dialog(
            (windoww - 200) / 2 + inf_screenx,
            winposy(60),
            selected_item->number());
        in = elona::stoi(inputlog(0));
        if (in > selected_item->number())
        {
            in = selected_item->number();
        }
        if (in == 0 || rtval == -1)
        {
            screenupdate = -1;
            update_screen();
            return OnEnterResult{2};
        }
    }
    else
    {
        in = selected_item->number();
    }
    if (mode == 6 && invctrl != 22 && invctrl != 24)
    {
        if (!g_config.skip_confirm_at_shop())
        {
            if (invctrl == 11)
            {
                txt(i18n::s.get(
                    "core.ui.inv.buy.prompt",
                    itemname(selected_item, in),
                    (in * calcitemvalue(selected_item, 0))));
            }
            if (invctrl == 12)
            {
                txt(i18n::s.get(
                    "core.ui.inv.sell.prompt",
                    itemname(selected_item, in),
                    (in * calcitemvalue(selected_item, 1))));
            }
            if (!yes_no())
            {
                screenupdate = -1;
                update_screen();
                return OnEnterResult{1};
            }
        }
        if (invctrl == 11)
        {
            if (calcitemvalue(selected_item, 0) * in > cdata.player().gold)
            {
                screenupdate = -1;
                update_screen();
                txt(i18n::s.get("core.ui.inv.buy.not_enough_money"));
                return OnEnterResult{1};
            }
        }
        if (invctrl == 12)
        {
            if (inventory_owner->role != Role::trader)
            {
                if (calcitemvalue(selected_item, 1) * in >
                    inventory_owner->gold)
                {
                    screenupdate = -1;
                    update_screen();
                    txt(i18n::s.get(
                        "core.ui.inv.sell.not_enough_money", *inventory_owner));
                    return OnEnterResult{1};
                }
            }
        }
    }
    auto& destination_inventory =
        (invctrl == 12 || (invctrl == 24 && invctrl(1) != 0)) ? g_inv.tmp()
                                                              : g_inv.pc();
    int stat =
        pick_up_item(destination_inventory, selected_item, inventory_owner)
            .type;
    if (stat == 0)
    {
        return OnEnterResult{1};
    }
    if (stat == -1)
    {
        result.turn_result = TurnResult::turn_end;
        return OnEnterResult{result};
    }
    if (invctrl == 22)
    {
        if (invctrl(1) == 1)
        {
            --game_data.rights_to_succeed_to;
            if (invctrl(1) == 1)
            {
                txt(i18n::s.get(
                    "core.ui.inv.take.can_claim_more",
                    game_data.rights_to_succeed_to));
            }
        }
        if (invctrl(1) == 4)
        {
            ++game_data.quest_flags.gift_count_of_little_sister;
            invsubroutine = 0;
            result.succeeded = true;
            return OnEnterResult{result};
        }
    }
    screenupdate = -1;
    update_screen();
    return OnEnterResult{1};
}



OnEnterResult on_enter_eat(const ItemRef& selected_item, MenuResult& result)
{
    if (selected_item->is_no_drop)
    {
        snd("core.fail1");
        txt(i18n::s.get("core.ui.inv.common.set_as_no_drop"));
        return OnEnterResult{2};
    }
    screenupdate = -1;
    update_screen();
    savecycle();
    if (cdata.player().nutrition > 10000)
    {
        txt(i18n::s.get("core.ui.inv.eat.too_bloated"));
        update_screen();
        result.turn_result = TurnResult::pc_turn_user_error;
        return OnEnterResult{result};
    }
    result.turn_result = do_eat_command(cdata.player(), selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_equip(const ItemRef& selected_item, MenuResult& result)
{
    if (trait(161) != 0)
    {
        if (selected_item->weight >= 1000)
        {
            txt(i18n::s.get("core.ui.inv.equip.too_heavy"));
            return OnEnterResult{2};
        }
    }
    equip_item(cdata.player(), body - 100, selected_item);
    chara_refresh(cdata.player());
    screenupdate = -1;
    update_screen();
    snd("core.equip1");
    Message::instance().linebreak();
    txt(i18n::s.get("core.ui.inv.equip.you_equip", selected_item));
    game_data.player_is_changing_equipment = 1;
    switch (selected_item->curse_state)
    {
    case CurseState::doomed:
        txt(i18n::s.get("core.ui.inv.equip.doomed", cdata.player()));
        break;
    case CurseState::cursed:
        txt(i18n::s.get("core.ui.inv.equip.cursed", cdata.player()));
        break;
    case CurseState::none: break;
    case CurseState::blessed:
        txt(i18n::s.get("core.ui.inv.equip.blessed", cdata.player()));
        break;
    }
    if (cdata.player().equipment_slots[body - 100].type == 5)
    {
        equip_melee_weapon(cdata.player());
    }
    menucycle = true;
    result.turn_result = TurnResult::menu_equipment;
    return OnEnterResult{result};
}



OnEnterResult on_enter_read(const ItemRef& selected_item, MenuResult& result)
{
    screenupdate = -1;
    update_screen();
    savecycle();
    result.turn_result = do_read_command(cdata.player(), selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_drink(const ItemRef& selected_item, MenuResult& result)
{
    screenupdate = -1;
    update_screen();
    savecycle();
    result.turn_result = do_drink_command(cdata.player(), selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_zap(const ItemRef& selected_item, MenuResult& result)
{
    screenupdate = -1;
    update_screen();
    savecycle();
    result.turn_result = do_zap_command(selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_give(
    const ItemRef& selected_item,
    MenuResult& result,
    Character& inventory_owner)
{
    if (selected_item->is_no_drop)
    {
        snd("core.fail1");
        txt(i18n::s.get("core.ui.inv.common.set_as_no_drop"));
        return OnEnterResult{2};
    }
    if (inventory_owner.sleep)
    {
        txt(i18n::s.get("core.ui.inv.give.is_sleeping", inventory_owner));
        snd("core.fail1");
        return OnEnterResult{2};
    }
    const auto slot_opt = inv_get_free_slot(g_inv.for_chara(inventory_owner));
    if (!slot_opt)
    {
        txt(i18n::s.get("core.ui.inv.give.inventory_is_full", inventory_owner));
        snd("core.fail1");
        return OnEnterResult{2};
    }
    const auto slot = *slot_opt;
    reftype = (int)the_item_db[selected_item->id]->category;
    if (selected_item->id == "core.gift")
    {
        txt(i18n::s.get(
            "core.ui.inv.give.present.text", inventory_owner, selected_item));
        selected_item->modify_number(-1);
        txt(i18n::s.get("core.ui.inv.give.present.dialog", inventory_owner));
        chara_modify_impression(
            inventory_owner, giftvalue(selected_item->param4));
        inventory_owner.emotion_icon = 317;
        refresh_burden_state();
        if (invally == 1)
        {
            return OnEnterResult{1};
        }
        update_screen();
        result.turn_result = TurnResult::turn_end;
        return OnEnterResult{result};
    }
    f = 0;
    p = inventory_owner.get_skill(10).level * 500 +
        inventory_owner.get_skill(11).level * 500 +
        inventory_owner.get_skill(153).level * 2500 + 25000;
    if (inventory_owner.id == CharaId::golden_knight)
    {
        p *= 5;
    }
    if (inv_weight(g_inv.for_chara(inventory_owner)) + selected_item->weight >
        p)
    {
        f = 1;
    }
    if (inventory_owner.id != CharaId::golden_knight)
    {
        if (reftype == 60000)
        {
            f = 2;
        }
        if (reftype == 64000)
        {
            f = 3;
        }
    }
    if (selected_item->weight < 0)
    {
        f = 4;
    }
    if (f)
    {
        snd("core.fail1");
        txt(i18n::s.get_enum(
            "core.ui.inv.give.refuse_dialog", f - 1, inventory_owner));
        return OnEnterResult{2};
    }
    f = 0;
    if (inventory_owner.relationship == 10)
    {
        f = 1;
    }
    else
    {
        if (selected_item->identify_state <= IdentifyState::partly)
        {
            snd("core.fail1");
            txt(i18n::s.get("core.ui.inv.give.too_creepy", inventory_owner));
            return OnEnterResult{2};
        }
        if (is_cursed(selected_item->curse_state))
        {
            snd("core.fail1");
            txt(i18n::s.get("core.ui.inv.give.cursed", inventory_owner));
            return OnEnterResult{2};
        }
        if (reftype == 53000)
        {
            f = 1;
            if (strutil::contains(
                    the_item_db[selected_item->id]->filter, u8"/neg/"))
            {
                f = 0;
            }
            // scroll of teleport/treasure map/deeds
            switch (the_item_db[selected_item->id]->legacy_id)
            {
            case 16:
            case 245:
            case 621:
            case 344:
            case 521:
            case 522:
            case 542:
            case 543:
            case 572:
            case 712: f = 0; break;
            default: break;
            }
        }
        if (reftype == 52000)
        {
            f = 1;
            if (the_item_db[selected_item->id]->subcategory == 52002)
            {
                if (inventory_owner.drunk)
                {
                    snd("core.fail1");
                    txt(i18n::s.get(
                        "core.ui.inv.give.no_more_drink", inventory_owner));
                    return OnEnterResult{2};
                }
            }
            if (strutil::contains(
                    the_item_db[selected_item->id]->filter, u8"/neg/"))
            {
                f = 0;
            }
            if (strutil::contains(
                    the_item_db[selected_item->id]->filter, u8"/nogive/"))
            {
                f = 0;
            }
            if (inventory_owner.is_pregnant())
            {
                if (selected_item->id == "core.poison" ||
                    selected_item->id == "core.bottle_of_dye" ||
                    selected_item->id == "core.bottle_of_sulfuric")
                {
                    f = 1;
                    txt(i18n::s.get("core.ui.inv.give.abortion"));
                }
            }
        }
    }
    if (f)
    {
        snd("core.equip1");
        txt(i18n::s.get(
            "core.ui.inv.give.you_hand", selected_item, inventory_owner));
        if (selected_item->id == "core.engagement_ring" ||
            selected_item->id == "core.engagement_amulet")
        {
            txt(i18n::s.get("core.ui.inv.give.engagement", inventory_owner),
                Message::color{ColorIndex::green});
            chara_modify_impression(inventory_owner, 15);
            inventory_owner.emotion_icon = 317;
        }
        if (selected_item->id == "core.love_potion")
        {
            txt(i18n::s.get(
                    "core.ui.inv.give.love_potion.text",
                    inventory_owner,
                    selected_item),
                Message::color{ColorIndex::purple});
            snd("core.crush2");
            txt(i18n::s.get(
                    "core.ui.inv.give.love_potion.dialog", inventory_owner),
                Message::color{ColorIndex::cyan});
            chara_modify_impression(inventory_owner, -20);
            inventory_owner.emotion_icon = 318;
            selected_item->modify_number(-1);
            return OnEnterResult{1};
        }
        const auto handed_over_item = item_separate(selected_item, slot, 1);
        const auto stacked_item =
            inv_stack(g_inv.for_chara(inventory_owner), handed_over_item, true)
                .stacked_item;
        chara_set_ai_item(inventory_owner, stacked_item);
        wear_most_valuable_equipment_for_all_body_parts(inventory_owner);
        if (inventory_owner.is_player_or_ally())
        {
            create_pcpic(inventory_owner);
        }
        chara_refresh(inventory_owner);
        refresh_burden_state();
        if (invally == 1)
        {
            return OnEnterResult{1};
        }
        update_screen();
        result.turn_result = TurnResult::turn_end;
        return OnEnterResult{result};
    }
    snd("core.fail1");
    txt(i18n::s.get(
        "core.ui.inv.give.refuses", inventory_owner, selected_item));
    return OnEnterResult{2};
}



OnEnterResult on_enter_identify(
    const ItemRef& selected_item,
    MenuResult& result)
{
    screenupdate = -1;
    update_screen();
    const auto identify_result = item_identify(selected_item, efp);
    if (identify_result == IdentifyState::unidentified)
    {
        txt(i18n::s.get("core.ui.inv.identify.need_more_power"));
    }
    else if (identify_result != IdentifyState::completely)
    {
        txt(i18n::s.get("core.ui.inv.identify.partially", selected_item));
    }
    else
    {
        txt(i18n::s.get("core.ui.inv.identify.fully", selected_item));
    }
    inv_stack(g_inv.pc(), selected_item, true);
    refresh_burden_state();
    invsubroutine = 0;
    result.succeeded = true;
    return OnEnterResult{result};
}



OnEnterResult on_enter_use(const ItemRef& selected_item, MenuResult& result)
{
    savecycle();
    result.turn_result = do_use_command(selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_cook(const ItemRef& selected_item, MenuResult& result)
{
    screenupdate = -1;
    update_screen();
    invsubroutine = 0;
    result.succeeded = true;
    return OnEnterResult{result, selected_item};
}



OnEnterResult on_enter_open(const ItemRef& selected_item, MenuResult& result)
{
    screenupdate = -1;
    update_screen();
    savecycle();
    result.turn_result = do_open_command(selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_mix(const ItemRef& selected_item, OptionalItemRef& cidip)
{
    cidip = selected_item;
    savecycle();
    invctrl = 18;
    Message::instance().linebreak();
    snd("core.pop2");
    return OnEnterResult{1};
}



OnEnterResult on_enter_mix_target(
    const ItemRef& selected_item,
    MenuResult& result,
    const OptionalItemRef& cidip)
{
    screenupdate = -1;
    update_screen();
    result.turn_result = do_dip_command(cidip.unwrap(), selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_offer(const ItemRef& selected_item, MenuResult& result)
{
    if (selected_item->is_no_drop)
    {
        snd("core.fail1");
        txt(i18n::s.get("core.ui.inv.common.set_as_no_drop"));
        return OnEnterResult{2};
    }
    screenupdate = -1;
    update_screen();
    savecycle();
    result.turn_result = do_offer_command(selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_trade(
    const ItemRef& selected_item,
    OptionalItemRef& citrade)
{
    citrade = selected_item;
    invctrl = 21;
    snd("core.pop2");
    return OnEnterResult{1};
}



OnEnterResult on_enter_trade_target(
    const ItemRef& selected_item,
    MenuResult& result,
    OptionalItemRef& citrade,
    Character& inventory_owner)
{
    if (selected_item->is_no_drop)
    {
        snd("core.fail1");
        txt(i18n::s.get("core.ui.inv.common.set_as_no_drop"));
        return OnEnterResult{2};
    }
    if (inventory_owner.activity)
    {
        inventory_owner.activity.type = Activity::Type::none;
        inventory_owner.activity.turn = 0;
        inventory_owner.activity.item = nullptr;
    }
    snd("core.equip1");
    citrade->is_quest_target = false;
    txt(i18n::s.get(
        "core.ui.inv.trade.you_receive", selected_item, citrade.unwrap()));
    if (citrade->body_part != 0)
    {
        p = citrade->body_part;
        inventory_owner.equipment_slots[p - 100].unequip();
        citrade->body_part = 0;
    }

    if (inventory_owner.ai_item == citrade)
    {
        inventory_owner.ai_item = nullptr;
    }
    Inventory::exchange(selected_item, citrade.unwrap());
    item_convert_artifact(citrade.unwrap());

    wear_most_valuable_equipment_for_all_body_parts(inventory_owner);
    if (!inventory_owner.is_player_or_ally())
    {
        supply_new_equipment(inventory_owner);
    }
    inv_make_free_slot_force(g_inv.for_chara(inventory_owner));
    chara_refresh(inventory_owner);
    refresh_burden_state();
    invsubroutine = 0;
    result.succeeded = true;
    return OnEnterResult{result};
}



OnEnterResult on_enter_target(const ItemRef& selected_item, MenuResult& result)
{
    if (invctrl(1) == 4)
    {
        if (selected_item->is_no_drop)
        {
            snd("core.fail1");
            txt(i18n::s.get("core.ui.inv.common.set_as_no_drop"));
            return OnEnterResult{2};
        }
    }
    item_separate(selected_item);
    invsubroutine = 0;
    result.succeeded = true;
    return OnEnterResult{result, selected_item};
}



OnEnterResult on_enter_put_into(const ItemRef& selected_item)
{
    if (invctrl(1) == 0)
    {
        snd("core.inv");
        if (game_data.current_map == mdata_t::MapId::lumiest)
        {
            game_data.guild.mages_guild_quota -=
                (selected_item->param1 + 1) * selected_item->number();
            if (game_data.guild.mages_guild_quota <= 0)
            {
                game_data.guild.mages_guild_quota = 0;
            }
            txt(i18n::s.get(
                    "core.ui.inv.put.guild.you_deliver", selected_item) +
                    u8"("s +
                    (selected_item->param1 + 1) * selected_item->number() +
                    u8" Guild Point)"s,
                Message::color{ColorIndex::green});
            if (game_data.guild.mages_guild_quota == 0)
            {
                snd("core.complete1");
                txt(i18n::s.get("core.ui.inv.put.guild.you_fulfill"),
                    Message::color{ColorIndex::green});
            }
        }
        else
        {
            quest_data.immediate().extra_info_2 +=
                selected_item->weight * selected_item->number();
            txt(i18n::s.get(
                    "core.ui.inv.put.harvest",
                    selected_item,
                    cnvweight(selected_item->weight * selected_item->number()),
                    cnvweight(quest_data.immediate().extra_info_2),
                    cnvweight(quest_data.immediate().extra_info_1)),
                Message::color{ColorIndex::green});
        }
        selected_item->remove();
        refresh_burden_state();
        return OnEnterResult{1};
    }
    if (invctrl(1) == 2)
    {
        if (cdata.player().gold < selected_item->subname)
        {
            snd("core.fail1");
            txt(i18n::s.get("core.ui.inv.put.tax.not_enough_money"));
            return OnEnterResult{2};
        }
        if (game_data.left_bill <= 0)
        {
            snd("core.fail1");
            txt(i18n::s.get("core.ui.inv.put.tax.do_not_have_to"));
            return OnEnterResult{1};
        }
        cdata.player().gold -= selected_item->subname;
        snd("core.paygold1");
        txt(i18n::s.get("core.ui.inv.put.tax.you_pay", selected_item),
            Message::color{ColorIndex::green});
        selected_item->modify_number(-1);
        --game_data.left_bill;
        screenupdate = -1;
        update_screen();
        return OnEnterResult{1};
    }
    throw "unreachable";
}



OnEnterResult on_enter_receive(
    const ItemRef& selected_item,
    Character& inventory_owner)
{
    const auto slot_opt = inv_get_free_slot(g_inv.pc());
    if (!slot_opt)
    {
        txt(i18n::s.get("core.ui.inv.common.inventory_is_full"));
        return OnEnterResult{2};
    }
    const auto slot = *slot_opt;
    if (the_item_db[selected_item->id]->category == ItemCategory::ore)
    {
        snd("core.fail1");
        txt(i18n::s.get("core.ui.inv.take_ally.refuse_dialog", inventory_owner),
            Message::color{ColorIndex::blue});
        return OnEnterResult{2};
    }
    if (selected_item->body_part != 0)
    {
        if (is_cursed(selected_item->curse_state))
        {
            txt(i18n::s.get("core.ui.inv.take_ally.cursed", selected_item));
            return OnEnterResult{1};
        }
        p = selected_item->body_part;
        inventory_owner.equipment_slots[p - 100].unequip();
        selected_item->body_part = 0;
    }
    if (selected_item->id == "core.engagement_ring" ||
        selected_item->id == "core.engagement_amulet")
    {
        txt(i18n::s.get(
                "core.ui.inv.take_ally.swallows_ring",
                inventory_owner,
                selected_item),
            Message::color{ColorIndex::purple});
        snd("core.offer1");
        chara_modify_impression(inventory_owner, -20);
        inventory_owner.emotion_icon = 318;
        selected_item->modify_number(-1);
        return OnEnterResult{1};
    }
    snd("core.equip1");
    selected_item->is_quest_target = false;
    if (selected_item->id == "core.gold_piece")
    {
        in = selected_item->number();
    }
    else
    {
        in = 1;
    }
    txt(i18n::s.get(
        "core.ui.inv.take_ally.you_take", itemname(selected_item, in)));
    if (selected_item->id == "core.gold_piece")
    {
        earn_gold(cdata.player(), in);
        selected_item->remove();
    }
    else
    {
        const auto received_item = item_separate(selected_item, slot, in);
        const auto stacked_item =
            inv_stack(g_inv.pc(), received_item, true).stacked_item;
        item_convert_artifact(stacked_item);
    }
    wear_most_valuable_equipment_for_all_body_parts(inventory_owner);
    if (inventory_owner.is_player_or_ally())
    {
        create_pcpic(inventory_owner);
    }
    chara_refresh(inventory_owner);
    refresh_burden_state();
    return OnEnterResult{1};
}



OnEnterResult on_enter_throw(const ItemRef& selected_item, MenuResult& result)
{
    savecycle();
    int stat = target_position();
    if (stat != 1)
    {
        if (stat == 0)
        {
            txt(i18n::s.get("core.ui.inv.throw.cannot_see"));
            update_screen();
        }
        result.turn_result = TurnResult::pc_turn_user_error;
        return OnEnterResult{result};
    }
    if (chip_data.for_cell(tlocx, tlocy).effect & 4)
    {
        txt(i18n::s.get("core.ui.inv.throw.location_is_blocked"));
        update_screen();
        result.turn_result = TurnResult::pc_turn_user_error;
        return OnEnterResult{result};
    }
    result.turn_result = do_throw_command(cdata.player(), selected_item);
    return OnEnterResult{result};
}



OnEnterResult on_enter_steal(const ItemRef& selected_item, MenuResult& result)
{
    start_stealing(cdata.player(), selected_item);
    invsubroutine = 0;
    result.succeeded = true;
    return OnEnterResult{result};
}



OnEnterResult on_enter_small_medal(const ItemRef& selected_item)
{
    Message::instance().linebreak();
    const auto slot_opt = inv_get_free_slot(g_inv.pc());
    if (!slot_opt)
    {
        txt(i18n::s.get("core.ui.inv.trade_medals.inventory_full"));
        snd("core.fail1");
        return OnEnterResult{1};
    }
    const auto slot = *slot_opt;
    OptionalItemRef small_medals;
    if ((small_medals =
             item_find("core.small_medal", ItemFindLocation::player_inventory)))
    {
        p = small_medals->number();
    }
    else
    {
        p = 0;
    }
    if (p < calcmedalvalue(selected_item))
    {
        txt(i18n::s.get("core.ui.inv.trade_medals.not_enough_medals"));
        snd("core.fail1");
        return OnEnterResult{1};
    }
    assert(small_medals);
    small_medals->modify_number(-calcmedalvalue(selected_item));
    snd("core.paygold1");
    const auto received_item = item_copy(selected_item, slot);
    txt(i18n::s.get("core.ui.inv.trade_medals.you_receive", received_item));
    const auto stacked_item =
        inv_stack(g_inv.pc(), received_item, true).stacked_item;
    item_convert_artifact(stacked_item, true);
    return OnEnterResult{1};
}



OnEnterResult on_enter(
    optional_ref<Character> inventory_owner,
    int selected_item_index,
    OptionalItemRef& citrade,
    OptionalItemRef& cidip,
    bool dropcontinue)
{
    MenuResult result = {false, false, TurnResult::none};

    const auto selected_item = g_inv[selected_item_index];

    if (!cargocheck(selected_item))
    {
        return OnEnterResult{3};
    }

    if (invctrl == 1)
    {
        return on_enter_examine(selected_item);
    }
    if (invctrl == 2)
    {
        return on_enter_drop(selected_item, result, dropcontinue);
    }
    if (invctrl == 3 || invctrl == 11 || invctrl == 12 || invctrl == 22 ||
        (invctrl == 24 && (invctrl(1) == 3 || invctrl(1) == 5)))
    {
        return on_enter_external_inventory(
            selected_item, result, dropcontinue, inventory_owner);
    }
    if (invctrl == 5)
    {
        return on_enter_eat(selected_item, result);
    }
    if (invctrl == 6)
    {
        return on_enter_equip(selected_item, result);
    }
    if (invctrl == 7)
    {
        return on_enter_read(selected_item, result);
    }
    if (invctrl == 8)
    {
        return on_enter_drink(selected_item, result);
    }
    if (invctrl == 9)
    {
        return on_enter_zap(selected_item, result);
    }
    if (invctrl == 10)
    {
        assert(inventory_owner);
        return on_enter_give(selected_item, result, *inventory_owner);
    }
    if (invctrl == 13)
    {
        return on_enter_identify(selected_item, result);
    }
    if (invctrl == 14)
    {
        return on_enter_use(selected_item, result);
    }
    if (invctrl == 16)
    {
        return on_enter_cook(selected_item, result);
    }
    if (invctrl == 15)
    {
        return on_enter_open(selected_item, result);
    }
    if (invctrl == 17)
    {
        return on_enter_mix(selected_item, cidip);
    }
    if (invctrl == 18)
    {
        return on_enter_mix_target(selected_item, result, cidip);
    }
    if (invctrl == 19)
    {
        return on_enter_offer(selected_item, result);
    }
    if (invctrl == 20)
    {
        return on_enter_trade(selected_item, citrade);
    }
    if (invctrl == 21)
    {
        assert(inventory_owner);
        return on_enter_trade_target(
            selected_item, result, citrade, *inventory_owner);
    }
    if (invctrl == 23)
    {
        return on_enter_target(selected_item, result);
    }
    if (invctrl == 24)
    {
        return on_enter_put_into(selected_item);
    }
    if (invctrl == 25)
    {
        assert(inventory_owner);
        return on_enter_receive(selected_item, *inventory_owner);
    }
    if (invctrl == 26)
    {
        return on_enter_throw(selected_item, result);
    }
    if (invctrl == 27)
    {
        return on_enter_steal(selected_item, result);
    }
    if (invctrl == 28)
    {
        return on_enter_small_medal(selected_item);
    }

    throw "unreachable";
}



bool on_show_description()
{
    if (listmax != 0)
    {
        const auto item_index = list(0, pagesize * page + cs);
        item_show_description(g_inv[item_index]);
        return true;
    }
    return false;
}



bool on_next_page()
{
    if (pagemax != 0)
    {
        snd("core.pop1");
        ++page;
        return true;
    }
    return false;
}



bool on_previous_page()
{
    if (pagemax != 0)
    {
        snd("core.pop1");
        --page;
        return true;
    }
    return false;
}



bool on_change_menu(bool next, bool dropcontinue)
{
    i = get_menu_cycle_type(dropcontinue);
    p = -1;
    for (int cnt = 0; cnt < 12; ++cnt)
    {
        if (invctrl == cycle(cnt, i))
        {
            p = cnt;
            break;
        }
    }
    if (p != -1)
    {
        if (next)
        {
            ++p;
            if (cycle(p, i) == -1)
            {
                p = 0;
            }
        }
        else
        {
            --p;
            if (p < 0)
            {
                p = cyclemax(i);
            }
        }
        if (invctrl != cycle(p, i))
        {
            invctrl = cycle(p, i);
            snd("core.inv");
            screenupdate = -1;
            update_screen();
            return true;
        }
    }
    return false;
}



bool on_switch_mode_2(bool& dropcontinue)
{
    if (invctrl == 1)
    {
        const auto item_index = list(0, pagesize * page + cs);
        if (g_inv[item_index]->is_no_drop)
        {
            g_inv[item_index]->is_no_drop = false;
            txt(i18n::s.get(
                "core.ui.inv.examine.no_drop.unset", g_inv[item_index]));
        }
        else
        {
            g_inv[item_index]->is_no_drop = true;
            txt(i18n::s.get(
                "core.ui.inv.examine.no_drop.set", g_inv[item_index]));
        }
    }
    if (invctrl == 2)
    {
        if (!dropcontinue)
        {
            txt(i18n::s.get("core.ui.inv.drop.multi"));
            dropcontinue = true;
            snd("core.inv");
            screenupdate = -1;
            update_screen();
            return true;
        }
    }
    return false;
}



void on_switch_mode()
{
    g_show_additional_item_info = get_next_enum(g_show_additional_item_info);
    snd("core.pop1");
}



optional<MenuResult> on_cancel(bool dropcontinue)
{
    MenuResult result = {false, false, TurnResult::none};

    savecycle();
    if (invctrl == 22 && invctrl(1) == 0)
    {
        if (listmax > 0)
        {
            txt(i18n::s.get("core.ui.inv.take.really_leave"));
            if (!yes_no())
            {
                return none;
            }
        }
    }

    efcancel = 1;
    if (invsubroutine == 1)
    {
        invsubroutine = 0;
        result.succeeded = false;
        return result;
    }
    if (invctrl == 6)
    {
        screenupdate = -1;
        update_screen();
        menucycle = true;
        result.turn_result = TurnResult::menu_equipment;
        return result;
    }
    if (invctrl == 11 || invctrl == 12 || invctrl == 22 || invctrl == 28)
    {
        shop_load_shoptmp();
        result.succeeded = false;
        return result;
    }
    if (invally == 1)
    {
        invally = 0;
    }
    if (dropcontinue)
    {
        result.turn_result = TurnResult::turn_end;
        return result;
    }
    screenupdate = 0;
    update_screen();
    result.turn_result = TurnResult::pc_turn_user_error;
    return result;
}



bool on_assign_shortcut(const std::string& action, int shortcut)
{
    snd("core.ok1");
    p = the_item_db[g_inv[list(0, pagesize * page + cs)]->id]->legacy_id +
        invctrl * 10000;
    if (game_data.skill_shortcuts.at(shortcut) == p)
    {
        game_data.skill_shortcuts.at(shortcut) = 0;
        return true;
    }
    for (int cnt = 0; cnt < 20; ++cnt)
    {
        if (game_data.skill_shortcuts.at(cnt) == p)
        {
            game_data.skill_shortcuts.at(cnt) = 0;
        }
    }
    game_data.skill_shortcuts.at(shortcut) = p;
    txt(i18n::s.get(
        "core.ui.assign_shortcut",
        get_bound_shortcut_key_name_by_action_id(action)));
    return true;
}

} // namespace



CtrlInventoryResult ctrl_inventory(optional_ref<Character> inventory_owner)
{
    OptionalItemRef mainweapon;
    OptionalItemRef citrade;
    OptionalItemRef cidip;
    bool dropcontinue = false;

    remove_card_and_figure_from_heir_trunk();

    bool init = true;
    bool update_page = true;
    while (true)
    {
        if (init)
        {
            init = false;

            fallback_to_default_command_if_unavailable();
            restore_cursor();
            mainweapon = nullptr;
            make_item_list(inventory_owner, mainweapon, citrade, cidip);
            if (const auto result = check_command(inventory_owner, citrade))
            {
                return {*result, nullptr};
            }
            sort_list_by_column1();
            if (const auto result = check_pick_up())
            {
                return {*result, nullptr};
            }
            show_message(citrade, cidip);
        }

        if (update_page)
        {
            update_page = false;

            if (auto result = on_shortcut(citrade, cidip, dropcontinue))
            {
                switch (result->type)
                {
                case 0: return {result->menu_result, result->selected_item};
                case 1:
                    init = true;
                    update_page = true;
                    continue;
                case 2: update_page = true; continue;
                case 3: continue;
                default: assert(0); throw "unreachable";
                }
            }
            set_cursor();
            draw_menu(dropcontinue);
        }

        draw_window(inventory_owner, dropcontinue);
        update_key_list();
        draw_item_list(mainweapon);
        save_csbk();
        show_money(inventory_owner);
        redraw();
        const auto action = get_action();
        if (p != -1)
        {
            auto result =
                on_enter(inventory_owner, p(0), citrade, cidip, dropcontinue);
            switch (result.type)
            {
            case 0: return {result.menu_result, result.selected_item};
            case 1:
                init = true;
                update_page = true;
                continue;
            case 2: update_page = true; continue;
            case 3: continue;
            default: assert(0); throw "unreachable";
            }
        }
        if (action == "identify")
        {
            if (on_show_description())
            {
                init = true;
                update_page = true;
                continue;
            }
        }
        else if (action == "next_page")
        {
            if (on_next_page())
            {
                update_page = true;
                continue;
            }
        }
        else if (action == "previous_page")
        {
            if (on_previous_page())
            {
                update_page = true;
                continue;
            }
        }
        else if (action == "next_menu" || action == "previous_menu")
        {
            if (menucycle)
            {
                if (on_change_menu(action == "next_menu", dropcontinue))
                {
                    init = true;
                    update_page = true;
                    continue;
                }
            }
        }
        else if (action == "switch_mode_2")
        {
            if (on_switch_mode_2(dropcontinue))
            {
                init = true;
                update_page = true;
                continue;
            }
        }
        else if (action == "switch_mode")
        {
            on_switch_mode();
            update_page = true;
            continue;
        }
        else if (action == "cancel")
        {
            if (const auto result = on_cancel(dropcontinue))
            {
                return {*result, nullptr};
            }
            else
            {
                update_page = true;
                continue;
            }
        }
        else if (auto shortcut = get_shortcut(action))
        {
            if (can_assign_shortcut(invctrl(0)))
            {
                const auto success = on_assign_shortcut(action, *shortcut);
                if (success)
                {
                    update_page = true;
                    continue;
                }
                else
                {
                    init = true;
                    update_page = true;
                    continue;
                }
            }
        }
    }
}

} // namespace elona
