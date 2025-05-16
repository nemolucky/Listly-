#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "authmanager.h"
#include "thememanager.h"
#include "listmanager.h"
#include "fontmanager.h"
#include "database.h"
#include "goalmanager.h"
#include "aimanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    qmlRegisterSingletonType<AuthManager>("com.company", 1, 0, "AuthManager", AuthManager::create);
    qmlRegisterSingletonType<ThemeManager>("com.company", 1, 0, "ThemeManager",
                                           [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
                                               return new ThemeManager();
                                           });
    qmlRegisterSingletonType<ListManager>("com.company", 1, 0, "ListManager",
                                          [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
                                              return new ListManager();
                                          });
    qmlRegisterSingletonType<FontManager>("com.company", 1, 0, "FontManager",
                                          [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
                                              return new FontManager();
                                          });
    qmlRegisterSingletonType<Database>("com.company", 1, 0, "Database",
                                       [](QQmlEngine*, QJSEngine*) -> QObject* {
                                            return new Database;
                                        });
    qmlRegisterSingletonType<GoalManager>("com.company", 1, 0, "GoalManager",
                                          [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
                                              return new GoalManager();
                                          });
    qmlRegisterSingletonType<AIManager>("com.company", 1, 0, "AIManager",
                                          [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
                                              return new AIManager();
                                          });

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("Listly", "Main");

    return app.exec();
}
