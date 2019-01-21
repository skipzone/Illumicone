/*
    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <assert.h>
#include <iomanip>
#include <iostream>

#include "ConfigReader.h"
#include "MeasurementMapper.h"

using namespace std;


Log logger;                     // this is the global Log object used everywhere


void measurementMapperUnitTests()
{
    cout << "----- MeasurementMapper -----" << endl;

    MeasurementMapper<int, float> mapper0;
    assert(!mapper0.addRange(100, 0, 0, 10));
    cout << "    mapper0 passed." << endl;

    MeasurementMapper<int, float> mapper1;
    assert(mapper1.addRange(0, 100, 0, 10));

    float result1;

    assert(mapper1.mapMeasurement(0, result1));
    assert(result1 == 0);
    assert(mapper1.getLastRawMeasurement() == 0);
    assert(mapper1.getLastMappedMeasurement() == 0);

    assert(mapper1.mapMeasurement(50, result1));
    assert(result1 == 5);
    assert(mapper1.getLastRawMeasurement() == 50);
    assert(mapper1.getLastMappedMeasurement() == 5);

    assert(mapper1.mapMeasurement(99, result1));
    cout << "    1)  99 -> " << result1 << endl;
    assert(9.9 - result1 < 0.0001);
    assert(mapper1.getLastRawMeasurement() == 99);
    assert(9.9 - mapper1.getLastMappedMeasurement()  < 0.0001);

    assert(!mapper1.mapMeasurement(100, result1));

    assert(!mapper1.mapMeasurement(-1, result1));

    cout << "    mapper1 passed." << endl;

    MeasurementMapper<int, float> mapper2;
    assert(mapper2.addRange(0, 100, 10, 0));

    assert(mapper2.mapMeasurement(0));
    assert(mapper2.getLastRawMeasurement() == 0);
    assert(10.0 - mapper2.getLastMappedMeasurement() < 0.0001);

    assert(mapper2.mapMeasurement(49, result1));
    assert(mapper2.getLastRawMeasurement() == 49);
    assert(5.1 - mapper2.getLastMappedMeasurement() < 0.0001);

    assert(mapper2.mapMeasurement(50));
    assert(mapper2.getLastRawMeasurement() == 50);
    assert(5.0 - mapper2.getLastMappedMeasurement() < 0.0001);

    assert(mapper2.mapMeasurement(60));
    assert(mapper2.getLastRawMeasurement() == 60);
    cout << "    2)  60 -> " << mapper2.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper2.getLastMappedMeasurement() < 0.0001);

    assert(mapper2.mapMeasurement(99, result1));
    assert(mapper2.getLastRawMeasurement() == 99);
    assert(0.1 - mapper2.getLastMappedMeasurement()  < 0.0001);

    assert(!mapper2.mapMeasurement(100));
    assert(!mapper2.mapMeasurement(-1));

    cout << "    mapper2 passed." << endl;

    MeasurementMapper<int, float> mapper3;
    assert(mapper3.addRange(-9001, -3000, 0.1, 0.1));
    assert(mapper3.addRange(-3000,    -2, 0.1, 1.0));
    assert(mapper3.addRange(    1,  3000, 1.0, 4.0));
    assert(mapper3.addRange( 3000,  9001, 4.0, 4.0));

    assert(mapper3.mapMeasurement(-9001));
    cout << "    3)  -9001 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(0.1 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(-3001));
    cout << "    3)  -3001 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(0.1 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(-3000));
    assert(0.1 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(-3));
    cout << "    3)  -3 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(1.0 - mapper3.getLastMappedMeasurement() < 0.001);

    assert(mapper3.mapMeasurement(1));
    assert(1.0 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(2999));
    cout << "    3)  2999 -> " << mapper3.getLastMappedMeasurement() << ", diff = " << 4.0 - mapper3.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3.getLastMappedMeasurement() < 0.0011);

    assert(mapper3.mapMeasurement(3000));
    cout << "    3)  3000 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(mapper3.mapMeasurement(9000));
    cout << "    3)  9000 -> " << mapper3.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3.getLastMappedMeasurement() < 0.0001);

    assert(!mapper3.mapMeasurement(-32768));
    assert(!mapper3.mapMeasurement(-9002));
    assert(!mapper3.mapMeasurement(-2));
    assert(!mapper3.mapMeasurement(-1));
    assert(!mapper3.mapMeasurement(0));
    assert(!mapper3.mapMeasurement(9001));
    assert(!mapper3.mapMeasurement(32767));

    cout << "    mapper3 passed." << endl;


    ConfigReader config;
    assert(config.readConfigurationFile("../config/unitTests.json"));

    MeasurementMapper<int, float> mapper3config;
    assert(mapper3config.readConfig(config.getJsonObject(), "mapper3", "unitTests mapper3config"));

    assert(mapper3config.mapMeasurement(-9001));
    cout << "    3)  -9001 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(0.1 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(-3001));
    cout << "    3)  -3001 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(0.1 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(-3000));
    assert(0.1 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(-3));
    cout << "    3)  -3 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(1.0 - mapper3config.getLastMappedMeasurement() < 0.001);

    assert(mapper3config.mapMeasurement(1));
    assert(1.0 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(2999));
    cout << "    3)  2999 -> " << mapper3config.getLastMappedMeasurement() << ", diff = " << 4.0 - mapper3config.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3config.getLastMappedMeasurement() < 0.0011);

    assert(mapper3config.mapMeasurement(3000));
    cout << "    3)  3000 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(mapper3config.mapMeasurement(9000));
    cout << "    3)  9000 -> " << mapper3config.getLastMappedMeasurement() << endl;
    assert(4.0 - mapper3config.getLastMappedMeasurement() < 0.0001);

    assert(!mapper3config.mapMeasurement(-32768));
    assert(!mapper3config.mapMeasurement(-9002));
    assert(!mapper3config.mapMeasurement(-2));
    assert(!mapper3config.mapMeasurement(-1));
    assert(!mapper3config.mapMeasurement(0));
    assert(!mapper3config.mapMeasurement(9001));
    assert(!mapper3config.mapMeasurement(32767));

    cout << "    mapper3config passed." << endl;


    MeasurementMapper<int, float> mapper4(mapper3);

    assert(mapper4.mapMeasurement(-9001));
    assert(0.1 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(-3001));
    assert(0.1 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(-3000));
    assert(0.1 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(-3));
    assert(1.0 - mapper4.getLastMappedMeasurement() < 0.001);

    assert(mapper4.mapMeasurement(1));
    assert(1.0 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(2999));
    assert(4.0 - mapper4.getLastMappedMeasurement() < 0.0011);

    assert(mapper4.mapMeasurement(3000));
    assert(4.0 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(mapper4.mapMeasurement(9000));
    assert(4.0 - mapper4.getLastMappedMeasurement() < 0.0001);

    assert(!mapper4.mapMeasurement(-32768));
    assert(!mapper4.mapMeasurement(-9002));
    assert(!mapper4.mapMeasurement(-2));
    assert(!mapper4.mapMeasurement(-1));
    assert(!mapper4.mapMeasurement(0));
    assert(!mapper4.mapMeasurement(9001));
    assert(!mapper4.mapMeasurement(32767));

    cout << "    mapper4 passed." << endl;

    MeasurementMapper<int, float> mapper5;
    assert(mapper5.addRange(-2, 1, 999, 999));
    assert(mapper5.mapMeasurement(0));
    assert(999 - mapper5.getLastMappedMeasurement() < 0.0001);
    assert(!mapper5.mapMeasurement(-3));
    assert(!mapper5.mapMeasurement(1));
    mapper5 = mapper3;

    assert(mapper5.mapMeasurement(-9001));
    assert(0.1 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(-3001));
    assert(0.1 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(-3000));
    assert(0.1 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(-3));
    assert(1.0 - mapper5.getLastMappedMeasurement() < 0.001);

    assert(mapper5.mapMeasurement(1));
    assert(1.0 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(2999));
    assert(4.0 - mapper5.getLastMappedMeasurement() < 0.0011);

    assert(mapper5.mapMeasurement(3000));
    assert(4.0 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(mapper5.mapMeasurement(9000));
    assert(4.0 - mapper5.getLastMappedMeasurement() < 0.0001);

    assert(!mapper5.mapMeasurement(-32768));
    assert(!mapper5.mapMeasurement(-9002));
    assert(!mapper5.mapMeasurement(-2));
    assert(!mapper5.mapMeasurement(-1));
    assert(!mapper5.mapMeasurement(0));
    assert(!mapper5.mapMeasurement(9001));
    assert(!mapper5.mapMeasurement(32767));

    cout << "    mapper5 passed." << endl;

    cout << "    All MeasurementMapper tests passed." << endl;
}


int main(int argc, char **argv)
{
    cout << "Illumicone unit tests." << endl;

    logger.startLogging("unitTests", Log::LogTo::console);

    measurementMapperUnitTests();

    logger.stopLogging();

    cout << "All unit tests passed." << endl;
}

