//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#define QT_LOOP
#define IMGUI_DISABLE_OBSOLETE_KEYIO

#include <QCoreApplication>
#include <QString>
#include <QFile>
#include <QDir>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QElapsedTimer>
#include <QImage>
#include <QtDebug>
#include <QDateTime>
#include <QCryptographicHash>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <imgui\imgui_impl_opengl3.h>
#include <Metastuff\Meta.h>

#include <IncludeAll.h> // Neshny

#include <QApplication>
#include <QMainWindow>
#include <QOpenGLWindow>

#include <Utils\ImGuiQTAdapter\QtImGui.h>
#include <Utils\ImGuiQTAdapter\QtImGuiRenderer.h>

#pragma msg("Compiling precompiled header...")