#include "vsdk/toolkit/render/dithering/Dithering.h"
#include <stdlib.h>
#include <string.h>
#include "vsdk/toolkit/render/dithering/CutEntry.h"
#include "vsdk/toolkit/render/dithering/CutBox.h"

class Dithering::Finder {
public:
    explicit Finder(const Palette &palette) : count(palette.count), cache(0) {
        for (unsigned i = 0; i < count * 3; i++) {
            int v = palette.rgb[i] & 63;
            pal[i] = (v << 2) | (v >> 4);
        }
        cache = (uint16_t *)malloc(CACHE_SIZE * sizeof(uint16_t));
        if (cache) {
            memset(cache, 0xFF, CACHE_SIZE * sizeof(uint16_t));
        }
    }
    ~Finder() {
        if (cache) {
            free(cache);
        }
    }
    Finder(const Finder &) = delete;
    Finder &operator=(const Finder &) = delete;

    int component(unsigned index, unsigned channel) const { return pal[index * 3 + channel]; }

    unsigned nearest(int r, int g, int b) {
        unsigned key = ((unsigned)(r >> 3) << 10) | ((unsigned)(g >> 3) << 5) | (unsigned)(b >> 3);
        if (cache && cache[key] != EMPTY) {
            return cache[key];
        }
        unsigned best = search(r, g, b);
        if (cache) {
            cache[key] = (uint16_t)best;
        }
        return best;
    }

private:
    static constexpr unsigned CACHE_SIZE = 32 * 32 * 32;
    static constexpr uint16_t EMPTY = 0xFFFF;

    unsigned search(int r, int g, int b) const {
        unsigned best = 0;
        int bestDistance = 1 << 30;
        for (unsigned i = 0; i < count; i++) {
            int dr = r - pal[i * 3];
            int dg = g - pal[i * 3 + 1];
            int db = b - pal[i * 3 + 2];
            int distance = dr * dr + dg * dg + db * db;
            if (distance < bestDistance) {
                bestDistance = distance;
                best = i;
            }
        }
        return best;
    }

    unsigned count;
    int pal[MAX_COLORS * 3];
    uint16_t *cache;
};

int
Dithering::bayer(unsigned x, unsigned y) {
    unsigned mixed = x ^ y;
    unsigned v = 0;
    for (unsigned bit = 0; bit < 3; bit++) {
        v = (v << 2) | (((y >> bit) & 1) << 1) | ((mixed >> bit) & 1);
    }
    return (int)v;
}

void
Dithering::orderedRgb565(uint8_t *rgb, unsigned width, unsigned height) {
    if (!rgb) {
        return;
    }
    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            uint8_t *pixel = rgb + ((size_t)y * width + x) * 3;
            unsigned threshold = (unsigned)bayer(x % BAYER_SIZE, y % BAYER_SIZE);
            for (unsigned c = 0; c < 3; c++) {
                unsigned levels = c == 1 ? 63 : 31;
                unsigned scaled = pixel[c] * levels;
                unsigned value = scaled / 255;
                if ((scaled % 255) * 64 > threshold * 255 && value < levels) {
                    value++;
                }
                pixel[c] = (uint8_t)((value * 255 + levels - 1) / levels);
            }
        }
    }
}

unsigned
Dithering::spreadOf(unsigned colors) {
    unsigned levels = 2;
    while (levels * levels * levels < colors) {
        levels++;
    }
    return 255 / (levels - 1);
}

void
Dithering::convertNearest(Finder &finder, const uint8_t *rgb, unsigned width, unsigned height,
                          uint8_t *indices) {
    for (unsigned i = 0; i < width * height; i++) {
        indices[i] = (uint8_t)finder.nearest(rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2]);
    }
}

void
Dithering::convertOrdered(Finder &finder, const uint8_t *rgb, unsigned width, unsigned height,
                          unsigned spread, uint8_t *indices) {
    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            const uint8_t *p = rgb + (y * width + x) * 3;
            int offset = (bayer(x % BAYER_SIZE, y % BAYER_SIZE) * 2 + 1 - 64) * (int)spread / 128;
            indices[y * width + x] = (uint8_t)finder.nearest(clamp(p[0] + offset), clamp(p[1] + offset), clamp(p[2] + offset));
        }
    }
}

bool
Dithering::convertFloydSteinberg(Finder &finder, const uint8_t *rgb, unsigned width, unsigned height,
                                 uint8_t *indices) {
    unsigned cells = (width + 2) * 3;
    int *rows = (int *)malloc(2 * cells * sizeof(int));
    if (!rows) {
        return false;
    }
    memset(rows, 0, 2 * cells * sizeof(int));
    int *current = rows;
    int *next = rows + cells;
    for (unsigned y = 0; y < height; y++) {
        bool leftToRight = (y & 1) == 0;
        int step = leftToRight ? 1 : -1;
        memset(next, 0, cells * sizeof(int));
        for (unsigned i = 0; i < width; i++) {
            unsigned x = leftToRight ? i : width - 1 - i;
            const uint8_t *p = rgb + (y * width + x) * 3;
            int *here = current + (x + 1) * 3;
            int wanted[3];
            for (unsigned c = 0; c < 3; c++) {
                wanted[c] = clamp(p[c] + here[c] / 16);
            }
            unsigned index = finder.nearest(wanted[0], wanted[1], wanted[2]);
            indices[y * width + x] = (uint8_t)index;
            int *ahead = here + step * 3;
            int *below = next + (x + 1) * 3;
            for (int c = 0; c < 3; c++) {
                int error = wanted[c] - finder.component(index, (unsigned)c);
                ahead[c] += error * 7;
                below[c - step * 3] += error * 3;
                below[c] += error * 5;
                below[c + step * 3] += error;
            }
        }
        int *swap = current;
        current = next;
        next = swap;
    }
    free(rows);
    return true;
}

