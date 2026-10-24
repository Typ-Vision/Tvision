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
#include <cmath>
using namespace visionflow;
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    ToolDefinition blob{QUuid::createUuid().toString(),"blob","Blob","",true,defaultParameters("blob")};
    auto cancel=std::make_shared<std::atomic_bool>(false);
    auto run=[&](const QImage& image,const std::optional<ToolRoi>& roi=std::nullopt,const std::optional<QRect>& upstream=std::nullopt){auto frame=ImageFrame::fromBuffer(image.constBits(),image.sizeInBytes(),image.width(),image.height(),image.bytesPerLine(),PixelFormat::Gray8,FrameMetadata{"source","frame","cycle","image"});
        const auto hash=frame.contentId();QMap<QString,PortValue> input{{"image",{inputPorts("blob").first(),frame,true,"frame","cycle"}}};
        if(upstream)input["region"]={inputPorts("blob")[1],*upstream,true,"frame","cycle"};
        auto r=ToolExecutors::get("blob",{},roi)(blob,input,{},"frame","cycle",cancel);check(hash==frame.contentId(),"mask leaves raw pixels intact");return r;};
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
    // Every edge/corner, both polarities and component connectivity modes.
    for(const auto polarity:{"bright","dark"})for(int connectivity:{4,8}){
        blob.parameters=defaultParameters("blob");blob.parameters["polarity"]=polarity;blob.parameters["connectivity"]=connectivity;
        blob.parameters["searchRoi"]=MatchService::rectangle({2,2,8,6});
        const bool dark=QString(polarity)=="dark";
        for(const auto edge:{QPoint(2,4),QPoint(9,4),QPoint(5,2),QPoint(5,7),QPoint(2,2),QPoint(9,2),QPoint(2,7),QPoint(9,7)}){
            image.fill(dark ? 255:0);image.scanLine(edge.y())[edge.x()]=dark ? 0:255;image.scanLine(4)[5]=dark ? 0:255;
            blob.parameters["rejectBorderTouching"]=false;auto off=run(image);check(number(off,"count")==2,"off keeps border and interior");
            blob.parameters["rejectBorderTouching"]=true;r=run(image);check(number(r,"candidateCount")==2 && number(r,"count")==1 && number(r,"area")==1 && std::get<QPointF>(r.outputs["point"].data)==QPointF(5,4),"all edges/corners rejected, interior retained");
        }
        image.fill(dark ? 0:255);r=run(image);check(number(r,"candidateCount")==1 && number(r,"count")==0 && number(r,"area")==0 && !std::get<bool>(r.outputs["found"].data) && !r.outputs["point"].valid && !r.outputs["region"].valid && r.regions.isEmpty(),"full domain rejected and outputs invalid");for(auto pixel:r.mask.pixels)check(!pixel,"all rejected clears mask");
    }
    blob.parameters=defaultParameters("blob");blob.parameters["rejectBorderTouching"]=true;
    image.fill(0);for(int y=3;y<=5;++y)for(int x=4;x<=6;++x)if(x!=5 || y!=4)image.scanLine(y)[x]=255;
    r=run(image);check(number(r,"area")==8 && !r.mask.pixels[4*12+5],"interior hollow object retained, hole is not domain boundary");
    image.scanLine(0)[0]=255;image.scanLine(1)[1]=255;blob.parameters["connectivity"]=4;r=run(image);check(number(r,"count")==2,"4-connectivity keeps diagonal interior singleton");blob.parameters["connectivity"]=8;r=run(image);check(number(r,"count")==1 && number(r,"area")==8,"8-connectivity rejects whole diagonally joined border object");
    blob.parameters["minArea"]=9;r=run(image);check(number(r,"candidateCount")==2 && number(r,"count")==0,"area and border filters combine without changing candidates");
    blob.parameters["minArea"]=1;blob.parameters["searchRoi"]=MatchService::rectangle({0,0,12,10});
    image.fill(0);image.scanLine(2)[4]=255;image.scanLine(4)[5]=255;r=run(image,std::nullopt,QRect(2,2,8,6));check(number(r,"count")==1 && std::get<QPointF>(r.outputs["point"].data)==QPointF(5,4),"upstream effective region overrides stored search ROI");
    // A diamond: |x-10|+|y-10|<=8. The sloped edge pixel (14,13)
    // is strictly inside the bounding box, but its diagonal neighbour is outside.
    QImage diamond(21,21,QImage::Format_Grayscale8);diamond.fill(0);
    diamond.scanLine(13)[14]=255;diamond.scanLine(12)[14]=255;diamond.scanLine(3)[3]=255;
    for(int y=9;y<=11;++y)for(int x=9;x<=11;++x)if(x!=10 || y!=10)diamond.scanLine(y)[x]=255;
    const ToolRoi rotated{{"image"},RotatedRectangle{{10,10},{8*std::sqrt(2.),8*std::sqrt(2.)},45}};
    blob.parameters["searchRoi"]=QJsonObject{};
    for(int connectivity:{4,8}){blob.parameters["connectivity"]=connectivity;
    r=run(diamond,rotated);check(number(r,"candidateCount")==2 && number(r,"count")==1 && number(r,"area")==8 && std::get<QPointF>(r.outputs["point"].data)==QPointF(10,10),"rotated mask boundary rejects whole component away from bbox");}
    blob.parameters["connectivity"]=8;
    blob.parameters["rejectBorderTouching"]=false;r=run(diamond,rotated);check(number(r,"count")==2,"off preserves old rotated segmentation");
    const auto explicitOff=r;blob.parameters.remove("rejectBorderTouching");r=run(diamond,rotated);
    check(r.mask.pixels==explicitOff.mask.pixels && r.regions==explicitOff.regions && r.points==explicitOff.points && number(r,"area")==number(explicitOff,"area") && number(r,"count")==number(explicitOff,"count"),"absent field equals explicit off results");
    blob.parameters=defaultParameters("blob");blob.parameters["rejectBorderTouching"]=true;blob.parameters["searchRoi"]=MatchService::rectangle({-1,0,12,10});bool outsideRejected=false;try{run(image);}catch(...){outsideRejected=true;}check(outsideRejected,"invalid ROI remains rejected without clipping");
    blob.parameters=defaultParameters("blob");
    QImage many(130,130,QImage::Format_Grayscale8);many.fill(0);for(int y=0;y<130;y+=2)for(int x=0;x<130;x+=2)many.scanLine(y)[x]=255;r=run(many);check(number(r,"count")==4225 && r.regions.size()==2048 && r.message.contains("2048"),"bbox cap does not change count");int pixels=0;for(auto p:r.mask.pixels)if(p)++pixels;check(pixels==4225,"all accepted pixels shown beyond bbox cap");
    for(const auto key:{"minArea","maxArea"})for(const auto value:{QJsonValue(-1),QJsonValue(0),QJsonValue(1.5),QJsonValue("10")}){auto bad=blob;bad.parameters[key]=value;bool rejected=false;try{ToolRegistry::validate(bad);}catch(...){rejected=true;}check(rejected,"invalid area rejected");}
    for(auto value:{QJsonValue(1),QJsonValue("true"),QJsonValue(QJsonValue::Null)}){auto invalid=blob;invalid.parameters["rejectBorderTouching"]=value;bool failed=false;try{ToolRegistry::validate(invalid);}catch(...){failed=true;}check(failed,"border filter requires boolean");}
    auto bad=blob;bad.parameters["minArea"]=10;bad.parameters["maxArea"]=9;bool rejected=false;try{ToolRegistry::validate(bad);}catch(...){rejected=true;}check(rejected,"inverted bounds rejected");
    cancel->store(true);r=run(many);check(r.mask.pixels.isEmpty() && r.outputs.isEmpty(),"cancelled executor emits no mask/results");cancel->store(false);
    CycleResult cycle;cycle.tools.append(run(many));EditorSession::suppressCancelled(cycle);check(cycle.tools[0].mask.pixels.isEmpty(),"Stop clears mask");
    QTemporaryDir dir(QDir(app.applicationDirPath()).filePath("blob-XXXXXX"));check(dir.isValid(),"isolated project fixture");ProjectService project;project.createProject("Blob","",dir.filePath("blob.vfproject"),"file");auto recipe=project.createRecipe("Blob","",ApplicationType::DefectInspection,"file");auto id=project.addTool("blob");project.save();
    QFile file(project.path());check(file.open(QIODevice::ReadOnly),"read legacy fixture");auto root=QJsonDocument::fromJson(file.readAll()).object();file.close();auto recipes=root["recipes"].toArray();auto rec=recipes[0].toObject();auto def=rec["definition"].toObject();auto editor=def["editor"].toObject();editor["schemaVersion"]=5;auto tools=editor["tools"].toArray();auto tool=tools[0].toObject();auto params=tool["parameters"].toObject();for(auto key:{"minArea","maxArea","polarity","connectivity","rejectBorderTouching"})params.remove(key);tool["parameters"]=params;tools[0]=tool;editor["tools"]=tools;def["editor"]=editor;rec["definition"]=def;recipes[0]=rec;root["recipes"]=recipes;check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"write legacy fixture");const auto legacyBytes=QJsonDocument(root).toJson();file.write(legacyBytes);file.close();
    project.openProject(project.path());project.selectRecipe(recipe);check(!project.dirty() && project.tools()[0].parameters==defaultParameters("blob"),"legacy default normalization is clean");const auto version=project.activeRecipe()->recipeVersion;project.updateTool(project.tools()[0]);project.save();check(!project.dirty() && project.activeRecipe()->recipeVersion==version && project.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==5,"no-op keeps old schema and version");
    check(file.open(QIODevice::ReadOnly),"read no-op legacy");check(file.readAll()==legacyBytes,"legacy no-op exact bytes/hash");file.close();
    auto updated=project.tools()[0];updated.parameters["rejectBorderTouching"]=true;updated.parameters["minArea"]=12;updated.parameters["maxArea"]=200;updated.parameters["polarity"]="dark";updated.parameters["connectivity"]=4;project.updateTool(updated);project.save();project.openProject(project.path());project.selectRecipe(recipe);check(project.tools()[0].id==id && project.tools()[0].parameters==updated.parameters && project.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==6,"first change writes schema6 and roundtrips parameters");
    // Missing optional field is normalized for all previously supported codecs.
    for(int schema:{6,7,8,9}){
        check(file.open(QIODevice::ReadOnly),"read codec fixture");root=QJsonDocument::fromJson(file.readAll()).object();file.close();
        recipes=root["recipes"].toArray();rec=recipes[0].toObject();def=rec["definition"].toObject();editor=def["editor"].toObject();editor["schemaVersion"]=schema;tools=editor["tools"].toArray();tool=tools[0].toObject();params=tool["parameters"].toObject();params.remove("rejectBorderTouching");tool["parameters"]=params;if(schema>=7)tool["dependencies"]=QJsonArray{};else tool.remove("dependencies");tools[0]=tool;editor["tools"]=tools;def["editor"]=editor;rec["definition"]=def;recipes[0]=rec;root["recipes"]=recipes;
        const auto bytes=QJsonDocument(root).toJson();check(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"write codec fixture");file.write(bytes);file.close();
        project.openProject(project.path());project.selectRecipe(recipe);check(!project.tools()[0].parameters["rejectBorderTouching"].toBool() && !project.dirty(),"legacy codec reads false without dirty");project.updateTool(project.tools()[0]);project.save();check(file.open(QIODevice::ReadOnly),"read codec no-op");check(file.readAll()==bytes,"all old codec no-op bytes/hash preserved");file.close();
    }
    def=project.activeRecipe()->definition;editor=def["editor"].toObject();editor["schemaVersion"]=10;def["editor"]=editor;rejected=false;try{editorTools(def);}catch(...){rejected=true;}check(rejected,"future schema rejected");
    QTextStream(stdout)<<"Blob contract passed\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<"\n";return 1;}}
