#ifndef EYE_WIDGET_H
#define EYE_WIDGET_H

#include "Widget.h"

class EyeWidget : public Widget
{
    public:
        EyeWidget() {};
        ~EyeWidget() {};

        EyeWidget(const EyeWidget&) = delete;
        EyeWidget& operator = (const EyeWidget&) = delete;
        bool moveData();
};

#endif /* EYE_WIDGET_H */
