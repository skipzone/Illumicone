#include <iostream>
#include <vector>

#include "illumiconeTypes.h"
#include "ledscape.h"

using namespace std;

class Pattern {
    public:
        Pattern(int, int);
        int pixelsPerString, numStrings;
        vector<ledscape_pixel_t> OPCVector;
};

Pattern::Pattern(int numStrings, int pixelsPerString) : OPCVector(numStrings * pixelsPerString) {
    this->pixelsPerString = pixelsPerString;
    this->numStrings = numStrings;
}

int main(void)
{
    Pattern pattern(5,5);

    cout << "Pattern initialization!\n";
    cout << pattern.numStrings << "\n";
    cout << pattern.pixelsPerString << "\n";
    cout << pattern.OPCVector.size() << "\n";

    for (auto& i: pattern.OPCVector) {
        i.a = 5;
        i.b = 6;
        i.c = 7;
    }

    for (auto i: pattern.OPCVector) {
        cout << "a: " << unsigned(i.a) << endl;
        cout << "b: " << unsigned(i.b) << endl;
        cout << "c: " << unsigned(i.c) << endl;
    }
}
