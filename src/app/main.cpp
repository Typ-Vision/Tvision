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
    if (QApplication::platformName()=="offscreen" || app.arguments().contains("--smoke-test") || (app.arguments().contains("--circle-smoke") || app.arguments().contains("--geometry-smoke")))
        QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));
    QFile theme(":/resources/theme.qss");
    if (!theme.open(QIODevice::ReadOnly)) return 2;
    app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    const bool variablesOnly=app.arguments().contains("--variables-smoke");
    const bool roiOnly=app.arguments().contains("--roi-smoke");
    const bool multiRuntimeOnly=app.arguments().contains("--multi-source-runtime-smoke");
    const bool recipeFieldsOnly=app.arguments().contains("--recipe-fields-smoke");
    const bool multiOnly=app.arguments().contains("--multi-source-smoke");
    const bool circleOnly=app.arguments().contains("--circle-smoke");
    const bool systemOnly=app.arguments().contains("--system-calibration-smoke");
    const bool geometryOnly=app.arguments().contains("--geometry-smoke");
    const bool poseOnly=app.arguments().contains("--pose-tracking-smoke");
    const bool experimentalOnly=app.arguments().contains("--experimental-editor-smoke");
    const bool usabilityOnly=app.arguments().contains("--usability-smoke");
    const bool editorCamerasOnly=app.arguments().contains("--editor-camera-sessions-smoke");
    const bool runtimeCamerasOnly=app.arguments().contains("--runtime-camera-sessions-smoke");
    const bool responsivenessOnly=app.arguments().contains("--editor-responsiveness-probe");
    const bool restartOnly=app.arguments().contains("--restart-runtime-smoke");
    const bool failureOnly=app.arguments().contains("--runtime-failure-smoke");
    const bool visibilityOnly=app.arguments().contains("--runtime-visibility-smoke");
    const bool overlaysOnly=app.arguments().contains("--runtime-overlays-smoke");
    const bool smoke = variablesOnly || overlaysOnly || visibilityOnly || restartOnly || failureOnly || responsivenessOnly || runtimeCamerasOnly || editorCamerasOnly || roiOnly || app.arguments().contains("--smoke-test") || circleOnly || geometryOnly || poseOnly || usabilityOnly || systemOnly || multiOnly || multiRuntimeOnly || experimentalOnly || recipeFieldsOnly;
    const QDir buildDir(smoke ? qEnvironmentVariable("VISIONFLOW_TEST_ARTIFACTS",QCoreApplication::applicationDirPath()):QCoreApplication::applicationDirPath());
    if(smoke)QDir().mkpath(buildDir.absolutePath());
    visionflow::MainWindow window(buildDir.filePath(smoke ? "ui-smoke.ini" : "../config/local.ui.ini"));
    window.show();
    if (smoke) QTimer::singleShot(100, &app, [&]{
        if(variablesOnly){bool valid=false;try{valid=window.verifyVariableCatalog(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid?0:1);return;}
        if(overlaysOnly){bool valid=false;try{valid=window.verifyRuntimeOverlays(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid?0:1);return;}
        if(visibilityOnly){QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));bool valid=false;try{valid=window.verifyRuntimeVisibility(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
        if(restartOnly){bool valid=false;try{valid=window.verifyRestartRuntime(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid?0:1);return;}
        if(failureOnly){bool valid=false;try{valid=window.verifyRuntimeFailures(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid?0:1);return;}
        if(responsivenessOnly){bool valid=false;try{valid=window.verifyEditorResponsiveness(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid?0:1);return;}
        if(runtimeCamerasOnly){bool valid=false;try{valid=window.verifyRuntimeCameraSessions(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid?0:1);return;}
        if(editorCamerasOnly){bool valid=false;try{valid=window.verifyEditorCameraSessions(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid?0:1);return;}
        if(roiOnly){bool valid=false;try{valid=window.verifyRoi(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}QTextStream(stdout)<<(valid ? "PASS":"FAIL")<<": ROI draw/bind/save/reopen/Run Tool/Flow\n";window.close();app.exit(valid ? 0:1);return;}
        if(recipeFieldsOnly){QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));bool valid=false;try{window.resize(1400,900);valid=window.verifyWorkspace(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}QTextStream(stdout)<<(valid ? "PASS":"FAIL")<<": reference-free recipe dialogs, three types, validation and metadata roundtrip\n";window.close();app.exit(valid ? 0:1);return;}
        if(multiRuntimeOnly){QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));bool valid=false;try{valid=window.verifyMultiSourceRuntime(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
        if(multiOnly){QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));bool valid=false;try{valid=window.verifyMultiSource(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
        if(systemOnly){QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));bool valid=false;try{valid=window.verifySystemCalibration(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
        if(experimentalOnly){QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));bool valid=false;try{valid=window.verifyExperimentalEditor(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
        if(usabilityOnly){QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));bool valid=false;try{valid=window.verifyEditorUsability(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
        if(poseOnly){QFontDatabase::addApplicationFont(QDir(qEnvironmentVariable("WINDIR", "C:/Windows")).filePath("Fonts/segoeui.ttf"));bool valid=false;try{valid=window.verifyPoseTracking(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
        if(geometryOnly){bool valid=false;try{valid=window.verifyGeometry(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
        if(circleOnly){bool valid=false;try{valid=window.verifyCircle(buildDir.absolutePath());}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';}window.close();app.exit(valid ? 0:1);return;}
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
        bool preprocessValid=false;
        if(runtimeValid)try{preprocessValid=window.verifyPreprocessing(buildDir.absolutePath());}catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        bool codesValid=false;
        if(preprocessValid)try{codesValid=window.verifyCodeReading(buildDir.absolutePath());}catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        bool edgeValid=false;
        if(codesValid)try{edgeValid=window.verifyEdgePoint(buildDir.absolutePath());}catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        bool metrologyValid=false;
        if(edgeValid)try{metrologyValid=window.verifyMetrology(buildDir.absolutePath());}catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        bool circleValid=false;
        if(metrologyValid)try{circleValid=window.verifyCircle(buildDir.absolutePath());}catch(const std::exception& error){QTextStream(stderr)<<error.what()<<'\n';}
        const bool valid=circleValid && metrologyValid && edgeValid && codesValid && preprocessValid && shellValid && workspaceValid && editorValid && matchingValid && executionValid && cameraValid && webcamValid && measurementValid && runtimeValid && window.verifySession();
        window.close();
        QTextStream(stdout) << (valid ? "PASS" : "FAIL") << ": workspace, session restore, image viewer, ROI teaching, matching and persistence\n";
        app.exit(valid ? 0 : 1);
    });
    return app.exec();
}
