////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <nlohmann/json.hpp>

namespace meta {
template <typename T>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional< std::optional<T> > : std::true_type {};
}

namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
// JSON Serialization/Deserialization
////////////////////////////////////////////////////////////////////////////////
namespace Json {

////////////////////////////////////////////////////////////////////////////////
class ParseError {
public:
    explicit ParseError() = default;
    inline void AddMessage(QString errStr) { m_ErrorList.prepend(errStr); }
    inline QString Error() const { return m_ErrorList.join(", "); }
    inline operator bool() const { return !m_ErrorList.empty(); }
private:
    QStringList m_ErrorList = QStringList();
};

////////////////////////////////////////////////////////////////////////////////
// Serialization
////////////////////////////////////////////////////////////////////////////////

template <typename T, typename = std::enable_if_t <meta::isRegistered<T>()>>
void Serialise(const T& obj, nlohmann::json &jVal, Json::ParseError &err);

template <typename T, typename = std::enable_if_t <!meta::isRegistered<T>()>, typename = void>
void Serialise(const T& obj, nlohmann::json &jVal, Json::ParseError &err);

////////////////////////////////////////////////////////////////////////////////
template <typename Class>
inline void SerialiseByType(const Class& obj, nlohmann::json &json, [[maybe_unused]] ParseError &err) {
    if constexpr (std::is_same<Class, bool>::value) {
        json = static_cast<bool>(obj);
    } else if constexpr (std::is_same<Class, float>::value) {
        json = static_cast<double>(obj);
    } else if constexpr (std::is_same<Class, double>::value) {
        json = static_cast<double>(obj);
    } else if constexpr (std::is_unsigned<Class>::value) {
        json = static_cast<qint64>(obj);
    } else if constexpr (std::is_arithmetic<Class>::value) {
        json = static_cast<qint64>(obj);
    } else if constexpr (std::is_enum<Class>::value) {
        json = static_cast<int>(obj);
    } else if constexpr (std::is_same<Class, QString>::value) {
        json = obj.toStdString();
    } else {
        json = obj;
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void SerialiseByType(const nlohmann::json& val, nlohmann::json& json, [[maybe_unused]] ParseError &err) {
    json = val;
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void SerialiseByType(const std::string& val, nlohmann::json& json, [[maybe_unused]] ParseError &err) {
    json = val;
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const BaseVec2<T>& val, nlohmann::json& json, [[maybe_unused]] ParseError& err) {
    json = nlohmann::json{ {"x", val.x}, {"y", val.y} };
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const BaseVec3<T>& val, nlohmann::json& json, [[maybe_unused]] ParseError& err) {
    json = nlohmann::json{ {"x", val.x}, {"y", val.y}, {"z", val.z} };
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const BaseVec4<T>& val, nlohmann::json& json, [[maybe_unused]] ParseError& err) {
    json = nlohmann::json{ {"x", val.x}, {"y", val.y}, {"z", val.z}, {"w", val.w} };
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
class SerialiserFunc {
public:
    SerialiserFunc(const T& obj, nlohmann::json& json, ParseError &err) : m_ClassObj(obj), m_JsonObject(json), m_Error(err) {}

    template<typename Member>
    void operator()(Member& member) {
        if (m_Error) {
            return;
        }
        if constexpr (meta::is_optional<meta::get_member_type<Member>>::value) {
            if (member.canGetConstRef()) {
                auto& optionalVal = member.get(m_ClassObj);
                if(optionalVal) {
                    nlohmann::json serialisedValue;
                    Json::Serialise(*member.get(m_ClassObj), serialisedValue, m_Error);
                    if (m_Error) {
                        return m_Error.AddMessage(QString("parsing member %1 failed").arg(member.getName()));
                    }
                    m_JsonObject[member.getName()] = serialisedValue;
                }
            }
        } else {
            nlohmann::json serialisedValue;
            if (member.canGetConstRef()) {
                Json::Serialise(member.get(m_ClassObj), serialisedValue, m_Error);
                if (m_Error) {
                    return m_Error.AddMessage(QString("parsing member %1 failed").arg(member.getName()));
                }
            } else if (member.hasGetter()) {
                Json::Serialise(member.getCopy(m_ClassObj), serialisedValue, m_Error); // passing copy as const ref
                if (m_Error) {
                    return m_Error.AddMessage(QString("parsing member %1 failed").arg(member.getName()));
                }
            } else {
                return m_Error.AddMessage(QString("member `%1` does not have getter or member-accessor").arg(member.getName()));
            }
            m_JsonObject[member.getName()] = serialisedValue;
        }
    }
private:
    const T&        m_ClassObj;
    nlohmann::json& m_JsonObject;
    ParseError&     m_Error;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename>
inline void Serialise(const T& obj, nlohmann::json &val, ParseError &err) {
    SerialiserFunc<T> serializeFunc(obj, val, err);
    meta::doForAllMembers<T>(serializeFunc);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename, typename>
inline void Serialise(const T& obj, nlohmann::json &json, ParseError &err) {
    SerialiseByType(obj, json, err);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const std::vector<T>& obj, nlohmann::json& json, ParseError &err) {
    std::vector<nlohmann::json> arr;
    int i = 0;
    for (const auto& elem: obj) {
        nlohmann::json val;
        Json::Serialise(elem, val, err);
        if (err) {
            return err.AddMessage(QString("Parsing element at index %1 in array").arg(i));
        }
        arr.push_back(val);
        ++i;
    }
    json = arr;
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const std::list<T>& obj, nlohmann::json& json, ParseError &err) {
    std::vector<nlohmann::json> arr;
    int i = 0;
    for (const auto& elem: obj) {
        nlohmann::json val;
        Json::Serialise(elem, val, err);
        if (err) {
            return err.AddMessage(QString("Parsing element at index %1 in array").arg(i));
        }
        arr.push_back(val);
        ++i;
    }
    json = arr;
}

////////////////////////////////////////////////////////////////////////////////
template <typename K, typename T>
inline void SerialiseByType(const std::unordered_map<K, T>& obj, nlohmann::json& json, ParseError& err) {
    for (const auto& [key, value] : obj) {
        nlohmann::json elem;
        Json::Serialise(value, elem, err);
        json[key] = elem;
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline std::string ToJson(const T& obj, ParseError &err, bool readable = false) noexcept {
    nlohmann::json json;
    Json::Serialise(obj, json, err);
    if (err) {
        return std::string();
    }
    return json.dump(readable ? 4 : -1);
}

////////////////////////////////////////////////////////////////////////////////
// Deserialization
////////////////////////////////////////////////////////////////////////////////

template <typename T,
    typename = std::enable_if_t<meta::isRegistered<T>()>>
void Deserialise(T& obj, const nlohmann::json& json, Json::ParseError &err);

template <typename T,
    typename = std::enable_if_t<!meta::isRegistered<T>()>,
    typename = void>
void Deserialise(T& obj, const nlohmann::json& json, Json::ParseError &err);

template <typename T>
void DeserialiseByType(T& obj, const nlohmann::json& json, Json::ParseError &err);

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(double& doubleVal, const nlohmann::json& json, ParseError &err) {
    if (json.is_number()) {
        doubleVal = json.template get<double>();
    } else {
        err.AddMessage(QString("Expected number, got %1").arg(json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(bool& val, const nlohmann::json& json, ParseError &err) {
    if (json.is_boolean()) {
        val = json.template get<bool>();
    } else {
        err.AddMessage(QString("Expected bool, got %1").arg(json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(std::string& val, const nlohmann::json& json, ParseError &err) {
    if (json.is_string()) {
        val = json.template get<std::string>();
    } else {
        err.AddMessage(QString("expected string, got %1").arg(json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(QString& val, const nlohmann::json& json, ParseError &err) {
    if (json.is_string()) {
        std::string str = json.template get<std::string>();
        val = QString::fromStdString(str);
    } else {
        err.AddMessage(QString("Expected string, got %1").arg(json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(BaseVec2<T>& val, const nlohmann::json& json, ParseError &err) {
    if (!json.is_object()) {
        return err.AddMessage(QString("Expected object, got %1").arg(json.type_name()));
    }
    if (!json.contains("x") || !json.contains("y")) {
        return err.AddMessage(QString("Expected object to have fields x and y: %1").arg(QString::fromStdString(json.dump())));
    }
    T x, y;
    Json::Deserialise(x, json["x"], err);
    Json::Deserialise(y, json["y"], err);
    if (err) {
        return err.AddMessage(QStringLiteral("Deserialising the elements of BaseVec2"));
    }
    val = BaseVec2<T>(x, y);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(BaseVec3<T>& val, const nlohmann::json& json, ParseError &err) {
    if (!json.is_object()) {
        return err.AddMessage(QString("Expected object, got %1").arg(json.type_name()));
    }
    if (!json.contains("x") || !json.contains("y") || !json.contains("z")) {
        return err.AddMessage(QString("Expected object to have fields x, y and z: %1").arg(QString::fromStdString(json.dump())));
    }
    T x, y, z;
    Json::Deserialise(x, json["x"], err);
    Json::Deserialise(y, json["y"], err);
    Json::Deserialise(z, json["z"], err);
    if (err) {
        return err.AddMessage(QStringLiteral("Deserialising the elements of BaseVec3"));
    }
    val = BaseVec3<T>(x, y, z);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(BaseVec4<T>& val, const nlohmann::json& json, ParseError &err) {
    if (!json.is_object()) {
        return err.AddMessage(QString("Expected object, got %1").arg(json.type_name()));
    }
    if (!json.contains("x") || !json.contains("y") || !json.contains("z") || !json.contains("w")) {
        return err.AddMessage(QString("Expected object to have fields x, y, z and w: %1").arg(QString::fromStdString(json.dump())));
    }
    T x, y, z, w;
    Json::Deserialise(x, json["x"], err);
    Json::Deserialise(y, json["y"], err);
    Json::Deserialise(z, json["z"], err);
    Json::Deserialise(w, json["w"], err);
    if (err) {
        return err.AddMessage(QStringLiteral("Deserialising the elements of BaseVec4"));
    }
    val = BaseVec4<T>(x, y, z, w);
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
class DeserialiserFunc {
public:
    DeserialiserFunc(T& obj, const nlohmann::json& value, ParseError &err) : m_ClassObj{obj}, m_JsonObject{value}, m_Error{err} {}

    template<typename Member>
    void operator()(Member& member) {
        if (m_Error) {
            return;
        }
        if constexpr (!meta::is_optional<meta::get_member_type<Member>>::value) {
            if (m_JsonObject.contains(member.getName())) {
                nlohmann::json member_val = m_JsonObject[member.getName()];
                if (!member_val.is_discarded()) {
                    using MemberT = meta::get_member_type<decltype(member)>;
                    if (member.hasSetter()) {
                        MemberT membType;
                        Json::Deserialise(membType, member_val, m_Error);
                        if (m_Error) {
                            return m_Error.AddMessage(QString("Parsing member %1").arg(member.getName()));
                        }
                        member.set(m_ClassObj, std::move(membType));
                    } else if (member.canGetRef()) {
                        MemberT membType;
                        Json::Deserialise(membType, member_val, m_Error);
                        if (m_Error) {
                            return m_Error.AddMessage(QString("Parsing member %1").arg(member.getName()));
                        }
                        member.getRef(m_ClassObj) = std::move(membType);
                    } else {
                        m_Error.AddMessage(QString("Cannot deserialize member %1, read only").arg(member.getName()));
                    }
                    return;
                }
            }
            m_Error.AddMessage(QString("Required key %1 not found or null").arg(member.getName()));
        } else {
            if (m_JsonObject.contains(member.getName())) {
                if (member.canGetRef()) {
                    nlohmann::json member_val = m_JsonObject[member.getName()];
                    if (!member_val.isNull()) {
                        using MemberT = typename meta::get_member_type<decltype(member)>::value_type;
                        MemberT memb_type;
                        Json::Deserialise(memb_type, member_val, m_Error);
                        if (m_Error) {
                            m_Error.AddMessage(QString("Parsing member %1").arg(member.getName()));
                            return;
                        }
                        member.getRef(m_ClassObj).emplace(std::move(memb_type));
                    }
                } else {
                    m_Error.AddMessage(QString("cannot get reference to std::optional member %1").arg(member.getName()));
                }
            }
        }
    }

private:
    T&                      m_ClassObj;
    const nlohmann::json&   m_JsonObject;
    ParseError&             m_Error;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename>
inline void Deserialise(T& obj, const nlohmann::json& json, ParseError &err) {
    if (json.is_object()) {
        DeserialiserFunc<T> deserializerFunc(obj, json, err);
        meta::doForAllMembers<T>(deserializerFunc);
    } else {
        err.AddMessage("Cannot deserialize from QJsonObject to Class");
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename, typename>
inline void Deserialise(T& obj, const nlohmann::json& json, ParseError &err) {
    DeserialiseByType(obj, json, err);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(T& obj, const nlohmann::json& json, ParseError &err) {

    if constexpr (std::is_enum<T>::value) {
        using UnderlyingType = typename std::underlying_type<T>::type;
        if (json.is_number()) {
            obj = static_cast<T>((UnderlyingType)json.template get<double>());
        } else {
            err.AddMessage(QString("Expected number, got %1").arg(json.type_name()));
        }
    } else if constexpr ((std::is_arithmetic<T>::value || std::is_unsigned<T>::value) && !std::is_same<T, bool>::value) {
        if (json.is_number()) {
            obj = static_cast<T>(json.template get<double>());
        } else {
            err.AddMessage(QString("Expected number, got %1").arg(json.type_name()));
        }
    } else {
        err.AddMessage(QString("Unimplemented deserialization. Input JSON value type is %1").arg(json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(std::vector<T>& result, const nlohmann::json& json, ParseError &err) {
    if (!json.is_array()) {
        return err.AddMessage(QString("Expected array, got %1").arg(json.type_name()));
    }
    for (const auto& item: json) {
        T elem;
        Json::Deserialise(elem, item, err);
        if (err) {
            return err.AddMessage(QString("Deserialising array element %1 to std::vector").arg(result.size()));
        }
        result.push_back(elem);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(std::list<T>& result, const nlohmann::json& json, ParseError &err) {
    if (!json.is_array()) {
        return err.AddMessage(QString("Expected array, got %1").arg(json.type_name()));
    }
    for (const auto& item : json) {
        T elem;
        Json::Deserialise(elem, item, err);
        if (err) {
            return err.AddMessage(QString("Deserialising array element %1 to std::list").arg(result.size()));
        }
        result.push_back(elem);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename K, typename T>
inline void DeserialiseByType(std::unordered_map<K, T>& result, const nlohmann::json& json, ParseError& err) {
    if (!json.is_object()) {
        return err.AddMessage(QString("Expected object, got %1").arg(json.type_name()));
    }
    for (auto iter = json.begin(); iter != json.end(); iter++) {
        T elem;
        Json::Deserialise(elem, iter.value(), err);
        result[iter.key()] = elem;
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void FromJson(std::string_view data, T& obj, ParseError &err) noexcept {
    auto json = nlohmann::json::parse(data, nullptr, false);
    if (json.is_discarded()) {
        err.AddMessage("Parse error");
        return;
    }

    Json::Deserialise(obj, json, err);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void FromJson(std::string_view data, std::vector<T>& obj, ParseError &err) noexcept {
    auto json = nlohmann::json::parse(data, nullptr, false);
    if (json.is_discarded()) {
        err.AddMessage("Parse error");
        return;
    }

    Json::Deserialise(obj, json, err);
}

}

////////////////////////////////////////////////////////////////////////////////
// Binary Serialization/Deserialization
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename = std::enable_if_t<meta::isRegistered<T>()>>
QDataStream &operator<<(QDataStream &out, const T &obj) {
    meta::doForAllMembers<T>(
        [&out, &obj](auto &member) {
            if (out.status() != QDataStream::Status::Ok) {
                return;
            }
            QByteArray temp_storage;
            QDataStream temp_out(&temp_storage, QIODevice::WriteOnly);

            if constexpr (meta::is_optional<meta::get_member_type<decltype(member)>>::value) {
                if (member.canGetConstRef()) {
                    auto& optionalVal = member.get(obj);
                    if(optionalVal) {
                        temp_out << *member.get(obj);
                        if (temp_out.status() != QDataStream::Status::Ok) {
                            qDebug() << "ToBinary: error parsing member " << member.getName();
                            return;
                        }
                    } else {
                        qint8 is_null = true;
                        temp_out << is_null;
                    }
                } else if (member.hasGetter()) {
                    auto optionalVal = member.getCopy(obj);
                    if(optionalVal) {
                        temp_out << *member.get(obj);
                        if (temp_out.status() != QDataStream::Status::Ok) {
                            qDebug() << "ToBinary: error parsing member " << member.getName();
                            return;
                        }
                    } else {
                        qint8 is_null = true;
                        temp_out << is_null;
                    }
                }
            } else {
                if (member.canGetConstRef()) {
                    temp_out << member.get(obj);
                    if (temp_out.status() != QDataStream::Status::Ok) {
                        qDebug() << "ToBinary: error parsing member " << member.getName();
                        return;
                    }
                } else if (member.hasGetter()) {
                    temp_out << member.getCopy(obj);
                    if (temp_out.status() != QDataStream::Status::Ok) {
                        qDebug() << "ToBinary: error parsing member " << member.getName();
                        return;
                    }
                } else {
                    qDebug() << "ToBinary: no getter or member-accessor for member " << member.getName();
                    return;
                }
            }
            out.writeRawData(temp_storage.data(), temp_storage.size());
        }
    );

    return out;
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename = std::enable_if_t<meta::isRegistered<T>()>>
QDataStream &operator>>(QDataStream &in, T &obj) {
    meta::doForAllMembers<T>(
        [&in, &obj](auto &member) {
            if (in.status() != QDataStream::Status::Ok) {
                return;
            }

            if constexpr (meta::is_optional<meta::get_member_type<decltype(member)>>::value) {
                if (member.canGetRef()) {
                    qint8 is_null = false;
                    in >> is_null;
                    if (!is_null) {
                        using MemberT = typename meta::get_member_type<decltype(member)>::value_type;
                        MemberT membType;
                        in >> membType;
                        if (in.status() != QDataStream::Status::Ok) {
                            qDebug() << "FromBinary: error parsing member " << member.getName();
                            return;
                        }
                        member.getRef(obj).emplace(std::move(membType));
                    }
                } else {
                    qDebug() << "FromBinary: cannot get reference to std::optional member " << member.getName();
                    return;
                }
            } else {
                using MemberT = meta::get_member_type<decltype(member)>;
                if (member.hasSetter() || member.hasPtr()) {
                    MemberT membType;
                    in >> membType;
                    if (in.status() != QDataStream::Status::Ok) {
                        qDebug() << "FromBinary: error parsing member " << member.getName();
                        return;
                    }

                    if (member.hasSetter()) {
                        if constexpr (std::is_trivially_move_constructible<MemberT>::value) {
                            member.set(obj, std::move(membType));
                        } else {
                            member.set(obj, membType);
                        }
                    } else if constexpr (std::is_same<MemberT, std::string>::value) {
                        member.set(obj, membType.c_str());
                    } else {
                        auto member_ptr = member.getPtr();
                        obj.*member_ptr = membType;
                    }
                } else if (member.canGetRef()) {
                    MemberT membType;
                    in >> membType;
                    if (in.status() != QDataStream::Status::Ok) {
                        qDebug() << "FromBinary: error parsing member " << member.getName();
                        return;
                    }

                    if constexpr (std::is_trivially_move_constructible<MemberT>::value) {
                        member.getRef(obj) = std::move(membType);
                    } else if constexpr (std::is_copy_assignable<MemberT>::value) {
                        member.getRef(obj) = membType;
                    } else {
                        qDebug() << "FromBinary: member " << member.getName() << " needs a reference setter function registered with MetaStuff";
                        return;
                    }
                } else {
                    qDebug() << "FromBinary: cannot deserialize read only member " << member.getName();
                    return;
                }
            }
        }
    );

    return in;
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline QDataStream &operator<<(QDataStream &out, const std::vector<T> &args) {
    out << (unsigned int)args.size();
    for (const auto& val: args) {
        out << val;
    }
    return out;
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline QDataStream &operator>>(QDataStream &in, std::vector<T> &args) {
    unsigned int length = 0;
    in >> length;
    args.clear();
    args.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        T obj;
        in >> obj;
        if (in.status() != QDataStream::Ok) {
            args.clear();
            break;
        }
        args.push_back(obj);
    }
    return in;
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline QDataStream &operator<<(QDataStream &out, const std::list<T> &args) {
    out << (unsigned int)args.size();
    for (const auto& val : args) {
        out << val;
    }
    return out;
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline QDataStream &operator>>(QDataStream &in, std::list<T> &args) {
    unsigned int length = 0;
    in >> length;
    args.clear();
    for (unsigned int i = 0; i < length; i++) {
        T obj;
        in >> obj;
        if (in.status() != QDataStream::Ok) {
            args.clear();
            break;
        }
        args.push_back(obj);
    }
    return in;
}

////////////////////////////////////////////////////////////////////////////////
template<typename K, typename V>
inline QDataStream &operator<<(QDataStream &out, const std::unordered_map<K, V> &args) {
    out << (unsigned int)args.size();
    for (const auto& [key, val]: args) {
        out << key;
        out << val;
    }
    return out;
}

////////////////////////////////////////////////////////////////////////////////
template<typename K, typename V>
inline QDataStream &operator>>(QDataStream &in, std::unordered_map<K, V> &args) {
    unsigned int length = 0;
    in >> length;
    args.clear();
    for (unsigned int i = 0; i < length; i++) {
        K key;
        V val;
        in >> key;
        in >> val;
        if (in.status() != QDataStream::Ok) {
            args.clear();
            break;
        }
        args.insert(std::pair(key, val));
    }
    return in;
}

////////////////////////////////////////////////////////////////////////////////
inline QDataStream &operator<<(QDataStream &out, const std::string &args) {
    if (args.empty()) {
        out << (unsigned int)0;
        return out;
    }
    out << (unsigned int)(args.size() + 1);
    out.writeRawData(args.data(), (int)args.size() + 1);
    return out;
}

////////////////////////////////////////////////////////////////////////////////
inline QDataStream &operator>>(QDataStream &in, std::string &args) {
    unsigned int length = 0;
    in >> length;
    args.clear();
    args.reserve(length);
    in.readRawData(args.data(), length);
    if (in.status() != QDataStream::Ok)
    {
        args.clear();
        return in;
    }
    return in;
}

////////////////////////////////////////////////////////////////////////////////
inline QDataStream &operator<<(QDataStream &out, const long &args) {
    out << (uint64_t)(args);
    return out;
}

////////////////////////////////////////////////////////////////////////////////
inline QDataStream &operator>>(QDataStream &in, long &args) {
    in >> (uint64_t&)args;
    return in;
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline QDataStream& operator<<(QDataStream& out, const BaseVec2<T>& args) {
    out << args.x << args.y;
    return out;
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline QDataStream& operator>>(QDataStream& in, BaseVec2<T>& args) {
    in >> args.x >> args.y;
    return in;
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline QDataStream& operator<<(QDataStream& out, const BaseVec3<T>& args) {
    out << args.x << args.y << args.z;
    return out;
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline QDataStream& operator>>(QDataStream& in, BaseVec3<T>& args) {
    in >> args.x >> args.y >> args.z;
    return in;
}

} // namespace Neshny