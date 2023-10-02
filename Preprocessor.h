////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_PREPROCESS

namespace Neshny {

    QByteArray      Preprocess  ( QByteArray input, const std::function<QByteArray(QString, QString&)>& loader, QString& err_msg );

}