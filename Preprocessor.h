////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#define NESHNY_PREPROCESS

namespace Neshny {

    std::string      Preprocess  ( std::string_view input, const std::function<QByteArray(std::string_view, std::string&)>& loader, std::string& err_msg );

}