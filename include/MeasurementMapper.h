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

#include <vector>


template <class T_raw, class T_mapped>
class MeasurementMapper
{
    public:

        MeasurementMapper() {}
        virtual ~MeasurementMapper() {}

        MeasurementMapper(const MeasurementMapper&) = delete;
        MeasurementMapper& operator =(const MeasurementMapper&) = delete;

        void addRange(T_raw rawMin, T_raw rawMax, T_mapped mappedMin, T_mapped mappedMax);
        void ignoreRange(T_raw rawMin, T_raw rawMax);
        
    private:

        struct MeasurementRange
        {
            T_raw rawMin;
            T_raw rawMax;
            T_mapped mappedMin;
            T_mapped mappedMax;
            bool ignore;
        };

        std::vector<MeasurementRange*> measurementRanges;

        T_raw lastRawMeasurement;
        T_mapped lastMappedMeasurement;
};


template <class T_raw, class T_mapped>
void MeasurementMapper<T_raw, T_mapped>::addRange(T_raw rawMin, T_raw rawMax, T_mapped mappedMin, T_mapped mappedMax)
{
    auto newRange = new MeasurementRange;
    newRange->rawMin = rawMin;
    newRange->rawMax = rawMax;
    newRange->mappedMin = mappedMin;
    newRange->mappedMax = mappedMax;
    newRange->ignore = false;

    measurementRanges.emplace_back(newRange);
}


template <class T_raw, class T_mapped>
void MeasurementMapper<T_raw, T_mapped>::ignoreRange(T_raw rawMin, T_raw rawMax)
{
    auto newRange = new MeasurementRange;
    newRange->rawMin = rawMin;
    newRange->rawMax = rawMax;
    newRange->mappedMin = 0;
    newRange->mappedMax = 0;
    newRange->ignore = true;

    measurementRanges.emplace_back(newRange);
}


