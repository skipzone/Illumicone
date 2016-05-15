#ifndef PATTERN_H
#define PATTERN_H

class Pattern
{
    public:
        Pattern(int, int, int);
        int pixelsPerString;
        int numStrings;
        std::vector<std::vector<ledscape_pixel_t>> opcVector;
        std::vector<Widget> widgets;
};

#endif /* PATTERN_H */
