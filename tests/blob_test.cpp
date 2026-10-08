#include "application/ToolExecutors.hpp"
#include "application/EditorSession.hpp"
#include "application/MatchService.hpp"
#include "application/ProjectService.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QUuid>
#include <QTextStream>
#include <stdexcept>
using namespace visionflow;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    ToolDefinition blob{QUuid::createUuid().toString(),"blob","Blob","",true,defaultParameters("blob")};
    auto cancel=std::make_shared<std::atomic_bool>(false);
    auto run=[&](const QImage& image){auto frame=ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),image.width(),image.height(),image.bytesPerLine(),PixelFormat::Gray8);
        const auto hash=frame.contentId();QMap<QString,PortValue> input{{"image",{inputPorts("blob").first(),frame,true,"frame","cycle"}}};
        auto r=ToolExecutors::get("blob")(blob,input,{},"frame","cycle",cancel);check(hash==frame.contentId(),"mask leaves raw pixels intact");return r;};
    auto number=[](const ToolResult& r,const QString& key){return std::get<double>(r.outputs[key].data);};
    QImage image(12,10,QImage::Format_Grayscale8);image.fill(0);
    // Hollow 5x5 ring, plus rejected singleton, inside offset ROI.
    for(int y=3;y<8;++y)for(int x=4;x<9;++x)if(x==4 || x==8 || y==3 || y==7)image.scanLine(y)[x]=200;
    image.scanLine(2)[2]=200;blob.parameters["searchRoi"]=MatchService::rectangle({2,2,8,6});blob.parameters["minArea"]=16;blob.parameters["maxArea"]=16;
    auto r=run(image);check(number(r,"candidateCount")==2 && number(r,"count")==1 && number(r,"area")==16,"inclusive area uses pixel count, not bbox or contour");
    check(std::get<QRect>(r.outputs["region"].data)==QRect(4,3,5,5) && std::get<QPointF>(r.outputs["point"].data)==QPointF(6,5),"ROI offset bbox and centroid");
    check(r.mask.origin==QPoint(2,2) && r.mask.size==QSize(8,6) && !r.mask.pixels[0] && !r.mask.pixels[3*8+4] && r.mask.pixels[1*8+2],"mask keeps holes and excludes rejected component");
    auto owned=r.mask.pixels;image.fill(0);check(owned==r.mask.pixels,"mask owns storage");
    blob.parameters["minArea"]=17;blob.parameters["maxArea"]=QJsonValue::Null;r=run(image);check(number(r,"count")==0 && number(r,"area")==0 && !std::get<bool>(r.outputs["found"].data) && !r.outputs["region"].valid && !r.outputs["point"].valid,"empty outputs are invalid without part FAIL");
    blob.parameters=defaultParameters("blob");image.fill(128);r=run(image);check(number(r,"count")==0,"bright threshold equality excluded");blob.parameters["polarity"]="dark";r=run(image);check(number(r,"area")==120,"dark threshold equality included");
    image.fill(0);image.scanLine(1)[1]=255;image.scanLine(2)[2]=255;blob.parameters["polarity"]="bright";blob.parameters["connectivity"]=4;r=run(image);check(number(r,"count")==2 && std::get<QPointF>(r.outputs["point"].data)==QPointF(1,1),"4 connectivity and raster tie break");blob.parameters["connectivity"]=8;r=run(image);check(number(r,"count")==1 && number(r,"area")==2,"8 diagonal joins");
    QImage many(130,130,QImage::Format_Grayscale8);many.fill(0);for(int y=0;y<130;y+=2)for(int x=0;x<130;x+=2)many.scanLine(y)[x]=255;r=run(many);check(number(r,"count")==4225 && r.regions.size()==2048 && r.message.contains("2048"),"bbox cap does not change count");int pixels=0;for(auto p:r.mask.pixels)if(p)++pixels;check(pixels==4225,"all accepted pixels shown beyond bbox cap");
    for(const auto key:{"minArea","maxArea"})for(const auto value:{QJsonValue(-1),QJsonValue(0),QJsonValue(1.5),QJsonValue("10")}){auto bad=blob;bad.parameters[key]=value;bool rejected=false;try{ToolRegistry::validate(bad);}catch(...){rejected=true;}check(rejected,"invalid area rejected");}
    auto bad=blob;bad.parameters["minArea"]=10;bad.parameters["maxArea"]=9;bool rejected=false;try{ToolRegistry::validate(bad);}catch(...){rejected=true;}check(rejected,"inverted bounds rejected");
    cancel->store(true);r=run(many);check(r.mask.pixels.isEmpty() && r.outputs.isEmpty(),"cancelled executor emits no mask/results");cancel->store(false);
    CycleResult cycle;cycle.tools.append(run(many));EditorSession::suppressCancelled(cycle);check(cycle.tools[0].mask.pixels.isEmpty(),"Stop clears mask");
    QTemporaryDir dir(QDir(app.applicationDirPath()).filePath("blob-XXXXXX"));check(dir.isValid(),"isolated project fixture");ProjectService project;project.createProject("Blob","",dir.filePath("blob.vfproject"),"file");auto recipe=project.createRecipe("Blob","",ApplicationType::DefectInspection,"file");auto id=project.addTool("blob");project.save();
    QFile file(project.path());check(file.open(QIODevice::ReadOnly),"read legacy fixture");auto root=QJsonDocument::fromJson(file.readAll()).object();file.close();auto recipes=root["recipes"].toArray();auto rec=recipes[0].toObject();auto def=rec["definition"].toObject();auto editor=def["editor"].toObject();editor["schemaVersion"]=5;auto tools=editor["tools"].toArray();auto tool=tools[0].toObject();auto params=tool["parameters"].toObject();for(auto key:{"minArea","maxArea","polarity","connectivity"})params.remove(key);tool["parameters"]=params;tools[0]=tool;editor["tools"]=tools;def["editor"]=editor;rec["definition"]=def;recipes[0]=rec;root["recipes"]=recipes;check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"write legacy fixture");file.write(QJsonDocument(root).toJson());file.close();
    project.openProject(project.path());project.selectRecipe(recipe);check(!project.dirty() && project.tools()[0].parameters==defaultParameters("blob"),"legacy default normalization is clean");const auto version=project.activeRecipe()->recipeVersion;project.updateTool(project.tools()[0]);project.save();check(!project.dirty() && project.activeRecipe()->recipeVersion==version && project.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==5,"no-op keeps old schema and version");
    auto updated=project.tools()[0];updated.parameters["minArea"]=12;updated.parameters["maxArea"]=200;updated.parameters["polarity"]="dark";updated.parameters["connectivity"]=4;project.updateTool(updated);project.save();project.openProject(project.path());project.selectRecipe(recipe);check(project.tools()[0].id==id && project.tools()[0].parameters==updated.parameters && project.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==6,"first change writes schema6 and roundtrips parameters");
    def=project.activeRecipe()->definition;editor=def["editor"].toObject();editor["schemaVersion"]=7;def["editor"]=editor;rejected=false;try{editorTools(def);}catch(...){rejected=true;}check(rejected,"future schema rejected");
    QTextStream(stdout)<<"Blob contract passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}}
