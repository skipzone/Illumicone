#include <iostream>
#include <vector>

#include "illumiconeTypes.h"
#include "ledscape.h"
#include "Widget.h"
#include "Pattern.h"

using namespace std;

Pattern::Pattern(int numStrings, int pixelsPerString, int numWidgets) :
    pixelsPerString(pixelsPerString),
    numStrings(numStrings),
    opcVector(numStrings, vector<ledscape_pixel_t>(pixelsPerString)),
    widgets()
{
    int i;
    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(2, 0, 0);
        widgets.emplace_back(3, 0, 0);
    }
}

int main(void)
{
    Pattern pattern(5, 5, 3);

    cout << "Pattern initialization!\n";
    cout << pattern.numStrings << "\n";
    cout << pattern.pixelsPerString << "\n";
    cout << pattern.opcVector.size() << "\n";

    for (auto& column: pattern.opcVector) {
        for (auto& row : column) {
            row.a = 5;
            row.b = 6;
            row.c = 7;
        }
    }

    for (auto column: pattern.opcVector) {
        for (auto row : column) {
            cout << "a: " << unsigned(row.a) << endl;
            cout << "b: " << unsigned(row.b) << endl;
            cout << "c: " << unsigned(row.c) << endl;
        }
    }
}
