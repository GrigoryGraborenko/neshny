
namespace meta {
template <typename T>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional< std::optional<T> > : std::true_type {};
}

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
QString JsonValueTypeName(QJsonValue::Type type) {
    switch(type) {
        case QJsonValue::Null: return "Null";
        case QJsonValue::Bool: return "Bool";
        case QJsonValue::Double: return "Double";
        case QJsonValue::String: return "String";
        case QJsonValue::Array: return "Array";
        case QJsonValue::Object: return "Object";
        default:return "Undefined";
    }
}

////////////////////////////////////////////////////////////////////////////////
// Serialization
////////////////////////////////////////////////////////////////////////////////

template <typename T, typename = std::enable_if_t <meta::isRegistered<T>()>>
void Serialise(const T& obj, QJsonValue &jVal, Json::ParseError &err);

template <typename T, typename = std::enable_if_t <!meta::isRegistered<T>()>, typename = void>
void Serialise(const T& obj, QJsonValue &jVal, Json::ParseError &err);

////////////////////////////////////////////////////////////////////////////////
template <typename Class>
inline void SerialiseByType(const Class& obj, QJsonValue &json, [[maybe_unused]] ParseError &err) {
    if constexpr (std::is_same<Class, bool>::value) {
        json = QJsonValue(static_cast<bool>(obj));
    } else if constexpr (std::is_same<Class, float>::value) {
        json = QJsonValue(static_cast<double>(obj));
    } else if constexpr (std::is_same<Class, double>::value) {
        json = QJsonValue(static_cast<double>(obj));
    } else if constexpr (std::is_unsigned<Class>::value) {
        json = QJsonValue(static_cast<qint64>(obj));
    } else if constexpr (std::is_arithmetic<Class>::value) {
        json = QJsonValue(static_cast<qint64>(obj));
    } else if constexpr (std::is_enum<Class>::value) {
        json = QJsonValue(static_cast<int>(obj));
    } else {
        json = QJsonValue(obj);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void SerialiseByType(const QVariant& val, QJsonValue &json, [[maybe_unused]] ParseError &err) {
    json = QJsonValue::fromVariant(val);
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void SerialiseByType(const QJsonValue& val, QJsonValue & json, [[maybe_unused]] ParseError &err) {
    json = val;
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void SerialiseByType(const std::string& val, QJsonValue & json, [[maybe_unused]] ParseError &err) {
    json = QJsonValue(val.c_str());
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void SerialiseByType(const QVector2D& val, QJsonValue & json, [[maybe_unused]] ParseError &err) {
    json = QJsonObject({{"x", val.x()}, {"y", val.y()}});
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void SerialiseByType(const QVector3D& val, QJsonValue & json, [[maybe_unused]] ParseError &err) {
    json = QJsonObject({{"x", val.x()}, {"y", val.y()}, {"z", val.z()} });
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
class SerialiserFunc {
public:
    SerialiserFunc(const T& obj, QJsonObject& json, ParseError &err) : m_ClassObj(obj), m_JsonObject(json), m_Error(err) {}

    template<typename Member>
    void operator()(Member& member) {
        if (m_Error) {
            return;
        }
        if constexpr (meta::is_optional<meta::get_member_type<Member>>::value) {
            if (member.canGetConstRef()) {
                auto& optionalVal = member.get(m_ClassObj);
                if(optionalVal) {
                    QJsonValue serialisedValue;
                    Json::Serialise(*member.get(m_ClassObj), serialisedValue, m_Error);
                    if (m_Error) {
                        return m_Error.AddMessage(QString("parsing member %1 failed").arg(member.getName()));
                    }
                    m_JsonObject[member.getName()] = serialisedValue;
                }
            }
        } else {
            QJsonValue serialisedValue;
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
    QJsonObject&    m_JsonObject;
    ParseError&     m_Error;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename>
inline void Serialise(const T& obj, QJsonValue &val, ParseError &err) {
    QJsonObject json = val.toObject();
    SerialiserFunc<T> serializeFunc(obj, json, err);
    meta::doForAllMembers<T>(serializeFunc);
    val = json;
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename, typename>
inline void Serialise(const T& obj, QJsonValue &json, ParseError &err) {
    SerialiseByType(obj, json, err);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void SerialiseByType(const std::vector<T>& obj, QJsonValue& json, ParseError &err) {
    QJsonArray arr;
    int i = 0;
    for (const auto& elem: obj) {
        QJsonValue val;
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
inline void SerialiseByType(const std::list<T>& obj, QJsonValue& json, ParseError &err) {
    QJsonArray arr;
    int i = 0;
    for (const auto& elem: obj) {
        QJsonValue val;
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
inline QByteArray ToJson(const T& obj, ParseError &err) noexcept {
    QJsonValue json;
    Json::Serialise(obj, json, err);
    if (err) {
        return QByteArray();
    }
    QJsonDocument doc;
    if (json.isArray()) {
        doc.setArray(json.toArray());
    } else {
        doc.setObject(json.toObject());
    }
    return doc.toJson();
}

////////////////////////////////////////////////////////////////////////////////
// Deserialization
////////////////////////////////////////////////////////////////////////////////

template <typename T,
    typename = std::enable_if_t<meta::isRegistered<T>()>>
void Deserialise(T& obj, const QJsonValue& json, Json::ParseError &err);

template <typename T,
    typename = std::enable_if_t<!meta::isRegistered<T>()>,
    typename = void>
void Deserialise(T& obj, const QJsonValue& json, Json::ParseError &err);

template <typename T>
void DeserialiseByType(T& obj, const QJsonValue& json, Json::ParseError &err);

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(double& doubleVal, const QJsonValue& json, ParseError &err) {
    if (json.isDouble()) {
        doubleVal = json.toDouble();
    } else {
        err.AddMessage(QString("Expected number, got %1").arg(JsonValueTypeName(json.type())));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(bool& val, const QJsonValue& json, ParseError &err) {
    if (json.isBool()) {
        val = json.toBool();
    } else {
        err.AddMessage(QString("Expected bool, got %1").arg(JsonValueTypeName(json.type())));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(QVariant& val, const QJsonValue& json, [[maybe_unused]] ParseError &err) {
    val = json.toVariant();
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(QJsonObject& val, const QJsonValue& json, [[maybe_unused]] ParseError &err) {
    val = json.toObject();
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(std::string& val, const QJsonValue& json, ParseError &err) {
    if (json.isString()) {
        val = json.toString().toStdString();
    } else {
        err.AddMessage(QString("expected string, got %1").arg(JsonValueTypeName(json.type())));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(QString& val, const QJsonValue& json, ParseError &err) {
    if (json.isString()) {
        val = json.toString();
    } else {
        err.AddMessage(QString("Expected string, got %1").arg(JsonValueTypeName(json.type())));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(QVector2D& val, const QJsonValue& json, ParseError &err) {
    if (json.isObject()) {
        QJsonObject jObj = json.toObject();
        if (!jObj.contains("x") || !jObj.contains("y")) {
            return err.AddMessage(QString("Expected object to have two fields x and y, got %1").arg(jObj.keys().join(", ")));
        }
        int x, y;
        Json::Deserialise(x, jObj.value("x"), err);
        Json::Deserialise(y, jObj.value("y"), err);
        if (err) {
            return err.AddMessage(QStringLiteral("Deserialising the elements of QVector2D"));
        }
        val = QVector2D(x, y);
    } else {
        return err.AddMessage(QString("Expected object, got %1").arg(JsonValueTypeName(json.type())));
    }
}

////////////////////////////////////////////////////////////////////////////////
template <>
inline void DeserialiseByType(QVector3D& val, const QJsonValue& json, ParseError &err) {
    if (json.isObject()) {
        QJsonObject jObj = json.toObject();
        if (!jObj.contains("x") || !jObj.contains("y") || !jObj.contains("z")) {
            return err.AddMessage(QString("Expected object to have fields x, y and z, got %1").arg(jObj.keys().join(", ")));
        }
        int x, y,z;
        Json::Deserialise(x, jObj.value("x"), err);
        Json::Deserialise(y, jObj.value("y"), err);
        Json::Deserialise(z, jObj.value("z"), err);
        if (err) {
            return err.AddMessage(QStringLiteral("Deserialising the elements of QVector3D"));
        }
        val = QVector3D(x, y, z);
    } else {
        return err.AddMessage(QString("Expected object, got %1").arg(JsonValueTypeName(json.type())));
    }
}


////////////////////////////////////////////////////////////////////////////////
template<typename T>
class DeserialiserFunc {
public:
    DeserialiserFunc(T& obj, const QJsonObject& value, ParseError &err) : m_ClassObj{obj}, m_JsonObject{value}, m_Error{err} {}

    template<typename Member>
    void operator()(Member& member) {
        if (m_Error) {
            return;
        }
        if constexpr (!meta::is_optional<meta::get_member_type<Member>>::value) {
            auto it = m_JsonObject.find(member.getName());
            if(it != m_JsonObject.end()) {
                QJsonValue jMemberVal = it.value();
                if (!jMemberVal.isNull()) {
                    using MemberT = meta::get_member_type<decltype(member)>;
                    if (member.hasSetter()) {
                        MemberT membType;
                        Json::Deserialise(membType, jMemberVal, m_Error);
                        if (m_Error) {
                            return m_Error.AddMessage(QString("Parsing member %1").arg(member.getName()));
                        }
                        member.set(m_ClassObj, std::move(membType));
                    } else if (member.canGetRef()) {
                        MemberT membType;
                        Json::Deserialise(membType, jMemberVal, m_Error);
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
            auto it = m_JsonObject.find(member.getName());
            if(it != m_JsonObject.end()) {
                if (member.canGetRef()) {
                    QJsonValue jMemberVal = it.value();
                    if (!jMemberVal.isNull()) {
                        using MemberT = typename meta::get_member_type<decltype(member)>::value_type;
                        MemberT membType;
                        Json::Deserialise(membType, jMemberVal, m_Error);
                        if (m_Error) {
                            m_Error.AddMessage(QString("Parsing member %1").arg(member.getName()));
                            return;
                        }
                        member.getRef(m_ClassObj).insert_or_assign(membType);
                    }
                } else {
                    m_Error.AddMessage(QString("cannot get reference to std::optional member %1").arg(member.getName()));
                }
            }
        }
    }

private:
    T&                  m_ClassObj;
    const QJsonObject&  m_JsonObject;
    ParseError&         m_Error;
};

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename>
inline void Deserialise(T& obj, const QJsonValue& json, ParseError &err) {
    if (json.isObject()) {
        DeserialiserFunc<T> deserializerFunc(obj, json.toObject(), err);
        meta::doForAllMembers<T>(deserializerFunc);
    } else {
        err.AddMessage("Cannot deserialize from QJsonObject to Class");
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T, typename, typename>
inline void Deserialise(T& obj, const QJsonValue& json, ParseError &err) {
    DeserialiseByType(obj, json, err);
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(T& obj, const QJsonValue& json, ParseError &err) {
    if constexpr ((std::is_arithmetic<T>::value || std::is_unsigned<T>::value) && !std::is_same<T, bool>::value) {
        if (json.isDouble()) {
            obj = static_cast<T>(json.toDouble());
        } else {
            err.AddMessage(QString("Expected number, got %1").arg(JsonValueTypeName(json.type())));
        }
    } else if constexpr (std::is_enum<T>::value) {
        using UnderlyingType = typename std::underlying_type<T>::type;
        if (json.isDouble()) {
            obj = static_cast<T>((UnderlyingType)json.toDouble());
        } else {
            err.AddMessage(QString("Expected number, got %1").arg(JsonValueTypeName(json.type())));
        }
    } else {
        QVariant var = json.toVariant();
        if (var.canConvert<T>()) {
            obj = var.value<T>();
        } else {
            err.AddMessage(QString("Unimplemented deserialization. Input JSON value type is %1").arg(JsonValueTypeName(json.type())));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(std::vector<T>& vec, const QJsonValue& json, ParseError &err) {
    if (!json.isArray()) {
        return err.AddMessage(QString("Expected array, got %1").arg(json.type()));
    }
    const QJsonArray arr = json.toArray();
    vec.reserve(arr.size());
    for (auto ite = arr.cbegin(); ite != arr.cend(); ++ite) {
        T elemType;
        Json::Deserialise(elemType, *ite, err);
        if (err) {
            return err.AddMessage(QString("Deserialising array element %1 to std::vector").arg(vec.size()));
        }
        vec.push_back(elemType);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void DeserialiseByType(std::list<T>& lst, const QJsonValue& json, ParseError &err) {
    if (!json.isArray()) {
        return err.AddMessage(QString("Expected array, got %1").arg(json.type()));
    }
    const QJsonArray arr = json.toArray();
    for (auto ite = arr.cbegin(); ite != arr.cend(); ++ite) {
        T elemType;
        Json::Deserialise(elemType, *ite, err);
        if (err) {
            return err.AddMessage(QString("Deserialising array element %1 to std::list").arg(lst.size()));
        }
        lst.push_back(elemType);
    }
}

////////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void FromJson(const QByteArray& data, T& obj, ParseError &err) noexcept {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error) {
        err.AddMessage(error.errorString());
        return;
    }
    if (doc.isArray()) {
        Json::Deserialise(obj, doc.array(), err);
    } else {
        Json::Deserialise(obj, doc.object(), err);
    }
}

}