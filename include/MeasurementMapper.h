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
        MeasurementMapper(const MeasurementMapper& rhs);
        virtual ~MeasurementMapper();

        MeasurementMapper& operator =(const MeasurementMapper& rhs);

        bool addRange(T_raw rawMin, T_raw rawMax, T_mapped mappedStart, T_mapped mappedEnd);  // [rawMin, rawMax)
        T_raw getLastRawMeasurement();
        T_mapped getLastMappedMeasurement();
        bool mapMeasurement(T_raw rawMeasurement);
        bool mapMeasurement(T_raw rawMeasurement, T_mapped& mappedMeasurement);

    private:

        enum class MappedIntervalType {
            normal,
            reverse,
            constant
        };

        struct MeasurementRange
        {
            T_raw rawMin;
            T_raw rawMax;
            T_raw rawInterval;
            T_mapped mappedMin;
            T_mapped mappedMax;
            T_mapped mappedInterval;
            MappedIntervalType mappedIntervalType;
        };

        std::vector<MeasurementRange*> measurementRanges;

        T_raw lastRawMeasurement;
        T_mapped lastMappedMeasurement;
};


template <class T_raw, class T_mapped>
MeasurementMapper<T_raw, T_mapped>::MeasurementMapper(const MeasurementMapper<T_raw, T_mapped>& rhs)
{
    this->operator=(rhs);
}


template <class T_raw, class T_mapped>
MeasurementMapper<T_raw, T_mapped>::~MeasurementMapper()
{
    for (auto&& range : measurementRanges) {
        delete range;
    }
}


template <class T_raw, class T_mapped>
MeasurementMapper<T_raw, T_mapped>& MeasurementMapper<T_raw, T_mapped>::operator =(const MeasurementMapper<T_raw, T_mapped>& rhs)
{
    if (&rhs == this) {
        return *this;
    }

    for (auto&& range : measurementRanges) {
        delete range;
    }
    measurementRanges.clear();

    for (auto&& range : rhs.measurementRanges) {
        auto newRange = new MeasurementRange;
        measurementRanges.emplace_back(newRange);

        newRange->rawMin = range->rawMin;
        newRange->rawMax = range->rawMax;
        newRange->rawInterval = range->rawInterval;
        newRange->mappedMin = range->mappedMin;
        newRange->mappedMax = range->mappedMax;
        newRange->mappedInterval = range->mappedInterval;
        newRange->mappedIntervalType = range->mappedIntervalType;
    }

    this->lastRawMeasurement = rhs.lastRawMeasurement;
    this->lastMappedMeasurement = rhs.lastMappedMeasurement;

    return *this;
}


template <class T_raw, class T_mapped>
bool MeasurementMapper<T_raw, T_mapped>::addRange(T_raw rawMin, T_raw rawMax, T_mapped mappedStart, T_mapped mappedEnd)
{
    if (rawMax < rawMin) {
        return false;
    }

    auto newRange = new MeasurementRange;
    measurementRanges.emplace_back(newRange);

    newRange->rawMin = rawMin;
    newRange->rawMax = rawMax;
    newRange->rawInterval = rawMax - rawMin;

    if (mappedStart <= mappedEnd) {
        newRange->mappedMin = mappedStart;
        newRange->mappedMax = mappedEnd;
        newRange->mappedIntervalType = (mappedStart < mappedEnd)
            ? MappedIntervalType::normal
            : MappedIntervalType::constant;
    }
    else {
        newRange->mappedMin = mappedEnd;
        newRange->mappedMax = mappedStart;
        newRange->mappedIntervalType = MappedIntervalType::reverse;
    }
    newRange->mappedInterval = newRange->mappedMax - newRange->mappedMin;

    return true;
}


template <class T_raw, class T_mapped>
T_raw MeasurementMapper<T_raw, T_mapped>::getLastRawMeasurement()
{
    return lastRawMeasurement;
}


template <class T_raw, class T_mapped>
T_mapped MeasurementMapper<T_raw, T_mapped>::getLastMappedMeasurement()
{
    return lastMappedMeasurement;
}


template <class T_raw, class T_mapped>
bool MeasurementMapper<T_raw, T_mapped>::mapMeasurement(T_raw rawMeasurement)
{
    bool measurementMapped = false;
    for (auto&& range : measurementRanges) {
        if (rawMeasurement >= range->rawMin && rawMeasurement < range->rawMax) {

            measurementMapped = true;
            lastRawMeasurement = rawMeasurement;

            if (range->mappedIntervalType == MappedIntervalType::constant) {
                lastMappedMeasurement = range->mappedMin;
            }
            else {
                T_mapped mappedOffset = 
                    range->mappedInterval * ((T_mapped) rawMeasurement - (T_mapped) range->rawMin) / (T_mapped) range->rawInterval;
                if (range->mappedIntervalType == MappedIntervalType::normal) {
                    lastMappedMeasurement = range->mappedMin + mappedOffset;
                }
                else {
                    lastMappedMeasurement = range->mappedMax - mappedOffset;
                }
            }
        }
    }

    return measurementMapped;
}


template <class T_raw, class T_mapped>
bool MeasurementMapper<T_raw, T_mapped>::mapMeasurement(T_raw rawMeasurement, T_mapped& mappedMeasurement)
{
    bool measurementMapped = mapMeasurement(rawMeasurement);
    if (measurementMapped) {
        mappedMeasurement = lastMappedMeasurement;
    }
    return measurementMapped;
}

