#ifndef RGB_H
#define RGB_H

//Simple data structure for RGB colors with range from 0 to 255
struct ColorRGB
{
    unsigned char r;
    unsigned char g;
    unsigned char b;

    ColorRGB();
    ColorRGB(unsigned char red, unsigned char green, unsigned char blue);
    ColorRGB(const ColorRGB &other);

    ColorRGB& operator=(const ColorRGB &other);
    bool operator==(const ColorRGB &other) const;
    bool operator!=(const ColorRGB &other) const;
};

#endif // RGB_H
