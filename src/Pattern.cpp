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
    int pattern[2] = {2, 3};

    cout << "Pattern initialization!\n";

    rgbPattern.initPattern(2, pattern[0]);

    cout << "rgbPattern sizes: " << rgbPattern.widgets.size() << " ";
}
