////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// based on code from https://github.com/seanchas116/qtimgui

#include "QtImGuiRenderer.h"
#include <QDateTime>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QClipboard>
#include <QCursor>
#include <QDebug>

namespace QtImGui {

QHash<int, ImGuiKey> keyMap = {
    { Qt::Key_Tab, ImGuiKey_Tab },
    { Qt::Key_Left, ImGuiKey_LeftArrow },
    { Qt::Key_Right, ImGuiKey_RightArrow },
    { Qt::Key_Up, ImGuiKey_UpArrow },
    { Qt::Key_Down, ImGuiKey_DownArrow },
    { Qt::Key_PageUp, ImGuiKey_PageUp },
    { Qt::Key_PageDown, ImGuiKey_PageDown },
    { Qt::Key_Home, ImGuiKey_Home },
    { Qt::Key_End, ImGuiKey_End },
    { Qt::Key_Delete, ImGuiKey_Delete },
    { Qt::Key_Backspace, ImGuiKey_Backspace },
    { Qt::Key_Enter, ImGuiKey_Enter },
    { Qt::Key_Return, ImGuiKey_Enter },
    { Qt::Key_Escape, ImGuiKey_Escape },
    { Qt::Key_Space, ImGuiKey_Space },
    { Qt::Key_A, ImGuiKey_A },
    { Qt::Key_C, ImGuiKey_C },
    { Qt::Key_V, ImGuiKey_V },
    { Qt::Key_X, ImGuiKey_X },
    { Qt::Key_Y, ImGuiKey_Y },
    { Qt::Key_Z, ImGuiKey_Z },
    { Qt::Key_0, ImGuiKey_0 },
    { Qt::Key_1, ImGuiKey_1 },
    { Qt::Key_2, ImGuiKey_2 },
    { Qt::Key_3, ImGuiKey_3 },
    { Qt::Key_4, ImGuiKey_4 },
    { Qt::Key_5, ImGuiKey_5 },
    { Qt::Key_6, ImGuiKey_6 },
    { Qt::Key_7, ImGuiKey_7 },
    { Qt::Key_8, ImGuiKey_8 },
    { Qt::Key_9, ImGuiKey_9 }
};

QByteArray g_currentClipboardText;

void ImGuiRenderer::initialize(WindowWrapper *window) {
    m_Window.reset(window);

    ImGuiIO &io = ImGui::GetIO();

    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;       // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "imgui_impl_qt";

    for (ImGuiKey key : keyMap.values()) {
        //io.KeyMap[key] = key;
    }

    io.SetClipboardTextFn = [](void *user_data, const char *text) {
        Q_UNUSED(user_data);
        QGuiApplication::clipboard()->setText(text);
    };
    io.GetClipboardTextFn = [](void *user_data) {
        Q_UNUSED(user_data);
        g_currentClipboardText = QGuiApplication::clipboard()->text().toUtf8();
        return (const char *)g_currentClipboardText.data();
    };

    window->installEventFilter(this);

    g_ElapsedTimer.start();
}

void ImGuiRenderer::renderDrawList(ImDrawData *draw_data) {
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, &last_blend_src_rgb);
    GLint last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, &last_blend_dst_rgb);
    GLint last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_alpha);
    GLint last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_alpha);
    GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    const float ortho_projection[4][4] =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    glUseProgram(g_ShaderHandle);
    glUniform1i(g_AttribLocationTex, 0);
    glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(g_VaoHandle);

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glActiveTexture(last_active_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

bool ImGuiRenderer::createFontsTexture() {
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

    if (g_FontTexture) {
        glDeleteTextures(1, &g_FontTexture);
    }

    // Upload texture to graphics system
    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;
    g_FontsDirty = false;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);

    return true;
}

bool ImGuiRenderer::createDeviceObjects() {
    // Backup GL state
    GLint last_texture, last_array_buffer, last_vertex_array;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    const GLchar *vertex_shader =
        "#version 330\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "   Frag_UV = UV;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader =
        "#version 330\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "   Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
        "}\n";

    g_ShaderHandle = glCreateProgram();
    const GLuint vertHandle = glCreateShader(GL_VERTEX_SHADER);
    const GLuint fragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertHandle, 1, &vertex_shader, 0);
    glShaderSource(fragHandle, 1, &fragment_shader, 0);
    glCompileShader(vertHandle);
    glCompileShader(fragHandle);
    glAttachShader(g_ShaderHandle, vertHandle);
    glAttachShader(g_ShaderHandle, fragHandle);
    glLinkProgram(g_ShaderHandle);

    // now that our program is linked, we can delete our shaders
    glDetachShader(g_ShaderHandle, vertHandle);
    glDetachShader(g_ShaderHandle, fragHandle);
    glDeleteShader(vertHandle);
    glDeleteShader(fragHandle);

    g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
    g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
    g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
    g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
    g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

    glGenBuffers(1, &g_VboHandle);
    glGenBuffers(1, &g_ElementsHandle);

    glGenVertexArrays(1, &g_VaoHandle);
    glBindVertexArray(g_VaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    glEnableVertexAttribArray(g_AttribLocationPosition);
    glEnableVertexAttribArray(g_AttribLocationUV);
    glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

    g_Initialised = true;

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);

    return true;
}

