#include "input_prompt.hpp"

#include "audio.hpp"
#include "blending.hpp"
#include "draw.hpp"
#include "i18n.hpp"
#include "input.hpp"
#include "keybind/keybind.hpp"
#include "ui.hpp"
#include "variables.hpp"



namespace elona
{

int _show_prompt_val{};

int Prompt::query(int x, int y, int width)
{
    snd("core.pop2");

    int csprev = cs;
    cs = 0;
    cs_bk = -1;

    snail::Input::instance().clear_pressed_keys_and_modifiers();

    _replace_null_keys_from_key_select();
    _draw_keys_and_background(x, y, width);

    _draw_box();

    while (1)
    {
        gmode(2);
        _draw_window();

        _draw_main_frame(width);
        _draw_entries();

        _draw_window2();

        redraw();
        auto action = cursor_check_ex();
        int ret = -1;

        auto keys = snail::Input::instance().pressed_keys();

        for (const auto& entry : _entries)
        {
            if (keys.find(entry.key) != keys.end())
            {
                ret = entry.value;
                break;
            }
        }
        if (action == "enter")
        {
            ret = _entries.at(cs).value;
        }
        _modify_result(action);
        if (ret != -1)
        {
            cs = csprev;
            return ret;
        }
        if (_type == Type::can_cancel)
        {
            if (action == "cancel")
            {
                cs = csprev;
                return -1;
            }
        }
    }
}

void Prompt::_replace_null_keys_from_key_select()
{
    int cnt = 0;
    for (auto& entry : _entries)
    {
        if (entry.key == snail::Key::none)
        {
            entry.key = keybind_selection_key_from_index(cnt);
        }
        cnt++;
    }
}

void Prompt::_draw_keys_and_background(int x, int y, int width)
{
    gsel(3);

    int cnt = 0;
    for (auto& entry : _entries)
    {
        draw_select_key(
            keybind_key_short_name(entry.key, false), cnt * 24 + 624, 30);
        cnt++;
    }

    gsel(0);
    sx = x - width / 2;
    sy = y - _promptmax * 10;
    boxf(sx + 12, sy + 12, width - 17, _promptmax * 20 + 25, {60, 60, 60, 128});
    input_halt_input(HaltInput::force);
}

void Prompt::_draw_main_frame(int width)
{
    window2(sx + 8, sy + 8, width - 16, _promptmax * 20 + 42 - 16, 0, 0);
    draw("core.radar_deco", sx - 16, sy);
    font(14 - en * 2);
}

static void _display_prompt_key(int x, int y, int nth)
{
    gcopy(3, nth * 24 + 624, 30, 24, 18, x, y);
}

void Prompt::_draw_entries()
{
    keyrange = 0;
    cs_listbk();
    int cnt = 0;
    for (const auto& entry : _entries)
    {
        _display_prompt_key(sx + 30, cnt * 20 + sy + 22, cnt);

        auto text = entry.locale_key;
        if (auto text_opt =
                i18n::s.get_optional(_locale_key_root + "." + entry.locale_key))
        {
            text = *text_opt;
        }
        else if (auto text_opt = i18n::s.get_optional(entry.locale_key))
        {
            text = *text_opt;
        }

        cs_list(cs == cnt, text, sx + 56, cnt * 20 + sy + 21);
        key_list(cnt) = u8"aaa";
        ++keyrange;
        ++cnt;
    }
    cs_bk = cs;
}



PromptWithNumber::Result PromptWithNumber::query(int x, int y, int width)
{
    const auto index = Prompt::query(x, y, width);
    return {index, _show_prompt_val};
}


void PromptWithNumber::_draw_box()
{
    dx(0) = 200;
    dx(1) = 10;
    dy = sy + 140;
    _max = _number;
    _number = 1;
    _show_prompt_val = 1;
    if (strlen_u(std::to_string(_max)) >= 3)
    {
        dx += std::to_string(_max).size() * 8;
    }
    boxf(dx(1) + sx + 24, dy + 4, dx - 42, 35, {0, 0, 0, 127});
}

void PromptWithNumber::_draw_window()
{
    window2(dx(1) + sx + 20, dy, dx - 40, 36, 0, 2);
    draw("core.label_input", dx(1) + sx + dx / 2 - 56, dy - 32);
    draw("core.arrow_left", dx(1) + sx + 28, dy + 4);
    draw("core.arrow_right", dx(1) + sx + dx - 51, dy + 4);
    const auto inputlog2 =
        ""s + elona::stoi(inputlog(0)) + u8"("s + _max + u8")"s;
    mes(dx(1) + sx + dx - 70 - strlen_u(inputlog2) * 8 + 8,
        dy + 11,
        inputlog2,
        {255, 255, 255});
    inputlog = ""s + _number;
}

void PromptWithNumber::_draw_window2()
{
    window_recipe2(_show_prompt_val);
    font(14 - en * 2);
}

void PromptWithNumber::_modify_result(const std::string& action)
{
    _show_prompt_val = elona::stoi(inputlog(0));
    if (action == "west" || action == "previous_page")
    {
        snd("core.cursor1");
        --_number;
        if (_number < 1)
        {
            _number = _max;
        }
    }
    if (action == "east" || action == "next_page")
    {
        snd("core.cursor1");
        ++_number;
        if (_number > _max)
        {
            _number = 1;
        }
    }
}

} // namespace elona
