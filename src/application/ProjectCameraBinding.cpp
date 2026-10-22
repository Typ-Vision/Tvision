#include "ProjectService.hpp"
#include "MeasurementEnvironment.hpp"
#include "SystemCalibration.hpp"
#include <stdexcept>
#include <QJsonArray>
namespace visionflow {
namespace { void fail(const char* message){throw std::runtime_error(message);} }
void ProjectService::bindCameraSource(const QString& id,const MeasurementEnvironment& e,const CameraId& camera,int timeout) {
    if(!activeRecipe())fail("Recipe required.");e.current.validate();
    const auto defaults=e.defaults(camera);
    if(defaults.status!=CameraDefaultsStatus::Ready || !defaults.setup)fail("Ready unambiguous camera defaults required; open Setup.");
    auto definition=activeRecipe()->definition;bool present=false;
    auto editor=definition["editor"].toObject();auto array=editor["tools"].toArray();
    for(int i=0;i<array.size();++i)if(array[i].toObject()["id"]==id){
        present=true;auto tool=array[i].toObject();const auto kind=tool["kind"].toString();
        if(kind=="industrial_camera"){
            if(!defaults.profile)fail("Camera default profile missing.");
            const auto reference=IndustrialSourceReference::bind(e.current,*defaults.profile,timeout);
            tool["parameters"]=QJsonObject{{"sourceReference",reference.toJson()}};array[i]=tool;
        }else if(kind=="webcam"){
            for(const auto& c:e.current.cameras)if(c.id==camera && c.physical)fail("Webcam cannot bind an industrial camera.");
        }else if(kind!="image_file")fail("Acquisition source required.");
    }
    if(!present)fail("Tool not found.");editor["tools"]=array;definition["editor"]=editor;
    for(const auto& setup:e.current.setups)if(setup.id==*defaults.setup && setup.camera==camera){
        const auto binding=SystemCalibration::sourceBinding(id,SetupReference{camera,setup.id,setup.revision});
        if(editor["schemaVersion"]==9){auto bindings=definition["sourceSetups"].toObject();const auto legacy=definition.value("sourceSetup").toObject();if(!legacy.isEmpty()){bindings[legacy["sourceToolId"].toString()]=legacy;definition.remove("sourceSetup");}bindings[id]=binding;definition["sourceSetups"]=bindings;}else definition["sourceSetup"]=binding;
        definition["calibrationPolicy"]="scoped-source/1";validateEditorDefinition(definition);if(definition==activeRecipe()->definition)return;
        auto candidate=project();for(auto& recipe:candidate.recipes)if(recipe.id==activeId_){recipe.definition=definition;recipe.calibrationBinding.reset();recipe.calibrationReference.clear();recipe.status=RecipeStatus::Draft;}
        validateProject(candidate);project_=std::move(candidate);dirty_=true;changedRecipes_.insert(activeId_);return;
    }
    fail("Camera setup not found.");
}

void ProjectService::refreshCameraSources(const MeasurementEnvironment& e) {
    auto candidate=*this;
    for(const auto& tool:tools())if(tool.enabled && tool.kind=="industrial_camera" && tool.parameters["sourceReference"].isObject()){
        const auto old=IndustrialSourceReference::fromJson(tool.parameters["sourceReference"].toObject());
        const auto branch=SystemCalibration::branch(*activeRecipe(),tool.id);const auto target=SystemCalibration::setupReference(branch);
        if(!target || target->camera!=old.camera || target->setup!=old.setup || target->revision!=old.setupRevision)continue;
        const auto defaults=e.defaults(old.camera);
        if(defaults.status!=CameraDefaultsStatus::Ready || defaults.setup!=std::optional(old.setup) || defaults.profile!=std::optional(old.profile))continue;
        const auto next=IndustrialSourceReference::bind(e.current,old.profile,old.timeoutMs);
        if(next.physical!=old.physical || next.deviceRevision!=old.deviceRevision || next.setupRevision!=old.setupRevision)continue;
        candidate.bindCameraSource(tool.id,e,old.camera,old.timeoutMs);
    }
    *this=std::move(candidate);
}
}
