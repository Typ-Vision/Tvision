#include "application/CommunicationStationComposition.hpp"
#include "application/ProjectService.hpp"
#include "persistence/StationIdentityStore.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <QTextStream>
#include <QTimer>
#include <QTcpServer>
#include <QJsonArray>
#include <QSaveFile>
using namespace visionflow;
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    using C=CommunicationV4Codec;int checks=0;auto check=[&](bool b,const char* reason){++checks;if(!b)throw std::runtime_error(reason);};auto uuid=[] {return QUuid::createUuid().toString(QUuid::WithoutBraces);};
    check(argc==2,"Fixture directory argument");QDir().mkpath(QString::fromLocal8Bit(argv[1]));QTemporaryDir root(QString::fromLocal8Bit(argv[1])+"/machine-XXXXXX");root.setAutoRemove(false);check(root.isValid(),"Fixture root");
    JsonStationIdentityStore identities(root.filePath("communication-identities-v4.json"));StationIdentityRegistry registry(identities);registry.initialize(QUuid::createUuid());const auto peer=QUuid::createUuid(),epoch=QUuid::createUuid();registry.enrollEpoch(identities.load().revision,peer,epoch);
    auto enroll=[&](CatalogueCategory kind,QUuid entity,QUuid revision=QUuid{}){return registry.bind(identities.load().revision,{kind,entity,revision,{}},"Fixture");};
    auto publication=PublicationTemplate::create(QUuid::createUuid(),QUuid::createUuid(),{QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid(),QUuid::createUuid()});publication.numbers[15].required=true;publication.flags[31].required=true;const auto tk=enroll(CatalogueCategory::TemplateRevision,publication.id,publication.revision).key;
    QDir().mkpath(root.filePath("publication-templates-v4"));QFile templateFile(root.filePath("publication-templates-v4/"+publication.revision.toString(QUuid::WithoutBraces)+".json"));check(templateFile.open(QIODevice::WriteOnly),"Template manifest");templateFile.write(QJsonDocument(publication.toJson()).toJson());templateFile.close();
    QVector<std::shared_ptr<const StationSnapshot>> snapshots;
    {
        StationRuntimeBoundary boundary;FileStationActivationStore store(root.filePath("communication-stations-v4.json"));StationActivationService activation(store,identities,boundary);activation.initialize();std::vector<std::unique_ptr<RuntimeService>> runtimes;std::vector<std::unique_ptr<RuntimeCoordinator>> coordinators;
        for(int n=0;n<4;++n){
            const auto station=QUuid::createUuid(),camera=QUuid::createUuid(),revision=QUuid::createUuid();const auto se=enroll(CatalogueCategory::Station,station),ce=enroll(CatalogueCategory::Camera,camera);
            ProjectService project;project.createProject("Composition","",root.filePath(QString("p%1.vfproject").arg(n)),"file");project.createRecipe(QString("Recipe %1").arg(n),"",ApplicationType(n%3),"file");QImage image(32,32,QImage::Format_Grayscale8);image.fill(255);const auto imagePath=root.filePath(QString("image%1.png").arg(n));check(image.save(imagePath),"Acquisition raster");const auto source=project.setImageFile(imagePath);
            VariableDefinition number;number.name="Value";number.type=VariableType::Int32;const auto numberId=project.addVariable(number);VariableDefinition flag;flag.name="Flag";flag.type=VariableType::Boolean;const auto flagId=project.addVariable(flag);const auto setter=project.addTool("set_variables");
            ValueEnvelope nv;nv.type=ScalarType::Int32;nv.validity=ValueValidity::Valid;nv.value=qint32(40+n);ValueEnvelope fv;fv.type=ScalarType::Boolean;fv.validity=ValueValidity::Valid;fv.value=false;
            for(auto tool:project.tools())if(tool.id==setter){tool.parameters=encodeAssignmentConfig({setter,1,{{numberId,ConstantAssignmentSource{nv}},{flagId,ConstantAssignmentSource{fv}}}});project.updateTool(tool);}project.save();
            auto recipe=*project.activeRecipe();recipe.definition=withRecipePublicationAssignments(recipe.definition,{publication.id,publication.revision,{{PublicationSlotKind::Number,15,{PublicationSourceKind::Variable,numberId,{},0}},{PublicationSlotKind::Flag,31,{PublicationSourceKind::Variable,flagId,{},0}}}});const auto re=enroll(CatalogueCategory::RecipeRevision,QUuid(recipe.id),revision);
            runtimes.push_back(std::make_unique<RuntimeService>(root.filePath(QString("setup%1").arg(n)),root.filePath("calibrations.json"),RuntimeIdentity::localEngineer()));coordinators.push_back(std::make_unique<RuntimeCoordinator>(*runtimes.back()));boundary.bind(station,*runtimes.back(),*coordinators.back());
            snapshots.append(activation.activate({station,se.key,QString("S%1").arg(n)},{},{revision,re.key,recipe,root.path(),{{source,camera,ce.key}}}));
        }
    }
    CommunicationServerProfiles profiles(root.filePath("communication-server-v4.json"));CommunicationServerProfile p;profiles.save(p);
    CommunicationPeerSessionService sessions(identities);CommunicationStationComposition machine(root.path(),{},identities,sessions);CommunicationServerLifecycle server(profiles,root.filePath("shutdown.json"),&sessions);CommunicationWorkspace workspace(server,profiles,identities);machine.attach(server,workspace);
    check(workspace.recipes().size()==4 && workspace.stations().size()==4,qPrintable(workspace.listener()));for(const auto& row:workspace.stations())check(row.runtime.mode==StationMode::Stopped && !row.automatic,"Restore never grants Run/Auto");

    workspace.setRemoteEnabled(true);
    QTcpServer listener;check(listener.listen(QHostAddress::LocalHost,0),"Fixture listen");
    QObject::connect(&listener,&QTcpServer::newConnection,&app,[&]{
        while(listener.hasPendingConnections()){
            auto socket=listener.nextPendingConnection();
            QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            attachCommunicationPeer(*socket,sessions);
        }
    });
    QJsonArray stationRows;
    for(const auto& s:snapshots)stationRows.append(QJsonObject{{"station",int(s->station.key)},{"recipe",int(s->selection.recipeKey)},{"template",int(tk)},{"activation",s->activation.toString(QUuid::WithoutBraces)},{"id",s->station.id.toString(QUuid::WithoutBraces)}});
    auto save=[&](QString name,QJsonObject object){QSaveFile f(QString::fromLocal8Bit(argv[1])+"/"+name);check(f.open(QIODevice::WriteOnly),"Fixture report open");f.write(QJsonDocument(object).toJson());check(f.commit(),"Fixture report commit");};
    save("ready.json",{{"port",int(listener.serverPort())},{"peer",peer.toString(QUuid::WithoutBraces)},{"epoch",epoch.toString(QUuid::WithoutBraces)},{"stations",stationRows},{"root",root.path()},{"physical","BLOCKED"}});
    QTimer timer;timer.setInterval(2);QObject::connect(&timer,&QTimer::timeout,&app,[&]{
        const QDir commands(QString::fromLocal8Bit(argv[1]));
        if(QFile::exists(commands.filePath("local-stop"))){workspace.command(snapshots[0]->station.id,{},"stop");QFile::remove(commands.filePath("local-stop"));save("local-stop-done.json",{{"stopped",true}});}
        if(!QFile::exists(commands.filePath("pause-poll")))workspace.poll();
        if(QFile::exists(commands.filePath("quit"))){
            for(const auto& s:snapshots)workspace.command(s->station.id,{},"stop");workspace.poll();
            server.beginShutdown(0);server.pollShutdown();
            save("server-result.json",{{"checks",checks},{"physical","BLOCKED"},{"fixtureOnly",true}});app.quit();
        }
    });timer.start();QTimer::singleShot(90000,&app,[&]{app.exit(5);});return app.exec();
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
