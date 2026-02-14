#pragma once
#include <array>
#include <algorithm>
#include <tuple>
#include <vector>
#include "slope.hpp"

template<class V>
class EdgeWalker {
public:
    using Iterator = typename std::vector<V>::iterator;

    // Walk polygon edges in top-left order to generate scanlines.
    EdgeWalker(std::vector<V>& points, int width)
        : begin(points.begin()), end(points.end()), screenWidth(width) {
        auto cmp_top_left = [&](const V& a, const V& b) {
            return std::tie(a.p_y, a.p_x) < std::tie(b.p_y, b.p_x);
        };
        auto minmax = std::minmax_element(begin, end, cmp_top_left);
        first = minmax.first;
        last = minmax.second;
        cur = { first, first };
    }

    int gety(int side) const { return cur[side]->p_y >> 16; }

    template<typename Fn>
    void walk(Fn&& drawScanlineFn) {
        for(int side = 0, cury = gety(side), nexty[2] = {cury,cury}, hy = cury * screenWidth; cur[side] != last; )
        {
            auto prev = std::move(cur[side]);

            if(side == forwards) cur[side] = (std::next(prev) == end) ? begin : std::next(prev);
            else                 cur[side] = std::prev(prev == begin ? end : prev);

            nexty[side]  = gety(side);
            slopes[side] = Slope<V>(*prev, *cur[side], nexty[side] - cury);

            side = (nexty[0] <= nexty[1]) ? 0 : 1;
            for(int limit = nexty[side]; cury < limit; ++cury, hy+= screenWidth) {
                drawScanline(hy, slopes[0], slopes[1], drawScanlineFn);
            }
        }
    }

private:
    Iterator begin;
    Iterator end;
    Iterator first;
    Iterator last;
    std::array<Iterator, 2> cur;
    int screenWidth;
    int forwards = 0;
    Slope<V> slopes[2] {};

    template<typename Fn>
    void drawScanline(int hy, Slope<V>& left, Slope<V>& right, Fn&& drawScanlineFn) {
        int xStart = left.getx() + hy;
        int xEnd = right.getx() + hy;
        int dx = xEnd - xStart;

        if (dx > 0) {
            drawScanlineFn(xStart, xEnd, dx, left, right);
        }

        left.down();
        right.down();
    }
};
