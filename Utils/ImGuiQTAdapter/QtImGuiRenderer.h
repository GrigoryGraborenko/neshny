////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// based on code from https://github.com/seanchas116/qtimgui

#pragma once

#include <QObject>
#include <QPoint>
#include <QEvent>
#include <QElapsedTimer>
#include <imgui\imgui.h>
#include <memory>
#include <deque>

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

namespace QtImGui {

struct KeyEvent {
    QEvent::Type            type;
    int                     key;
    ushort                  inputCharacter;
    Qt::KeyboardModifiers   modifiers;
};

class WindowWrapper {
public:
    virtual ~WindowWrapper() {}
    virtual void installEventFilter(QObject *object) = 0;
    virtual QSize size() const = 0;
    virtual qreal devicePixelRatio() const = 0;
    virtual bool isActive() const = 0;
    virtual void setFocus(Qt::FocusReason focusReason) = 0;
    virtual QPoint mapFromGlobal(const QPoint &p) const = 0;
};

class ImGuiRenderer : public QObject {
public:
    void initialize(WindowWrapper *window, IEngine* engine);
    void newFrame();
    void render();

    bool eventFilter(QObject *watched, QEvent *event);

    static ImGuiRenderer *instance(ImGuiRenderer* replace_instance = nullptr, bool create = false);
    void markFontsDirty() { g_FontsDirty = true; }
    ~ImGuiRenderer() {
        if (g_Initialised) {
            glDeleteBuffers(1, &g_VaoHandle);
            glDeleteBuffers(1, &g_VboHandle);
            glDeleteBuffers(1, &g_ElementsHandle);
            glDeleteProgram(g_ShaderHandle);
        }
    }

private:
    ImGuiRenderer() {}

    void onMousePressedChange(QMouseEvent *event);
    void onWheel(QWheelEvent *event);
    void onKeyPressRelease(QKeyEvent *event);

    void renderDrawList(ImDrawData *draw_data);
    bool createFontsTexture();
    bool createDeviceObjects();

    std::unique_ptr<WindowWrapper> m_Window;
    IEngine*       m_Engine = nullptr;
    QElapsedTimer g_ElapsedTimer;
    std::deque<bool> g_MousePressed[3] = { { false }, { false }, { false } };
    float        g_MouseWheel = 0;
    float        g_MouseWheelH = 0;
    GLuint       g_FontTexture = 0;
    int          g_ShaderHandle = 0;
    int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
    int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
    unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;
    std::vector<KeyEvent> g_KeyEvents;
    bool         g_Initialised = false;
    bool         g_FontsDirty = true;
};

}
