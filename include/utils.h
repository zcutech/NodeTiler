//
// Created by Charlie Zhong on 2021/9/29.
//

#ifndef NODETILER_UTILS_H
#define NODETILER_UTILS_H

#include <QtCore/QString>
#include <QApplication>

void loadStyleSheet(QApplication *app, const QString& fileName);
void loadStyleSheets(QApplication *app, std::initializer_list<const QString> fileNames);


#endif //NODETILER_UTILS_H
