/**
   @author Kenta Suzuki
*/

#include "rqt_ur/json_archive.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace rqt_ur {

class JsonArchive::Impl
{
public:
    JsonArchive* self;

    Impl(JsonArchive* self);
};

JsonArchive::JsonArchive()
{
    impl = new Impl(this);
}

JsonArchive::Impl::Impl(JsonArchive* self)
    : self(self)
{

}

JsonArchive::~JsonArchive()
{
    delete impl;
}

bool JsonArchive::loadFile(const QString& fileName)
{
    QFile loadFile(fileName);

    if(!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    read(loadDoc.object());

    return true;
}

bool JsonArchive::saveFile(const QString& fileName)
{
    QFile saveFile(fileName);

    if(!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonObject jsonObject;
    write(jsonObject);
    saveFile.write(QJsonDocument(jsonObject).toJson());

    return true;
}

int JsonArchive::get(const QJsonObject& json, const QString& key, const int& init) const
{
    int ret = init;
    if(json.contains(key) && json[key].isDouble()) {
        ret = json[key].toInt();
    }
    return ret;
}

double JsonArchive::get(const QJsonObject& json, const QString& key, const double& init) const
{
    double ret = init;
    if(json.contains(key) && json[key].isDouble()) {
        ret = json[key].toDouble();
    }
    return ret;
}

bool JsonArchive::get(const QJsonObject& json, const QString& key, const bool& init) const
{
    bool ret = init;
    if(json.contains(key) && json[key].isBool()) {
        ret = json[key].toBool();
    }
    return ret;
}

QString JsonArchive::get(const QJsonObject& json, const QString& key, const char* init) const
{
    QString ret = init;
    if(json.contains(key) && json[key].isString()) {
        ret = json[key].toString();
    }
    return ret;
}

bool JsonArchive::get(const QJsonObject& json, const QString& key, QVector<int>& vector)
{
    vector.clear();
    if(json.contains(key) && json[key].isArray()) {
        QJsonArray valueArray = json[key].toArray();
        for(int i = 0; i < valueArray.size(); ++i) {
            vector.push_back(valueArray[i].toInt());
        }
        return true;
    }
    return false;
}

bool JsonArchive::get(const QJsonObject& json, const QString& key, QVector<double>& vector)
{
    vector.clear();
    if(json.contains(key) && json[key].isArray()) {
        QJsonArray valueArray = json[key].toArray();
        for(int i = 0; i < valueArray.size(); ++i) {
            vector.push_back(valueArray[i].toDouble());
        }
        return true;
    }
    return false;
}

bool JsonArchive::get(const QJsonObject& json, const QString& key, QVector<bool>& vector)
{
    vector.clear();
    if(json.contains(key) && json[key].isArray()) {
        QJsonArray valueArray = json[key].toArray();
        for(int i = 0; i < valueArray.size(); ++i) {
            vector.push_back(valueArray[i].toBool());
        }
        return true;
    }
    return false;
}

bool JsonArchive::get(const QJsonObject& json, const QString& key, QVector<QString>& vector)
{
    vector.clear();
    if(json.contains(key) && json[key].isArray()) {
        QJsonArray valueArray = json[key].toArray();
        for(int i = 0; i < valueArray.size(); ++i) {
            vector.push_back(valueArray[i].toString());
        }
        return true;
    }
    return false;
}

}
