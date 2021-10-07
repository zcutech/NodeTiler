//
// Created by Charlie Zhong on 2021/9/29.
//

#include "utils.h"

#include <QtCore/QFile>


void loadStyleSheet(QApplication *app, const QString& fileName)
{
    QFile sheetFile(fileName);
    sheetFile.open(QFile::ReadOnly | QFile::Text);
    app->setStyleSheet(sheetFile.readAll());
    sheetFile.close();
}

void loadStyleSheets(QApplication *app, std::initializer_list<const QString> fileNames)
{
    QString res;
    for (const auto& name : fileNames) {
        QFile sheetFile(name);
        sheetFile.open(QFile::ReadOnly | QFile::Text);
        res += sheetFile.readAll();
    }
    app->setStyleSheet(res);
}