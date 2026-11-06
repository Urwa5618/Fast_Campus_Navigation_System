#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringList>

#include "MainWindow.h"
#include "DataLoader.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Look for the data folder next to the exe, one level up, or in the current folder
    QString base = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        base + "/data",
        base + "/../data",
        QDir::currentPath() + "/data"};
    QString dataDir;
    for (const QString &c : candidates)
    {
        if (QFileInfo::exists(c + "/locations.csv"))
        {
            dataDir = QDir::cleanPath(c);
            break;
        }
    }
    if (dataDir.isEmpty())
    {
        QMessageBox::critical(nullptr, "FAST Campus Navigation",
                              "Could not find the data folder (locations.csv / edges.csv).");
        return 1;
    }

    Graph g;
    std::string error;
    if (!loadGraph((dataDir + "/locations.csv").toStdString(),
                   (dataDir + "/edges.csv").toStdString(), g, error))
    {
        QMessageBox::critical(nullptr, "Data error", QString::fromStdString(error));
        return 1;
    }

    MainWindow window(g);
    window.show();
    return app.exec();
}