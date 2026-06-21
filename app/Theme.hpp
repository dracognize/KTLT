#pragma once

#include <ftxui/screen/color.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

namespace theme {
    
    inline const auto Rosewater = ftxui::Color::RGB(245, 224, 220);
    inline const auto Flamingo  = ftxui::Color::RGB(242, 205, 205);
    inline const auto Pink      = ftxui::Color::RGB(245, 194, 231);
    inline const auto Mauve     = ftxui::Color::RGB(203, 166, 247);
    inline const auto Red       = ftxui::Color::RGB(243, 139, 168);
    inline const auto Maroon    = ftxui::Color::RGB(235, 160, 172);
    inline const auto Peach     = ftxui::Color::RGB(250, 179, 135);
    inline const auto Yellow    = ftxui::Color::RGB(249, 226, 175);
    inline const auto Green     = ftxui::Color::RGB(166, 227, 161);
    inline const auto Teal      = ftxui::Color::RGB(148, 226, 213);
    inline const auto Sky       = ftxui::Color::RGB(137, 220, 235);
    inline const auto Sapphire  = ftxui::Color::RGB(116, 199, 236);
    inline const auto Blue      = ftxui::Color::RGB(137, 180, 250);
    inline const auto Lavender  = ftxui::Color::RGB(180, 190, 254);
    
    inline const auto Text      = ftxui::Color::RGB(205, 214, 244);
    inline const auto Subtext1  = ftxui::Color::RGB(186, 194, 222);
    inline const auto Subtext0  = ftxui::Color::RGB(166, 173, 200);
    
    inline const auto Overlay2  = ftxui::Color::RGB(147, 153, 178);
    inline const auto Overlay1  = ftxui::Color::RGB(127, 132, 156);
    inline const auto Overlay0  = ftxui::Color::RGB(108, 112, 134);
    
    inline const auto Surface2  = ftxui::Color::RGB(88, 91, 112);
    inline const auto Surface1  = ftxui::Color::RGB(69, 71, 90);
    inline const auto Surface0  = ftxui::Color::RGB(49, 50, 68);
    
    inline const auto Base      = ftxui::Color::RGB(30, 30, 46);
    inline const auto Mantle    = ftxui::Color::RGB(24, 24, 37);
    inline const auto Crust     = ftxui::Color::RGB(17, 17, 27);

    
    inline auto Button(ftxui::Color color) {
        auto option = ftxui::ButtonOption::Border();
        option.transform = [color](const ftxui::EntryState &s) {
            auto element = ftxui::text(s.label) | ftxui::center;
            if (s.focused) {
                return element | ftxui::bold | ftxui::color(theme::Base) | ftxui::bgcolor(color) | ftxui::border;
            }
            return element | ftxui::color(color) | ftxui::border;
        };
        return option;
    }
} 
