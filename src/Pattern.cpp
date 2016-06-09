#include <iostream>
#include <vector>

#include "illumiconeTypes.h"
#include "ledscape.h"
#include "Widget.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"

using namespace std;

int main(void)
{
    RgbVerticalPattern rgbPattern;
    int pattern[2] = {5, 3};

    cout << "Pattern initialization!\n";

    rgbPattern.initPattern(NUM_STRINGS, PIXELS_PER_STRING);
    rgbPattern.initWidgets(2, pattern[0]);
    rgbPattern.update();

    cout << "rgbPattern pixel array size X: " << rgbPattern.pixelArray.size() << endl;
    cout << "rgbPattern pixel array size Y: " << rgbPattern.pixelArray[0].size() << endl;
    cout << "rgbPattern widgets size: " << rgbPattern.widgets.size() << endl;
}
