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

#pragma once

#include "Widget.h"
#include "WidgetChannel.h"


/*
 * These are the measurements that Baton (an MPU6050-type widget) sends:
 *     Channel  Value
 *     -------  -------------
 *        0     Yaw
 *        1     Pitch
 *        2     Roll
 *        3     GyroX
 *        4     GyroY
 *        5     GyroZ
 *        6     AccelX
 *        7     AccelY
 *        8     AccelZ
 *        9     LinearAccelX
 *       10     LinearAccelY
 *       11     LinearAccelZ
 *       12     Temperature
*/

class BatonWidget : public Widget
{
    public:
        BatonWidget();
        virtual ~BatonWidget() {};

        BatonWidget(const BatonWidget&) = delete;
        BatonWidget& operator =(const BatonWidget&) = delete;

        void updateChannelSimulatedMeasurements(unsigned int chIdx);

    private:

        bool simulatedPositionGoingDown;
};

