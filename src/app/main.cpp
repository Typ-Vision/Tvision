#include <QApplication>
#include "MainWindow.hpp"
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QTextStream>
#include <QTimer>
#include <exception>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("VisionFlow"); app.setApplicationName("VisionFlow"); app.setStyle("Fusion");
    // The offscreen Windows platform may not enumerate system fonts.
    if (app.arguments().contains("--smoke-test"))
        QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));
    QFile theme(":/resources/theme.qss");
    if (!theme.open(QIODevice::ReadOnly)) return 2;
    app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    const bool smoke = app.arguments().contains("--smoke-test");
    const QDir buildDir(QCoreApplication::applicationDirPath());
    visionflow::MainWindow window(buildDir.filePath(smoke ? "ui-smoke.ini" : "../config/local.ui.ini"));
    window.show();
    if (smoke) QTimer::singleShot(100, &app, [&]{
        window.resize(1400,900);
        app.processEvents();
        const bool shellValid = window.verifyShell();
        window.grab().save(buildDir.filePath("step1-ui.png"));
        bool workspaceValid=false;
        try { workspaceValid=window.verifyWorkspace(buildDir.absolutePath()); }
        catch (const std::exception& error) { QTextStream(stderr) << error.what() << '\n'; }
        bool editorValid=false;
        if (workspaceValid) try {editorValid=window.verifyEditor(buildDir.absolutePath());}
        catch (const std::exception& error) {QTextStream(stderr)<<error.what()<<'\n';}
        bool matchingValid=false;
        if (editorValid) try {matchingValid=window.verifyMatching(buildDir.absolutePath());}
        catch (const std::exception& error) {QTextStream(stderr)<<error.what()<<'\n';}
        bool executionValid=false;
        if(matchingValid)try {executionValid=window.verifyExecution(buildDir.absolutePath());}
        catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        bool cameraValid=false;
        if(executionValid)try{cameraValid=window.verifyCamera(buildDir.absolutePath());}
        catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        bool webcamValid=false;
        if(cameraValid)try{webcamValid=window.verifyWebcam(buildDir.absolutePath());}
        catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        bool measurementValid=false;
        if(webcamValid)try{measurementValid=window.verifyMeasurement(buildDir.absolutePath());}
        catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        bool runtimeValid=false;
        if(measurementValid)try{runtimeValid=window.verifyRuntime(buildDir.absolutePath());}
        catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        const bool valid=shellValid && workspaceValid && editorValid && matchingValid && executionValid && cameraValid && webcamValid && measurementValid && runtimeValid && window.verifySession();
        window.close();
        QTextStream(stdout) << (valid ? "PASS" : "FAIL") << ": workspace, session restore, image viewer, ROI teaching, matching and persistence\n";
        app.exit(valid ? 0 : 1);
    });
    return app.exec();
}
