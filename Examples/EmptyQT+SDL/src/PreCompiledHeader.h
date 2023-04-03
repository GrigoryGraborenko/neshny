//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

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

#include <imgui\imgui_impl_sdl.h>
#include <imgui\imgui_impl_opengl3.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <Metastuff\Meta.h>

#include <IncludeAll.h> // Neshny
using namespace Neshny;

extern std::vector<std::string> g_ShaderBaseDirs;
struct EmbeddedFile { unsigned long long p_Size = 0; const unsigned char* p_Data = nullptr; };
extern std::unordered_map<std::string, EmbeddedFile> g_EmbeddedFiles;

#pragma msg("Compiling precompiled header...")