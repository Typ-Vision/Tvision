#include "application/ProjectService.hpp"
#include "domain/ToolRegistry.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QMap>
#include <QUuid>
using namespace visionflow;
namespace {
int checks=0;
void check(bool b,const char* s){++checks;if(!b)throw std::runtime_error(s);}
template<class F>void reject(F f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"Expected rejection");}
QByteArray bytes(const QString& p){QFile f(p);check(f.open(QIODevice::ReadOnly),"Read saved project");return f.readAll();}
ToolDefinition tool(ProjectService& p,const QString& id){for(const auto& t:p.tools())if(t.id==id)return t;throw std::runtime_error("Missing tool");}
void capabilityMatrix(){
    // Independent acceptance table: do not derive expected capabilities from the registry.
    const QMap<QString,QStringList> expected{
        {"template_match",{"point"}},{"pattern_pose",{"pose"}},{"shape_pose",{"pose"}},
        {"edge",{"point"}},{"edge_point",{"point"}},{"edge_pair",{"first","second","centre"}},
        {"circle_fit",{"centrePx"}},{"line_fit",{"originPx"}},{"blob",{"point"}}};
    const QStringList consumers{"edge","edge_point","edge_pair","line_fit","circle_fit","blob","code_read"};
    for(const auto& entry:ToolRegistry::entries()){
        check(supportsReference(entry.kind)==consumers.contains(entry.kind),"Exact consumer capability table");
        for(const auto& port:entry.outputs)
            check(referenceSourcePort(entry.kind,port.id)==expected.value(entry.kind).contains(port.id),"Exact source capability table including forbidden ports");
    }
    int pairs=0;
    for(auto it=expected.cbegin();it!=expected.cend();++it)for(const auto& port:it.value())for(const auto& kind:consumers){
        ToolDefinition image{"image","image_file","Image","",true,defaultParameters("image_file")};
        ToolDefinition source{"source",it.key(),"Source","",true,defaultParameters(it.key())};
        ToolDefinition target{"target",kind,"Target","",true,defaultParameters(kind)};
        image.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        source.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        target.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        const CoordinateFrame raw{image.id+"/image"};
        if(kind=="edge" || kind=="blob" || kind=="code_read")target.parameters["searchRoi"]=QJsonObject{{"x",70},{"y",70},{"width",20},{"height",20}};
        else if(kind=="line_fit")target.parameters["scanRoi"]=ToolRoi{raw,RotatedRectangle{{80,80},{20,20},0}}.toJson();
        else if(kind=="circle_fit")target.parameters["scanRoi"]=ToolRoi{raw,Annulus{{80,80},5,15}}.toJson();
        else target.parameters["scanRoi"]=ToolRoi{raw,ScanSegment{{80,70},{80,90},2}}.toJson();
        ReferenceBinding b;b.sourceTool=source.id;b.sourcePort=port;b.geometryId=referenceGeometryIdentity(source,port);
        b.imageSource=image.id;b.rawSize={256,256};b.nominal=referenceNominalRoi(target,raw);
        if(port=="pose"){b.mode=ReferenceMode::Rigid;b.taught=Pose2D{{40,40},0,raw};}
        else b.taught=Point2D{{40,40},raw};
        target.parameters["referenceBinding"]=b.toJson();
        const QVector<ToolDefinition> tools{image,source,target};
        const QVector<ToolConnection> links{{image.id,"image",source.id,"image"},{image.id,"image",target.id,"image"},{source.id,port,target.id,referenceInput(b)}};
        check(validateGraph(tools,links).valid(),"Every advertised source-port/consumer pair has a valid typed graph");++pairs;
        const auto definition=withEditorConnections(withEditorTools({},tools),links);
        check(editorConnections(definition)==links && toolReferenceBinding(editorTools(definition).last()).toJson()==b.toJson(),"Matrix pair strict editor codec roundtrip");
        auto badTools=tools;badTools[1].enabled=false;check(!validateGraph(badTools,links).valid(),"Disabled source rejected for every pair");
        auto badLinks=links;badLinks.last().targetPort=port=="pose" ? "reference":"poseReference";
        check(!validateGraph(tools,badLinks).valid(),"Point/Pose input swap rejected for every pair");
        badLinks=links;badLinks.last().sourcePort="missing";check(!validateGraph(tools,badLinks).valid(),"Missing source port rejected for every pair");
        badLinks=links;badLinks.last().sourceTool=target.id;check(!validateGraph(tools,badLinks).valid(),"Self reference rejected for every pair");
    }
    check(pairs==77,"Complete eleven source-port by seven consumer capability matrix");
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    capabilityMatrix();
    QTemporaryDir dir(QDir::currentPath()+"/reference-graph-XXXXXX");
    for(const QString k:{"edge","edge_point","edge_pair","line_fit","circle_fit","blob","code_read"}) {
        check(supportsReference(k),"Consumer capability");
        const auto inputs=inputPorts(k);for(const QString port:{"reference","poseReference"})check(std::any_of(inputs.begin(),inputs.end(),[&](const auto& p){return p.id==port && !p.required;}),"Typed reference inputs");
    }
    for(const auto& source:ToolRegistry::entries())for(const auto& port:source.outputs)if(referenceSourcePort(source.kind,port.id))check((port.type==PortType::Point || port.type==PortType::Pose2D) && port.unit=="px" && port.frame=="image","Source capability only real raw point/pose");
    check(!referenceSourcePort("line_fit","tangentPx") && !referenceSourcePort("circle_fit","centreMm") && !referenceSourcePort("code_read","codes"),"No implicit point conversion");
    for(auto type:{ApplicationType::Measurement,ApplicationType::DefectInspection,ApplicationType::RobotGuidance}) {
        ProjectService p;p.createProject("Reference","",dir.filePath(QString::number(int(type))+".vfproject"),"file");p.createRecipe("Part","",type,"file");
        const auto camera=p.addTool("image_file"),source=p.addTool("blob"),target=p.addTool("edge_point");
        const auto legacyId=p.addTool("edge");auto legacy=tool(p,legacyId);legacy.parameters["searchRoi"]=QJsonObject{{"x",60},{"y",40},{"width",20},{"height",20}};legacy.parameters["referenceOrigin"]=QJsonObject{{"x",40},{"y",40}};legacy.parameters["referenceGeometry"]=referenceGeometry(tool(p,source));p.updateTool(legacy);
        auto legacyLinks=editorConnections(p.activeRecipe()->definition);legacyLinks.append({source,"point",legacyId,"reference"});p.setConnections(legacyLinks);
        auto t=tool(p,target);t.parameters["scanRoi"]=ToolRoi{{camera+"/image"},ScanSegment{{60,50},{60,70},2}}.toJson();p.updateTool(t);p.save();
        const auto recipe=p.activeRecipe()->id;const auto old=bytes(p.path());p.openProject(p.path());p.selectRecipe(recipe);p.save();check(bytes(p.path())==old,"Legacy no-op bytes");
        ReferenceBinding b;b.sourceTool=source;b.sourcePort="point";b.geometryId=referenceGeometryIdentity(tool(p,source),"point");b.imageSource=camera;b.rawSize={256,256};b.nominal=referenceNominalRoi(tool(p,target),{camera+"/image"});b.taught=Point2D{{40,40},{camera+"/image"}};
        p.bindReference(target,b);check(p.activeRecipe()->definition["editor"].toObject()["schemaVersion"]==8 && p.dirty(),"Opt-in schema 8");
        const auto configured=p.activeRecipe()->definition;check(validateGraph(p.tools(),editorConnections(configured)).valid(),"Atomic binding and graph");
        auto bad=b;bad.sourceTool=target;bad.geometryId=referenceGeometryIdentity(tool(p,target),"point");reject([&]{p.bindReference(target,bad);});check(p.activeRecipe()->definition==configured,"Self edit atomic");
        auto links=editorConnections(configured);links.append({source,"region",target,"region"});reject([&]{p.updateToolConnections(tool(p,target),links);});
        auto renamed=tool(p,source);renamed.name="Renamed";p.updateTool(renamed);check(referenceGeometryIdentity(renamed,"point")==b.geometryId,"Rename preserves identity");
        renamed.parameters["threshold"]=140;p.updateTool(renamed);check(referenceGeometryIdentity(renamed,"point")==b.geometryId,"Threshold preserves teaching");
        p.moveTool(target,-1);check(validateGraph(p.tools(),editorConnections(p.activeRecipe()->definition)).valid(),"Flow order independent");
        p.save();const auto saved=bytes(p.path());p.openProject(p.path());p.selectRecipe(recipe);p.save();check(bytes(p.path())==saved,"New binding no-op bytes");
        const auto id=p.activeRecipe()->id;p.selectRecipe(id);p.clearReference(target);check(!tool(p,target).parameters.contains("referenceBinding") && tool(p,target).parameters["scanRoi"]==t.parameters["scanRoi"],"Clear keeps nominal");
        p.bindReference(target,b);p.removeTool(source);check(!validateGraph(p.tools(),editorConnections(p.activeRecipe()->definition)).valid() && tool(p,target).parameters.contains("referenceBinding"),"Deleted source stays visibly invalid");
    }
    QTextStream(stdout)<<checks<<" reference graph checks\n";return 0;
}catch(const std::exception& e){QTextStream(stderr)<<e.what()<<'\n';return 1;}}
