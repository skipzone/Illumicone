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

#include <string>

#include "ConfigReader.h"
#include "illumiconePixelUtility.h"
#include "illumiconeUtility.h"
#include "log.h"
#include "SimpleBlockIndicator.h"


using namespace std;


bool SimpleBlockIndicator::init(unsigned int numStrings, unsigned int pixelsPerString, const json11::Json& indicatorConfig)
{
    if (!IndicatorRegion::init(numStrings, pixelsPerString, indicatorConfig)) {
        return false;
    }

    string errMsgSuffix = " in indicator region configuration:  " + indicatorConfig.dump();

    if (!ConfigReader::getUnsignedIntValue(indicatorConfig, "fadeIntervalMs", fadeIntervalMs)) {
        fadeIntervalMs = 0;
    }

    if (!ConfigReader::getUnsignedIntValue(indicatorConfig, "flashIntervalMs", flashIntervalMs)) {
        flashIntervalMs = 0;
    }

    return true;
}


void SimpleBlockIndicator::makeAnimating(bool enable)
{
    if (enable) {
        if (flashIntervalMs == 0) {
            logMsg(LOG_ERR, "makeAnimating(true) called, but flashIntervalMs not specified in configuration for index "
                            + to_string(index) + ".");
            return;
        }
        if (!isAnimating) {
            state = AnimationState::flashStart;
        }
    }
    else {
        if (isAnimating) {
            state = AnimationState::inactive;
            if (isOn) {
                turnOnImmediately();
            }
            else {
                turnOffImmediately();
            }
        }
    }
    isAnimating = enable;
}


bool SimpleBlockIndicator::runAnimation()
{
    bool wantDisplay = true;

    unsigned int nowMs = getNowMs();

    float fadeValueTransition;

    switch (state) {

        case AnimationState::inactive:
            wantDisplay = false;
            break;

        case AnimationState::transitionOnStart:
        case AnimationState::transitionOffStart:
            nextFadeChangeMs = nowMs;
            fadeValueTransition = foregroundColor.v;
            if (fadeValueTransition > fadeIntervalMs) {
                fadeStepMs = 1;
                fadeStepValue = (float) fadeValueTransition / (float) fadeIntervalMs;
            }
            else {
                fadeStepMs = fadeIntervalMs / fadeValueTransition;
                fadeStepValue = (float) fadeValueTransition / (float) (fadeIntervalMs / fadeStepMs);
            }
            //logMsg(LOG_DEBUG, "fadeStepMs=" + to_string(fadeStepMs) + " fadeStepValue=" + to_string(fadeStepValue));
            if (state == AnimationState::transitionOnStart) {
                fadeValue = 0;
                state = AnimationState::transitionOn;
            }
            else {
                fadeValue = foregroundColor.v;
                state = AnimationState::transitionOff;
            }
            break;

        case AnimationState::transitionOn:
            if ((int) (nowMs - nextFadeChangeMs) >= 0) {
                nextFadeChangeMs += fadeStepMs;
                fadeValue += fadeStepValue;
                if (fadeValue < foregroundColor.v) {
                    HsvPixel color = foregroundColor;
                    color.v = fadeValue;
                    fillRegion(color);
                }
                else {
                    fillRegion(foregroundColor);
                    isTransitioning = false;
                    state = AnimationState::inactive;
                }
            }
            break;

        case AnimationState::transitionOff:
            if ((int) (nowMs - nextFadeChangeMs) >= 0) {
                nextFadeChangeMs += fadeStepMs;
                fadeValue -= fadeStepValue;
                if (fadeValue > 0) {
                    HsvPixel color = foregroundColor;
                    color.v = fadeValue;
                    fillRegion(color);
                }
                else {
                    fadeValue = 0;
                    fillRegion(backgroundColor);
                    isTransitioning = false;
                    state = AnimationState::inactive;
                }
            }
            break;

        case AnimationState::flashStart:
            nextFlashChangeMs = nowMs;
            state = isOn ? AnimationState::flashOff : AnimationState::flashOn;
            break;

        case AnimationState::flashOn:
            if ((int) (nowMs - nextFlashChangeMs) >= 0) {
                //logMsg(LOG_DEBUG, "turning flash on");
                fillRegion(foregroundColor);
                nextFlashChangeMs += flashIntervalMs / 2;
                state = AnimationState::flashOnWait;
            }
            break;

        case AnimationState::flashOnWait:
            if ((int) (nowMs - nextFlashChangeMs) >= 0) {
                state = AnimationState::flashOff;
            }
            break;

        case AnimationState::flashOff:
            if ((int) (nowMs - nextFlashChangeMs) >= 0) {
                //logMsg(LOG_DEBUG, "turning flash off");
                HsvPixel color = foregroundColor;
                color.v = 0;
                fillRegion(color);
                nextFlashChangeMs += flashIntervalMs / 2;
                state = AnimationState::flashOffWait;
            }
            break;

        case AnimationState::flashOffWait:
            if ((int) (nowMs - nextFlashChangeMs) >= 0) {
                state = AnimationState::flashOn;
            }
            break;
    }

    return wantDisplay;
}


void SimpleBlockIndicator::transitionOff()
{
    isOn = false;
    isAnimating = false;
    isTransitioning = true;
    state = AnimationState::transitionOffStart;
}


void SimpleBlockIndicator::transitionOn()
{
    if (fadeIntervalMs == 0) {
        logMsg(LOG_ERR, "makeAnimating(true) called, but fadeIntervalMs not specified in configuration for index "
                        + to_string(index) + ".");
        return;
    }
    isOn = true;
    isAnimating = false;
    isTransitioning = true;
    state = AnimationState::transitionOnStart;
}


void SimpleBlockIndicator::turnOffImmediately()
{
    isOn = false;
    isAnimating = false;
    isTransitioning = false;
    state = AnimationState::inactive;

    fillRegion(backgroundColor);
}


void SimpleBlockIndicator::turnOnImmediately()
{
    isOn = true;
    isAnimating = false;
    isTransitioning = false;
    state = AnimationState::inactive;

    fillRegion(foregroundColor);
}

