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
    inline void AddMessage(std::string_view errStr) { m_ErrorList.push_back(std::string(errStr)); }
    inline std::string Error() const { return JoinStrings(m_ErrorList, ", "); }
    inline operator bool() const { return !m_ErrorList.empty(); }
private:
    std::vector<std::string> m_ErrorList;
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
        json = static_cast<int64_t>(obj);
    } else if constexpr (std::is_arithmetic<Class>::value) {
        json = static_cast<int64_t>(obj);
    } else if constexpr (std::is_enum<Class>::value) {
        json = static_cast<int>(obj);
    } else {
        json = obj;
    }
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
template <typename T>
inline void SerialiseByType(const std::vector<T>& obj, nlohmann::json& json, ParseError &err) {
    std::vector<nlohmann::json> arr;
    int i = 0;
    for (const auto& elem: obj) {
        nlohmann::json val;
        Json::Serialise(elem, val, err);
        if (err) {
            return err.AddMessage(std::format("Parsing element at index {} in array", i));
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
            return err.AddMessage(std::format("Parsing element at index {} in array", i));
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
                auto& optional_val = member.get(m_ClassObj);
                if(optional_val) {
                    nlohmann::json serialised_value;
                    Json::Serialise(*member.get(m_ClassObj), serialised_value, m_Error);
                    if (m_Error) {
                        return m_Error.AddMessage(std::format("parsing member {} failed", member.getName()));
                    }
                    m_JsonObject[member.getName()] = serialised_value;
                }
            }
        } else {
            nlohmann::json serialised_value;
            if (member.canGetConstRef()) {
                Json::Serialise(member.get(m_ClassObj), serialised_value, m_Error);
                if (m_Error) {
                    return m_Error.AddMessage(std::format("parsing member {} failed", member.getName()));
                }
            } else if (member.hasGetter()) {
                Json::Serialise(member.getCopy(m_ClassObj), serialised_value, m_Error); // passing copy as const ref
                if (m_Error) {
                    return m_Error.AddMessage(std::format("parsing member {} failed", member.getName()));
                }
            } else {
                return m_Error.AddMessage(std::format("member `{}` does not have getter or member-accessor", member.getName()));
            }
            m_JsonObject[member.getName()] = serialised_value;
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

template <typename T, typename = std::enable_if_t<meta::isRegistered<T>()>>
void Deserialise(T& obj, const nlohmann::json& json, Json::ParseError &err);

template <typename T, typename = std::enable_if_t<!meta::isRegistered<T>()>, typename = void>
void Deserialise(T& obj, const nlohmann::json& json, Json::ParseError &err);

template <typename T>
void DeserialiseByType(T& obj, const nlohmann::json& json, Json::ParseError &err);

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(double& doubleVal, const nlohmann::json& json, ParseError &err) {
    if (json.is_number()) {
        doubleVal = json.template get<double>();
    } else {
        err.AddMessage(std::format("Expected number, got {}", json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(bool& val, const nlohmann::json& json, ParseError &err) {
    if (json.is_boolean()) {
        val = json.template get<bool>();
    } else {
        err.AddMessage(std::format("Expected bool, got {}", json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(std::string& val, const nlohmann::json& json, ParseError &err) {
    if (json.is_string()) {
        val = json.template get<std::string>();
    } else {
        err.AddMessage(std::format("expected string, got {}", json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(BaseVec2<T>& val, const nlohmann::json& json, ParseError &err) {
    if (!json.is_object()) {
        return err.AddMessage(std::format("Expected object, got {}", json.type_name()));
    }
    if (!json.contains("x") || !json.contains("y")) {
        return err.AddMessage(std::format("Expected object to have fields x and y: {}", json.dump()));
    }
    T x, y;
    Json::Deserialise(x, json["x"], err);
    Json::Deserialise(y, json["y"], err);
    if (err) {
        return err.AddMessage("Deserialising the elements of BaseVec2");
    }
    val = BaseVec2<T>(x, y);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(BaseVec3<T>& val, const nlohmann::json& json, ParseError &err) {
    if (!json.is_object()) {
        return err.AddMessage(std::format("Expected object, got {}", json.type_name()));
    }
    if (!json.contains("x") || !json.contains("y") || !json.contains("z")) {
        return err.AddMessage(std::format("Expected object to have fields x, y and z: {}", json.dump()));
    }
    T x, y, z;
    Json::Deserialise(x, json["x"], err);
    Json::Deserialise(y, json["y"], err);
    Json::Deserialise(z, json["z"], err);
    if (err) {
        return err.AddMessage("Deserialising the elements of BaseVec3");
    }
    val = BaseVec3<T>(x, y, z);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(BaseVec4<T>& val, const nlohmann::json& json, ParseError &err) {
    if (!json.is_object()) {
        return err.AddMessage(std::format("Expected object, got {}", json.type_name()));
    }
    if (!json.contains("x") || !json.contains("y") || !json.contains("z") || !json.contains("w")) {
        return err.AddMessage(std::format("Expected object to have fields x, y, z and w: {}", json.dump()));
    }
    T x, y, z, w;
    Json::Deserialise(x, json["x"], err);
    Json::Deserialise(y, json["y"], err);
    Json::Deserialise(z, json["z"], err);
    Json::Deserialise(w, json["w"], err);
    if (err) {
        return err.AddMessage("Deserialising the elements of BaseVec4");
    }
    val = BaseVec4<T>(x, y, z, w);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(T& obj, const nlohmann::json& json, ParseError &err) {

    if constexpr (std::is_enum<T>::value) {
        using UnderlyingType = typename std::underlying_type<T>::type;
        if (json.is_number()) {
            obj = static_cast<T>((UnderlyingType)json.template get<double>());
        } else {
            err.AddMessage(std::format("Expected number, got {}", json.type_name()));
        }
    } else if constexpr ((std::is_arithmetic<T>::value || std::is_unsigned<T>::value) && !std::is_same<T, bool>::value) {
        if (json.is_number()) {
            obj = static_cast<T>(json.template get<double>());
        } else {
            err.AddMessage(std::format("Expected number, got {}", json.type_name()));
        }
    } else {
        err.AddMessage(std::format("Unimplemented deserialization. Input JSON value type is {}", json.type_name()));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(std::vector<T>& result, const nlohmann::json& json, ParseError &err) {
    if (!json.is_array()) {
        return err.AddMessage(std::format("Expected array, got {}", json.type_name()));
    }
    for (const auto& item: json) {
        T elem;
        Json::Deserialise(elem, item, err);
        if (err) {
            return err.AddMessage(std::format("Deserialising array element {} to std::vector", result.size()));
        }
        result.push_back(elem);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(std::list<T>& result, const nlohmann::json& json, ParseError &err) {
    if (!json.is_array()) {
        return err.AddMessage(std::format("Expected array, got {}", json.type_name()));
    }
    for (const auto& item : json) {
        T elem;
        Json::Deserialise(elem, item, err);
        if (err) {
            return err.AddMessage(std::format("Deserialising array element {} to std::list", result.size()));
        }
        result.push_back(elem);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename K, typename T>
inline void DeserialiseByType(std::unordered_map<K, T>& result, const nlohmann::json& json, ParseError& err) {
    if (!json.is_object()) {
        return err.AddMessage(std::format("Expected object, got {}", json.type_name()));
    }
    for (auto iter = json.begin(); iter != json.end(); iter++) {
        T elem;
        Json::Deserialise(elem, iter.value(), err);
        result[iter.key()] = elem;
    }
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
                            return m_Error.AddMessage(std::format("Parsing member {}", member.getName()));
                        }
                        member.set(m_ClassObj, std::move(membType));
                    } else if (member.canGetRef()) {
                        MemberT membType;
                        Json::Deserialise(membType, member_val, m_Error);
                        if (m_Error) {
                            return m_Error.AddMessage(std::format("Parsing member {}", member.getName()));
                        }
                        member.getRef(m_ClassObj) = std::move(membType);
                    } else {
                        m_Error.AddMessage(std::format("Cannot deserialize member {}, read only", member.getName()));
                    }
                    return;
                }
            }
            m_Error.AddMessage(std::format("Required key {} not found or null", member.getName()));
        } else {
            if (m_JsonObject.contains(member.getName())) {
                if (member.canGetRef()) {
                    nlohmann::json member_val = m_JsonObject[member.getName()];
                    using MemberT = typename meta::get_member_type<decltype(member)>::value_type;
                    MemberT memb_type;
                    Json::Deserialise(memb_type, member_val, m_Error);
                    if (m_Error) {
                        m_Error.AddMessage(std::format("Parsing member {}", member.getName()));
                        return;
                    }
                    member.getRef(m_ClassObj).emplace(std::move(memb_type));
                } else {
                    m_Error.AddMessage(std::format("cannot get reference to std::optional member {}", member.getName()));
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

} // namespace Json

////////////////////////////////////////////////////////////////////////////////
// Binary Serialization/Deserialization
////////////////////////////////////////////////////////////////////////////////
namespace Binary {

////////////////////////////////////////////////////////////////////////////////
class ParseError {
public:
    explicit ParseError() = default;
    inline void AddMessage(std::string_view errStr) { m_ErrorList.push_back(std::string(errStr)); }
    inline std::string Error() const { return JoinStrings(m_ErrorList, ", "); }
    inline operator bool() const { return !m_ErrorList.empty(); }
private:
    std::vector<std::string> m_ErrorList;
};

////////////////////////////////////////////////////////////////////////////////
// Serialization
////////////////////////////////////////////////////////////////////////////////

template <typename T, typename = std::enable_if_t <meta::isRegistered<T>()>>
void Serialise(const T& obj, std::stringstream& stream, ParseError &err);

template <typename T, typename = std::enable_if_t <!meta::isRegistered<T>()>, typename = void>
void Serialise(const T& obj, std::stringstream& stream, ParseError &err);

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const T& obj, std::stringstream& stream, [[maybe_unused]] ParseError &err) {
    if constexpr (std::is_enum<T>::value) {
        int32_t value = static_cast<int>(obj);
        stream.write((char const*)&value, sizeof(value));
    //} else if constexpr (std::is_arithmetic<T>::value) {
    //    stream.write((char const*)&obj, sizeof(obj));
    } else {
        stream.write((char const*)&obj, sizeof(obj));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void SerialiseByType(const std::string& val, std::stringstream& stream, ParseError &err) {
    int32_t size = val.size();
    Serialise(size, stream, err);
    stream.write(val.data(), val.size());
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const std::vector<T>& obj, std::stringstream& stream, ParseError& err) {
    int32_t size = (int32_t)obj.size();
    Serialise(size, stream, err);
    int i = 0;
    for (const auto& elem: obj) {
        Serialise(elem, stream, err);
        if (err) {
            return err.AddMessage(std::format("Parsing element at index {} in array", i));
        }
        i++;
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const std::list<T>& obj, std::stringstream& stream, ParseError& err) {
    int32_t size = (int32_t)obj.size();
    Serialise(size, stream, err);
    int i = 0;
    for (const auto& elem : obj) {
        Serialise(elem, stream, err);
        if (err) {
            return err.AddMessage(std::format("Parsing element at index {} in array", i));
        }
        i++;
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename K, typename T>
inline void SerialiseByType(const std::unordered_map<K, T>& obj, std::stringstream& stream, ParseError& err) {

    int32_t size = (int32_t)obj.size();
    Serialise(size, stream, err);

    int i = 0;
    for (const auto& [key, val] : obj) {
        Serialise(key, stream, err);
        if (err) {
            return err.AddMessage(std::format("Parsing key at index {} in map", i));
        }
        Serialise(val, stream, err);
        if (err) {
            return err.AddMessage(std::format("Parsing value at index {} in map", i));
        }
        i++;
    }
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
class SerialiserFunc {
public:
    SerialiserFunc(const T& obj, std::stringstream& stream, ParseError &err) : m_ClassObj(obj), m_StreamObject(stream), m_Error(err) {}

    template<typename Member>
    void operator()(Member& member) {
        if (m_Error) {
            return;
        }
        if constexpr (meta::is_optional<meta::get_member_type<Member>>::value) {
            if (member.canGetConstRef()) {
                auto& optionalVal = member.get(m_ClassObj);
                bool has_val = optionalVal.has_value();
                Serialise(has_val, m_StreamObject, m_Error);
                if(has_val) {
                    Serialise(*member.get(m_ClassObj), m_StreamObject, m_Error);
                    if (m_Error) {
                        return m_Error.AddMessage(std::format("parsing member {} failed", member.getName()));
                    }
                }
            }
        } else {
            if (member.canGetConstRef()) {
                Serialise(member.get(m_ClassObj), m_StreamObject, m_Error);
                if (m_Error) {
                    return m_Error.AddMessage(std::format("parsing member {} failed", member.getName()));
                }
            } else if (member.hasGetter()) {
                Serialise(member.getCopy(m_ClassObj), m_StreamObject, m_Error); // passing copy as const ref
                if (m_Error) {
                    return m_Error.AddMessage(std::format("parsing member {} failed", member.getName()));
                }
            } else {
                return m_Error.AddMessage(std::format("member `{}` does not have getter or member-accessor", member.getName()));
            }
        }
    }
private:
    const T&        m_ClassObj;
    std::stringstream& m_StreamObject;
    ParseError&     m_Error;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename>
inline void Serialise(const T& obj, std::stringstream& stream, ParseError &err) {
    SerialiserFunc<T> serializeFunc(obj, stream, err);
    meta::doForAllMembers<T>(serializeFunc);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename, typename>
inline void Serialise(const T& obj, std::stringstream& stream, ParseError &err) {
    SerialiseByType(obj, stream, err);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline std::string ToBinary(const T& obj, ParseError &err) noexcept {
    std::stringstream stream;
    Serialise(obj, stream, err);
    return stream.str();
}

////////////////////////////////////////////////////////////////////////////////
// Deserialization
////////////////////////////////////////////////////////////////////////////////

template <typename T, typename = std::enable_if_t<meta::isRegistered<T>()>>
void Deserialise(T& obj, std::stringstream& stream, Binary::ParseError &err);

template <typename T, typename = std::enable_if_t<!meta::isRegistered<T>()>, typename = void>
void Deserialise(T& obj, std::stringstream& stream, Binary::ParseError &err);

template <typename T>
void DeserialiseByType(T& obj, std::stringstream& stream, Binary::ParseError &err);

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(std::string& val, std::stringstream& stream, ParseError &err) {
    int32_t size;
    Deserialise(size, stream, err);
    val.resize(size);
    stream.read(val.data(), val.size());
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(T& obj, std::stringstream& stream, ParseError &err) {
    if constexpr (std::is_enum<T>::value) {
        int32_t value;
        stream.read((char*)&value, sizeof(value));
        obj = static_cast<T>(value);
    } else {
        stream.read((char*)&obj, sizeof(obj));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(std::vector<T>& result, std::stringstream& stream, ParseError &err) {
    int32_t size;
    Deserialise(size, stream, err);
    for (int i = 0; i < size; i++) {
        T elem;
        Deserialise(elem, stream, err);
        if (err) {
            return err.AddMessage(std::format("Deserialising array element {} to std::vector", i));
        }
        result.push_back(elem);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(std::list<T>& result, std::stringstream& stream, ParseError &err) {
    int32_t size;
    Deserialise(size, stream, err);
    for (int i = 0; i < size; i++) {
        T elem;
        Deserialise(elem, stream, err);
        if (err) {
            return err.AddMessage(std::format("Deserialising array element {} to std::list", i));
        }
        result.push_back(elem);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename K, typename T>
inline void DeserialiseByType(std::unordered_map<K, T>& result, std::stringstream& stream, ParseError& err) {

    int32_t size;
    Deserialise(size, stream, err);
    for (int i = 0; i < size; i++) {
        K key;
        Deserialise(key, stream, err);
        if (err) {
            return err.AddMessage(std::format("Deserialising key {} to map", i));
        }
        T value;
        Deserialise(value, stream, err);
        if (err) {
            return err.AddMessage(std::format("Deserialising value {} to map", i));
        }
        result[key] = value;
    }
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
class DeserialiserFunc {
public:
    DeserialiserFunc(T& obj, std::stringstream& stream, ParseError &err) : m_ClassObj{obj}, m_StreamObject{stream}, m_Error{err} {}

    template<typename Member>
    void operator()(Member& member) {
        if (m_Error) {
            return;
        }
        if constexpr (!meta::is_optional<meta::get_member_type<Member>>::value) {
            using MemberT = meta::get_member_type<decltype(member)>;
            MemberT member_val;
            Deserialise(member_val, m_StreamObject, m_Error);
            if (m_Error) {
                return m_Error.AddMessage(std::format("Parsing member {}", member.getName()));
            }
            if (member.hasSetter()) {
                member.set(m_ClassObj, std::move(member_val));
            } else if (member.canGetRef()) {
                member.getRef(m_ClassObj) = std::move(member_val);
            } else {
                m_Error.AddMessage(std::format("Cannot deserialize member {}, read only", member.getName()));
            }
        } else {
            bool has_val;
            Deserialise(has_val, m_StreamObject, m_Error);
            if (has_val) {
                if (member.canGetRef()) {
                    using MemberT = typename meta::get_member_type<decltype(member)>::value_type;
                    MemberT memb_type;
                    Deserialise(memb_type, m_StreamObject, m_Error);
                    if (m_Error) {
                        m_Error.AddMessage(std::format("Parsing member {}", member.getName()));
                        return;
                    }
                    member.getRef(m_ClassObj).emplace(std::move(memb_type));
                } else {
                    m_Error.AddMessage(std::format("cannot get reference to std::optional member {}", member.getName()));
                }
            }
        }
    }

private:
    T&                      m_ClassObj;
    std::stringstream&      m_StreamObject;
    ParseError&             m_Error;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename>
inline void Deserialise(T& obj, std::stringstream& stream, ParseError &err) {
    DeserialiserFunc<T> deserializerFunc(obj, stream, err);
    meta::doForAllMembers<T>(deserializerFunc);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename, typename>
inline void Deserialise(T& obj, std::stringstream& stream, ParseError &err) {
    DeserialiseByType(obj, stream, err);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void FromBinary(std::string_view data, T& obj, ParseError &err) noexcept {
    std::stringstream in_stream(std::string(data), std::ios_base::in);
    Deserialise(obj, in_stream, err);
}

} //namespace Binary

} //namespace Neshny