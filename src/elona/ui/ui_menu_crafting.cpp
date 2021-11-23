#include "ui_menu_crafting.hpp"

#include "../ability.hpp"
#include "../audio.hpp"
#include "../crafting.hpp"
#include "../draw.hpp"
#include "../i18n.hpp"
#include "../inventory.hpp"
#include "../item.hpp"
#include "../message.hpp"

namespace elona
{
namespace ui
{

static int _can_produce_item(int created_item_id)
{
    elona_vector1<int> j_at_m110;

    auto recipe = crafting_find_recipe(created_item_id);
    if (!recipe)
    {
        return -1;
    }

    if (recipe->required_skill_level >
        cdata.player().get_skill(recipe->skill_used).level)
    {
        return -1;
    }

    for (const auto& required_mat : recipe->required_materials)
    {
        if (mat(required_mat.id) < required_mat.amount)
        {
            return -1;
        }
    }

    return 1;
}

static bool _should_show_entry(int item_id, int _prodtype)
{
    auto recipe = crafting_find_recipe(item_id);
    if (!recipe)
    {
        return false;
    }
    if (_prodtype == 2)
    {
        if (recipe->skill_used != 178)
        {
            return false;
        }
    }
    if (_prodtype == 1)
    {
        if (recipe->skill_used != 176)
        {
            return false;
        }
    }
    if (_prodtype == 3)
    {
        if (recipe->skill_used != 179)
        {
            return false;
        }
    }
    if (_prodtype == 4)
    {
        if (recipe->skill_used != 177)
        {
            return false;
        }
    }
    if (cdata.player().get_skill(recipe->skill_used).level + 3 <
        recipe->required_skill_level)
    {
        return false;
    }

    return true;
}

static void _populate_recipe_list(int _invctrl, int _prodtype)
{
    if (_invctrl == 0)
    {
        for (int item_id = 0, cnt_end = (maxitemid); item_id < cnt_end;
             ++item_id)
        {
            if (_should_show_entry(item_id, _prodtype))
            {
                listn(0, listmax) = ""s + _can_produce_item(item_id);
                list(0, listmax) = item_id;
                list(1, listmax) = 0;
                ++listmax;
            }
        }
    }
    else
    {
        for (int cnt = 0; cnt < 50; ++cnt)
        {
            listn(0, listmax) = ""s + (-1);
            list(0, listmax) = 630;
            list(1, listmax) = cnt;
            ++listmax;
        }
    }
}

bool UIMenuCrafting::init()
{
    listmax = 0;
    page = 0;
    pagesize = 10;
    cs = 0;
    cs_bk = -1;
    page_load();
    windowshadow = 1;

    _populate_recipe_list(_invctrl, _prodtype);

    return true;
}

void UIMenuCrafting::_draw_window()
{
    ui_display_window(
        i18n::s.get("core.crafting.menu.title"),
        strhint2 + strhint3b,
        (windoww - 640) / 2 + inf_screenx,
        winposy(448),
        640,
        448);
    display_topic(i18n::s.get("core.crafting.menu.product"), wx + 28, wy + 36);
    display_topic(i18n::s.get("core.crafting.menu.detail"), wx + 296, wy + 36);
    display_topic(
        i18n::s.get("core.crafting.menu.requirement"), wx + 28, wy + 258);
    display_topic(
        i18n::s.get("core.crafting.menu.material"), wx + 28, wy + 304);
}

void UIMenuCrafting::_draw_key(int cnt)
{
    display_key(wx + 58, wy + 66 + cnt * 19 - 2, cnt);
}

void UIMenuCrafting::_draw_keys()
{
    keyrange = 0;
    for (int cnt = 0, cnt_end = (pagesize); cnt < cnt_end; ++cnt)
    {
        int index = pagesize * page + cnt;
        if (index >= listmax)
        {
            break;
        }
        key_list(cnt) = key_select(cnt);
        ++keyrange;
        if (cnt % 2 == 0)
        {
            boxf(wx + 70, wy + 66 + cnt * 19, 540, 18, {12, 14, 16, 16});
        }
        _draw_key(cnt);
    }
}

void UIMenuCrafting::_draw_recipe_desc(const CraftingRecipe& recipe)
{
    font(13 - en * 2);

    std::string desc = i18n::s.get("core.crafting.menu.skill_needed") + u8": "s;
    if (auto text = i18n::s.get_enum_optional(
            "core.crafting.menu.skills", recipe.skill_used))
    {
        desc += *text;
    }

    desc += u8" "s + recipe.required_skill_level + u8"("s +
        cdata.player().get_skill(recipe.skill_used).level + u8")"s;

    const auto text_color = recipe.required_skill_level <=
            cdata.player().get_skill(recipe.skill_used).level
        ? snail::Color{30, 30, 200}
        : snail::Color{200, 30, 30};
    mes(wx + 37, wy + 288, desc + u8" "s, text_color);
}

void UIMenuCrafting::_draw_single_recipe_required_material(
    int mat_index,
    const RequiredMaterial& required_mat)
{
    std::string mat_desc = matname(required_mat.id) + " " +
        i18n::s.get("core.crafting.menu.x") + " " + required_mat.amount +
        u8"("s + mat(required_mat.id) + u8")"s;

    const auto text_color = mat(required_mat.id) >= required_mat.amount
        ? snail::Color{30, 30, 200}
        : snail::Color{200, 30, 30};
    mes(wx + 37 + mat_index % 3 * 192,
        wy + 334 + mat_index / 3 * 16,
        mat_desc,
        text_color);
}

void UIMenuCrafting::_draw_recipe_required_materials(
    const CraftingRecipe& recipe)
{
    int mat_index = 0;
    for (const auto required_mat : recipe.required_materials)
    {
        _draw_single_recipe_required_material(mat_index, required_mat);
        mat_index++;
    }
}

void UIMenuCrafting::_draw_recipe(int item_id, bool draw_desc)
{
    auto recipe = crafting_find_recipe(item_id);
    assert(recipe);
    if (draw_desc)
    {
        _draw_recipe_desc(*recipe);
    }

    _draw_recipe_required_materials(*recipe);
}

void UIMenuCrafting::_draw_single_list_entry(
    int cnt,
    int item_id,
    bool can_craft)
{
    std::string item_name = ioriginalnameref(item_id);
    std::string item_make = i18n::s.get("core.crafting.menu.make", item_name);

    font(14 - en * 2);

    const auto text_color =
        can_craft ? snail::Color{10, 10, 10} : snail::Color{160, 10, 10};
    cs_list(
        cs == cnt,
        cnven(item_name),
        wx + 86,
        wy + 66 + cnt * 19 - 1,
        0,
        text_color);

    mes(wx + 308, wy + 66 + cnt * 19 + 2, item_make, text_color);

    draw_item_material(ipicref(item_id), wx + 37, wy + 69 + cnt * 19 + 2);
}

bool UIMenuCrafting::_draw_list_entries(bool draw_desc)
{
    bool should_redraw = false;

    cs_listbk();
    for (int cnt = 0, cnt_end = (pagesize); cnt < cnt_end; ++cnt)
    {
        int entry_index = pagesize * page + cnt;
        if (entry_index >= listmax)
        {
            break;
        }

        int item_id = list(0, entry_index);
        bool can_craft = elona::stoi(listn(0, entry_index)) != -1;

        if (cs == cnt)
        {
            _draw_recipe(item_id, draw_desc);
            should_redraw = true;
        }

        _draw_single_list_entry(cnt, item_id, can_craft);
    }

    return should_redraw;
}

void UIMenuCrafting::update()
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

void UIMenuCrafting::draw()
{
    _draw_window();
    _draw_keys();
    bool draw_desc = _invctrl == 0;
    bool should_redraw = _draw_list_entries(draw_desc);

    if (keyrange != 0)
    {
        cs_bk = cs;
    }
    if (should_redraw || listmax == 0)
    {
        redraw();
    }
}

optional<UIMenuCrafting::ResultType> UIMenuCrafting::on_key(
    const std::string& action)
{
    if (auto created_item_id = get_selected_item())
    {
        s = ioriginalnameref(*created_item_id);
        if (_can_produce_item(*created_item_id) == -1)
        {
            snd("core.fail1");
            txt(i18n::s.get("core.crafting.you_do_not_meet_requirements"));
            set_reupdate();
            return none;
        }
        if (!inv_player()->has_free_slot())
        {
            snd("core.fail1");
            txt(i18n::s.get("core.ui.inv.common.inventory_is_full"));
            set_reupdate();
            return none;
        }
        return UIMenuCrafting::Result::finish(*created_item_id);
    }
    else if (action == "next_page")
    {
        if (pagemax != 0)
        {
            snd("core.pop1");
            ++page;
            set_reupdate();
        }
    }
    else if (action == "previous_page")
    {
        if (pagemax != 0)
        {
            snd("core.pop1");
            --page;
            set_reupdate();
        }
    }
    else if (action == "cancel")
    {
        return UIMenuCrafting::Result::cancel();
    }

    return none;
}


} // namespace ui
} // namespace elona
