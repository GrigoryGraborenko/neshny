////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// based on code from https://github.com/seanchas116/qtimgui

#include "QtImGui.h"
#include "QtImGuiRenderer.h"
#include <QWindow>

namespace QtImGui {

class QWindowWindowWrapper : public WindowWrapper {
public:
    QWindowWindowWrapper(QWindow *w) : window(w) {}
    void installEventFilter(QObject *object) override {
        return window->installEventFilter(object);
    }
    QSize size() const override {
        return window->size();
    }
    qreal devicePixelRatio() const override {
        return window->devicePixelRatio();
    }
    bool isActive() const override {
        return window->isActive();
    }
    void setFocus(Qt::FocusReason focusReason) override {
        ((QWidget*)window)->setFocus(focusReason);
    }
    QPoint mapFromGlobal(const QPoint &p) const override {
        return window->mapFromGlobal(p);
    }
private:
    QWindow *window;
};

void initialize(QWindow *window, IEngine* engine) {
    ImGuiRenderer::instance()->initialize(new QWindowWindowWrapper(window), engine);
}

void newFrame() {
    ImGuiRenderer::instance()->newFrame();
}

void render() {
    ImGuiRenderer::instance()->render();
}

}
