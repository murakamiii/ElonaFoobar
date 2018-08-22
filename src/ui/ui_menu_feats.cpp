#include "ui_menu_feats.hpp"
#include "../enchantment.hpp"
#include "../menu.hpp"
#include "../trait.hpp"

namespace elona
{
namespace ui
{

bool ui_menu_feats::init()
{
    if (_decorate)
    {
        listmax = 0;
        page = 0;
        pagesize = 15;
        cs = 0;
        tc = 0;
        cs_bk = -1;
        curmenu = 2;
        snd(96);
        ww = 700;
        wh = 400;
        wx = (windoww - ww) / 2 + inf_screenx;
        wy = winposy(wh);
        window_animation(wx, wy, ww, wh, 9, 4);
        gsel(3);
        pos(960, 96);
        picload(filesystem::dir::graphic() / u8"deco_feat.bmp", 1);
        gsel(0);
        windowshadow = 1;
    }

    return true;
}

void ui_menu_feats::update()
{
    trait_load_desc();

    std::vector<std::string> traits_by_enchantments;
    for (int i = 0; i < 30; ++i)
    {
        if (cdata[tc].body_parts[i] % 10000 != 0)
        {
            ci = cdata[tc].body_parts[i] % 10000 - 1;
            for (const auto& enc : inv[ci].enchantments)
            {
                if (enc.id == 0)
                    break;
                get_enchantment_description(enc.id, enc.power, 0, true);
                if (!s(0).empty())
                {
                    traits_by_enchantments.push_back(s);
                }
            }
        }
    }
    std::sort(
        std::begin(traits_by_enchantments), std::end(traits_by_enchantments));
    traits_by_enchantments.erase(
        std::unique(
            std::begin(traits_by_enchantments),
            std::end(traits_by_enchantments)),
        std::end(traits_by_enchantments));
    for (const auto& trait : traits_by_enchantments)
    {
        list(0, listmax) = 1;
        list(1, listmax) = 99999;
        listn(0, listmax) = i18n::s.get(
            "core.locale.trait.window.his_equipment", cnven(his(tc, 1)), trait);
        ++listmax;
    }
    if (tc != 0)
    {
        for (int cnt = 0, cnt_end = (listmax); cnt < cnt_end; ++cnt)
        {
            if (jp)
            {
                listn(0, cnt) =
                    strutil::replace(listn(0, cnt), u8"あなた", he(tc, 1));
            }
            if (en)
            {
                listn(0, cnt) =
                    strutil::replace(listn(0, cnt), u8" your", his(tc, 1));
                listn(0, cnt) =
                    strutil::replace(listn(0, cnt), u8" you", him(tc, 1));
            }
        }
    }
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
    if (cs < 0)
    {
        for (int cnt = 0, cnt_end = (listmax); cnt < cnt_end; ++cnt)
        {
            if (list(1, cnt) >= 10000)
            {
                if (list(0, cnt) - 10000 == cs)
                {
                    page = cnt / pagesize;
                    cs = cnt % pagesize;
                    break;
                }
            }
        }
        if (cs < 0)
        {
            cs = 0;
        }
    }
}

void ui_menu_feats::draw()
{
    s(0) = i18n::s.get("core.locale.trait.window.title");
    s(1) = i18n::s.get("core.locale.trait.window.enter") + "  " + strhint2
        + strhint3 + u8"z,x ["s + i18n::s.get("core.locale.trait.window.ally")
        + u8"]"s;
    if (_operation == operation::character_making)
    {
        i = 1;
    }
    else
    {
        i = 0;
    }
    display_window(
        (windoww - 730) / 2 + inf_screenx,
        winposy(430, i) + i * 15,
        730,
        430,
        55,
        40);
    s(0) = i18n::s.get("core.locale.trait.window.name");
    s(1) = i18n::s.get("core.locale.trait.window.level");
    s(2) = i18n::s.get("core.locale.trait.window.detail");
    display_topic(s, wx + 46, wy + 36);
    display_topic(s(2), wx + 255, wy + 36);
    pos(wx + 46, wy - 16);
    gcopy(3, 816, 48, 48, 48);
    pos(wx + ww - 56, wy + wh - 198);
    gcopy(3, 960, 96, 48, 192);
    pos(wx, wy);
    gcopy(3, 1008, 96, 48, 144);
    pos(wx + ww - 108, wy);
    gcopy(3, 960, 288, 96, 72);
    pos(wx, wy + wh - 70);
    gcopy(3, 1008, 240, 96, 48);
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
        if (list(0, p) < 0)
        {
            continue;
        }
        if (cnt % 2 == 0)
        {
            boxf(wx + 57, wy + 66 + cnt * 19, 640, 18, {12, 14, 16, 16});
        }
        if (list(1, p) >= 10000 || list(0, p) < 0)
        {
            continue;
        }
        display_key(wx + 58, wy + 66 + cnt * 19 - 2, cnt);
    }
    if (tc == 0)
    {
        s = i18n::s.get(
            "core.locale.trait.window.you_can_acquire",
            gdata_acquirable_feat_count);
    }
    else
    {
        s = i18n::s.get(
            "core.locale.trait.window.your_trait", cnven(cdatan(0, tc)));
    }
    display_note(s, 50);
    font(14 - en * 2);
    cs_listbk();
    for (int cnt = 0, cnt_end = (pagesize); cnt < cnt_end; ++cnt)
    {
        p = pagesize * page + cnt;
        if (p >= listmax)
        {
            break;
        }
        i = list(0, p);
        if (i < 0)
        {
            cs_list(cs == cnt, listn(0, p), wx + 114, wy + 66 + cnt * 19 - 1);
            continue;
        }
        int text_color{};
        if (list(1, p) != 99999)
        {
            int stat = trait_get_info(0, i);
            //_featrq = stat;
            if (trait(i) == 0)
            {
                text_color = 0;
            }
            else if (trait(i) > 0)
            {
                text_color = 1;
            }
            else
            {
                text_color = 2;
            }
        }
        else
        {
            traitref = 5;
        }
        if (list(1, p) < 10000)
        {
            pos(wx + 30, wy + 61 + cnt * 19);
            x = 84;
        }
        else
        {
            pos(wx + 45, wy + 61 + cnt * 19);
            x = 70;
        }
        gcopy(3, 384 + traitref * 24, 336, 24, 24);
        switch (text_color)
        {
        case 0: color(10, 10, 10); break;
        case 1: color(0, 0, 200); break;
        case 2: color(200, 0, 0); break;
        }
        cs_list(cs == cnt, listn(0, p), wx + x, wy + 66 + cnt * 19 - 1, 0, -1);
        color(0, 0, 0);
        if (list(1, p) < 10000)
        {
            pos(wx + 270, wy + 66 + cnt * 19 + 2);
            switch (text_color)
            {
            case 0: color(10, 10, 10); break;
            case 1: color(0, 0, 200); break;
            case 2: color(200, 0, 0); break;
            }
            mes(traitrefn(2));
            color(0, 0, 0);
        }
    }
    if (keyrange != 0)
    {
        cs_bk = cs;
    }
}

optional<ui_menu_feats::result_type> ui_menu_feats::on_key(
    const std::string& key)
{
    ELONA_GET_SELECTED_INDEX(p);

    if (tc == 0)
    {
        if (gdata_acquirable_feat_count > 0)
        {
            if (p > 0)
            {
                if (list(1, p) < 10000)
                {
                    int tid = list(0, p);
                    trait_get_info(0, tid);
                    if (traitref(2) <= trait(tid))
                    {
                        if (mode != 1)
                        {
                            txt(i18n::s.get(
                                "core.locale.trait.window.already_maxed"));
                        }
                        set_reupdate();
                        return none;
                    }
                    --gdata_acquirable_feat_count;
                    cs = -10000 + tid;
                    snd(61);
                    ++trait(tid);
                    chara_refresh(tc);
                    if (_operation == operation::character_making)
                    {
                        if (gdata_acquirable_feat_count == 0)
                        {
                            // result.succeeded = true
                            return ui_menu_feats::result::finish(true);
                        }
                    }
                    else
                    {
                        render_hud();
                    }
                    set_reupdate();
                    return none;
                }
            }
        }
    }
    if (key == key_pageup)
    {
        if (pagemax != 0)
        {
            snd(1);
            ++page;
            set_reupdate();
        }
    }
    else if (key == key_pagedown)
    {
        if (pagemax != 0)
        {
            snd(1);
            --page;
            set_reupdate();
        }
    }
    else if (key == u8"z"s || key == u8"x"s)
    {
        p = tc;
        for (int cnt = 0; cnt < 16; ++cnt)
        {
            if (key == u8"z"s)
            {
                --p;
                if (p < 0)
                {
                    p = 15;
                }
            }
            if (key == u8"x"s)
            {
                ++p;
                if (p == 16)
                {
                    p = 0;
                }
            }
            if (cdata[p].state() != character::state_t::alive)
            {
                continue;
            }
            break;
        }
        tc = p;
        snd(1);
        page = 0;
        cs = 0;
        set_reupdate();
    }
    else if (key == key_cancel)
    {
        if (mode != 1)
        {
            update_screen();
        }
        // result.turn_result = turn_result_t::pc_turn_user_error
        return ui_menu_feats::result::cancel();
    }
    else if (
        getkey(snail::key::f1) && _operation == operation::character_making)
    {
        show_game_help();
        // result.pressed_f1 = true
        return ui_menu_feats::result::finish(false);
    }

    return none;
}

} // namespace ui
} // namespace elona
