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
#include "WidgetId.h"
#include "WidgetChannel.h"


/*
 * These are the measurements that Mike (an MSGEQ7-type widget) sends:
 *     Channel  Sound Intensity Band
 *              Center Freq. (Hz)
 *     -------  --------------------
 *        0     63 
 *        1     160
 *        2     400
 *        3     1000
 *        4     2500
 *        5     6250
 *        6     16000
*/

class MikeWidget : public Widget
{
    public:
        MikeWidget(WidgetId id);
        virtual ~MikeWidget() {};

        MikeWidget(const MikeWidget&) = delete;
        MikeWidget& operator =(const MikeWidget&) = delete;

    private:

};

