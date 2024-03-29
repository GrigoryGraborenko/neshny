////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// based on code from https://github.com/seanchas116/qtimgui
#pragma once

class QWidget;
class QWindow;

namespace Neshny {

void QTInitialize(QWindow *window, IEngine* engine);
void QTNewFrame();
void QTRender();

} // namespace Neshny