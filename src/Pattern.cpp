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
    RgbVerticalPattern rgbPattern2;
    int pattern[2] = {2, 3};
    int pattern2[3] = {3, 2, 1};

    cout << "Pattern initialization!\n";

    rgbPattern.initPattern(2, pattern);
    rgbPattern2.initPattern(3, pattern2);

    cout << "rgbPattern sizes: " << rgbPattern.widgets.size() << " " << rgbPattern.widgets[1].channels.size() << endl;

    cout << "rgbPattern2 sizes: " << rgbPattern2.widgets.size() << " " << rgbPattern2.widgets[2].channels.size() << endl;
}
