#include "ColorRGB.h"

ColorRGB::ColorRGB()
    :r(0), g(0), b(0) {}

ColorRGB::ColorRGB(unsigned char red, unsigned char green, unsigned char blue)
    :r(red), g(green), b(blue) {}

ColorRGB::ColorRGB(const ColorRGB &other) = default;

ColorRGB &ColorRGB::operator=(const ColorRGB &other) = default;

bool ColorRGB::operator==(const ColorRGB &other) const
{
    return (r == other.r && g == other.g && b == other.b);
}

bool ColorRGB::operator!=(const ColorRGB &other) const
{
     return (r != other.r || g != other.g || b != other.b);
}