void ImGuiRenderer::newFrame() {
    if (!g_Initialised) {
        createDeviceObjects();
    }
    if (g_FontsDirty) {
        createFontsTexture();
    }

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2(m_Window->size().width(), m_Window->size().height());
    io.DisplayFramebufferScale = ImVec2(m_Window->devicePixelRatio(), m_Window->devicePixelRatio());

    // Setup time step
    io.DeltaTime = (float)(g_ElapsedTimer.restart() / 1000.0F);

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    auto pos = m_Window->mapFromGlobal(QCursor::pos());
    io.MousePos = ImVec2(pos.x(), pos.y());   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)

    for (int i = 0; i < 3; i++) {
        if (g_MousePressed[i].size() > 1) { // previous frame's mouse state can be discarded - always keep one state around so you can assume previous frame's state still holds
            g_MousePressed[i].pop_front();
        }
        io.MouseDown[i] = g_MousePressed[i].front();
    }

    io.MouseWheelH = g_MouseWheelH;
    io.MouseWheel = g_MouseWheel;
    g_MouseWheelH = 0;
    g_MouseWheel = 0;

    if (g_TabKeyPressed) {
        // handle tab key release events and make sure we still focus on the same View.
        io.KeysDown[keyMap[Qt::Key_Tab]] = false;
        m_Window->setFocus(Qt::FocusReason::TabFocusReason);
        g_TabKeyPressed = false;
    }

    for (auto event : g_KeyEvents) {
        if (keyMap.contains(event.key)) {
            io.KeysDown[keyMap[event.key]] = event.type == QEvent::KeyPress;
            g_TabKeyPressed = (event.key == Qt::Key_Tab);
        }
        if (event.type == QEvent::KeyPress && event.inputCharacter != 0) {
            io.AddInputCharacter(event.inputCharacter);
        }
    }

    Qt::KeyboardModifiers modifier = QGuiApplication::queryKeyboardModifiers();
#ifdef Q_OS_MAC
    io.KeyCtrl = modifier & Qt::MetaModifier;
    io.KeyShift = modifier & Qt::ShiftModifier;
    io.KeyAlt = modifier & Qt::AltModifier;
    io.KeySuper = modifier & Qt::ControlModifier;
#else
    io.KeyCtrl = modifier & Qt::ControlModifier;
    io.KeyShift = modifier & Qt::ShiftModifier;
    io.KeyAlt = modifier & Qt::AltModifier;
    io.KeySuper = modifier & Qt::MetaModifier;
#endif

    g_KeyEvents.clear();

    // Hide OS mouse cursor if ImGui is drawing it
    // glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}

void ImGuiRenderer::render() {
    ImGui::Render();
    renderDrawList(ImGui::GetDrawData());
}

void ImGuiRenderer::onMousePressedChange(QMouseEvent *event) {
    bool currentButtons[3] = {
        (bool)(event->buttons() & Qt::LeftButton),
        (bool)(event->buttons() & Qt::RightButton),
        (bool)(event->buttons() & Qt::MiddleButton)
    };
    for (int i = 0; i < 3; i++) {
        g_MousePressed[i].push_back(currentButtons[i]);
    }
}

void ImGuiRenderer::onWheel(QWheelEvent *event) {
    // Handle horizontal component
    if (event->angleDelta().x() == 0.0) {
        g_MouseWheelH += event->pixelDelta().x() / (ImGui::GetTextLineHeight());
    } else {
        // Magic number of 120 comes from Qt doc on QWheelEvent::pixelDelta()
        g_MouseWheelH += event->angleDelta().x() / 120;
    }

    // Handle vertical component
    if (event->angleDelta().y() == 0.0) {
        // 5 lines per unit
        g_MouseWheel += event->pixelDelta().y() / (5.0 * ImGui::GetTextLineHeight());
    } else {
        // Magic number of 120 comes from Qt doc on QWheelEvent::pixelDelta()
        g_MouseWheel += event->angleDelta().y() / 120;
    }
}

void ImGuiRenderer::onKeyPressRelease(QKeyEvent *event) {
    ushort character = 0;
    if (event->type() == QEvent::KeyPress) {
        QString text = event->text();
        if (text.size() == 1) {
            character = text.at(0).unicode();
        }
    } else if (event->key() == Qt::Key_Tab) { // event->type() == QEvent::KeyRelease
        // When we have multiple ImGuiRenderers (Views) active, tab can make current view out of focus.
        // Because tab key press happens in one View and released in another View.
        // So we ignore tab key release events here and manually inject tab key release event later.
        return;
    }

    g_KeyEvents.push_back({event->type(), event->key(), character, event->modifiers()});
}

bool ImGuiRenderer::eventFilter(QObject *watched, QEvent *event) {
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick: // MouseButtonDblClick *replaces* MouseButtonPress if you click too rapidly, so treat it like a MouseButtonPress
        this->onMousePressedChange(static_cast<QMouseEvent *>(event));
        break;
    case QEvent::Wheel:
        this->onWheel(static_cast<QWheelEvent *>(event));
        break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        this->onKeyPressRelease(static_cast<QKeyEvent *>(event));
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

ImGuiRenderer* ImGuiRenderer::instance(ImGuiRenderer* replace_instance, bool create) {
    static ImGuiRenderer* instance = nullptr;
    if (replace_instance) {
        instance = replace_instance;
    } else if (create || (!instance)) {
        instance = new ImGuiRenderer();
    }
    return instance;
}

}
