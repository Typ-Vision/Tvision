#pragma once
#include "ReferenceBinding.hpp"
#include "ToolDefinition.hpp"
namespace visionflow {
bool supportsReference(const QString& kind);
bool referenceSourcePort(const QString& kind,const QString& port);
QString referenceGeometryIdentity(const ToolDefinition&,const QString& port);
ToolRoi referenceNominalRoi(const ToolDefinition&,const CoordinateFrame&);
ReferenceBinding toolReferenceBinding(const ToolDefinition&);
QString referenceInput(const ReferenceBinding&);
}
