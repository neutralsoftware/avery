/*
* types.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ${FILE_DESCRIPTION}
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_GRAPHICSTYPES_H
#define AVERY_GRAPHICSTYPES_H

struct Color {
    int r; // from 0-255
    int g;
    int b;
    int a;

    constexpr Color() : r(0), g(0), b(0), a(255) {
    }

    constexpr Color(int r_, int g_, int b_, int a_ = 255) : r(r_), g(g_), b(b_), a(a_) {
    }

    static const Color red;
    static const Color green;
    static const Color blue;
    static const Color yellow;
    static const Color magenta;
    static const Color cyan;
    static const Color white;
    static const Color black;
    static const Color orange;
    static const Color purple;
    static const Color brown;
    static const Color gray;
    static const Color lightGray;
    static const Color darkGray;
    static const Color pink;
    static const Color lime;
    static const Color navy;
    static const Color teal;
    static const Color olive;
    static const Color maroon;
    static const Color silver;
    static const Color gold;
    static const Color skyBlue;
    static const Color lavender;
    static const Color coral;
    static const Color beige;
    static const Color mint;
    static const Color ochre;
    static const Color indigo;
    static const Color sienna;
    static const Color turquoise;
    static const Color khaki;
};

inline const Color Color::red = Color(255, 0, 0, 255);
inline const Color Color::green = Color(0, 255, 0, 255);
inline const Color Color::blue = Color(0, 0, 255, 255);
inline const Color Color::yellow = Color(255, 255, 0, 255);
inline const Color Color::magenta = Color(255, 0, 255, 255);
inline const Color Color::cyan = Color(0, 255, 255, 255);
inline const Color Color::white = Color(255, 255, 255, 255);
inline const Color Color::black = Color(0, 0, 0, 255);
inline const Color Color::orange = Color(255, 165, 0, 255);
inline const Color Color::purple = Color(128, 0, 128, 255);
inline const Color Color::brown = Color(165, 42, 42, 255);
inline const Color Color::gray = Color(128, 128, 128, 255);
inline const Color Color::lightGray = Color(211, 211, 211, 255);
inline const Color Color::darkGray = Color(64, 64, 64, 255);
inline const Color Color::pink = Color(255, 192, 203, 255);
inline const Color Color::lime = Color(191, 255, 0, 255);
inline const Color Color::navy = Color(0, 0, 128, 255);
inline const Color Color::teal = Color(0, 128, 128, 255);
inline const Color Color::olive = Color(128, 128, 0, 255);
inline const Color Color::maroon = Color(128, 0, 0, 255);
inline const Color Color::silver = Color(192, 192, 192, 255);
inline const Color Color::gold = Color(255, 215, 0, 255);
inline const Color Color::skyBlue = Color(135, 206, 235, 255);
inline const Color Color::lavender = Color(230, 230, 250, 255);
inline const Color Color::coral = Color(255, 127, 80, 255);
inline const Color Color::beige = Color(245, 245, 220, 255);
inline const Color Color::mint = Color(189, 252, 201, 255);
inline const Color Color::ochre = Color(204, 119, 34, 255);
inline const Color Color::indigo = Color(75, 0, 130, 255);
inline const Color Color::sienna = Color(160, 82, 45, 255);
inline const Color Color::turquoise = Color(64, 224, 208, 255);
inline const Color Color::khaki = Color(240, 230, 140, 255);

#endif //AVERY_GRAPHICSTYPES_H