bool
Dithering::convert(Method method, const uint8_t *rgb, unsigned width, unsigned height, const Palette &palette,
                   uint8_t *indices) {
    if (palette.count == 0 || palette.count > MAX_COLORS) {
        return false;
    }
    Finder finder(palette);
    switch (method) {
    case Method::Nearest:
        convertNearest(finder, rgb, width, height, indices);
        return true;
    case Method::Ordered:
        convertOrdered(finder, rgb, width, height, spreadOf(palette.count), indices);
        return true;
    case Method::FloydSteinberg:
        return convertFloydSteinberg(finder, rgb, width, height, indices);
    }
    return false;
}

static const unsigned CUT_SHIFT[3] = {10, 5, 0};

static unsigned
cutChannel(const CutEntry &e, unsigned channel) { return (e.key >> CUT_SHIFT[channel]) & 31; }

static void
measureBox(const CutEntry *entries, CutBox *box) {
    box->pixels = 0;
    for (unsigned c = 0; c < 3; c++) {
        box->low[c] = 31;
        box->high[c] = 0;
    }
    for (unsigned i = box->first; i < box->end; i++) {
        box->pixels += entries[i].count;
        for (unsigned c = 0; c < 3; c++) {
            unsigned v = cutChannel(entries[i], c);
            if (v < box->low[c]) {
                box->low[c] = v;
            }
            if (v > box->high[c]) {
                box->high[c] = v;
            }
        }
    }
    box->longest = 0;
    for (unsigned c = 1; c < 3; c++) {
        if (box->high[c] - box->low[c] > box->high[box->longest] - box->low[box->longest]) {
            box->longest = c;
        }
    }
    unsigned side = box->high[box->longest] - box->low[box->longest];
    uint64_t score = box->pixels * side;
    box->score = side == 0 ? 0 : (score > 0xFFFFFFFFu ? 0xFFFFFFFFu : (unsigned)score);
}

static void
cutBox(CutEntry *entries, CutBox *box, CutBox *upper) {
    unsigned c = box->longest;
    uint64_t perValue[32] = {};
    for (unsigned i = box->first; i < box->end; i++) {
        perValue[cutChannel(entries[i], c)] += entries[i].count;
    }
    unsigned split = box->low[c];
    uint64_t seen = 0;
    while (split < box->high[c] - 1 && (seen += perValue[split]) * 2 < box->pixels) {
        split++;
    }
    unsigned i = box->first;
    unsigned j = box->end;
    while (i < j) {
        if (cutChannel(entries[i], c) <= split) {
            i++;
        } else {
            j--;
            CutEntry tmp = entries[i];
            entries[i] = entries[j];
            entries[j] = tmp;
        }
    }
    upper->first = i;
    upper->end = box->end;
    box->end = i;
    measureBox(entries, box);
    measureBox(entries, upper);
}

bool
Dithering::optimizePalette(const uint8_t *rgb, unsigned width, unsigned height, unsigned colors, Palette *out) {
    if (colors == 0 || colors > MAX_COLORS) {
        return false;
    }
    const unsigned KEYS = 32 * 32 * 32;
    uint32_t *histogram = (uint32_t *)malloc(KEYS * sizeof(uint32_t));
    CutEntry *entries = (CutEntry *)malloc(KEYS * sizeof(CutEntry));
    if (!histogram || !entries) {
        if (histogram) {
            free(histogram);
        }
        if (entries) {
            free(entries);
        }
        return false;
    }
    memset(histogram, 0, KEYS * sizeof(uint32_t));
    for (unsigned i = 0; i < width * height; i++) {
        histogram[((unsigned)(rgb[i * 3] >> 3) << 10) | ((unsigned)(rgb[i * 3 + 1] >> 3) << 5) | (unsigned)(rgb[i * 3 + 2] >> 3)]++;
    }
    unsigned distinct = 0;
    for (unsigned key = 0; key < KEYS; key++) {
        if (histogram[key]) {
            entries[distinct++] = CutEntry{(uint16_t)key, histogram[key]};
        }
    }
    free(histogram);

    CutBox boxes[MAX_COLORS];
    unsigned boxCount = 0;
    if (distinct) {
        boxes[0].first = 0;
        boxes[0].end = distinct;
        measureBox(entries, &boxes[0]);
        boxCount = 1;
    }
    while (boxCount && boxCount < colors) {
        unsigned best = 0;
        for (unsigned b = 1; b < boxCount; b++) {
            if (boxes[b].score > boxes[best].score) {
                best = b;
            }
        }
        if (boxes[best].score == 0) {
            break;
        }
        cutBox(entries, &boxes[best], &boxes[boxCount]);
        boxCount++;
    }

    out->count = colors;
    for (unsigned b = 0; b < colors; b++) {
        int rgb6[3] = {0, 0, 0};
        if (b < boxCount) {
            uint64_t sum[3] = {0, 0, 0};
            for (unsigned i = boxes[b].first; i < boxes[b].end; i++) {
                for (unsigned c = 0; c < 3; c++) {
                    unsigned v5 = cutChannel(entries[i], c);
                    sum[c] += (uint64_t)((v5 << 3) | (v5 >> 2)) * entries[i].count;
                }
            }
            for (unsigned c = 0; c < 3; c++) {
                rgb6[c] = (int)((sum[c] / boxes[b].pixels) >> 2);
            }
        } else if (boxCount) {
            for (unsigned c = 0; c < 3; c++) {
                rgb6[c] = out->rgb[c];
            }
        }
        for (unsigned c = 0; c < 3; c++) {
            out->rgb[b * 3 + c] = rgb6[c];
        }
    }
    free(entries);
    return true;
}
