#pragma once
#include "application/ActiveCalibrationService.hpp"
#include "application/PlaneMappingService.hpp"

namespace visionflow::test {
// Structural integration fixture, independent of raster detection. The detector
// contract separately tests real raster -> observation -> wizard -> model.
inline CalibrationModelRecord framedModel(const CalibrationModelRecord& seed,
                                          const ImageFrame& frame,double widthMm){
    CalibrationGridObservation grid;
    grid.target={"integration-framed",7,7,widthMm/4,CalibrationGridPattern::FramedCircles,FramedPlateGeometry{}};
    grid.source=frame.metadata();grid.resolution=frame.size();
    grid.pixelHash=QString::fromLatin1(frame.contentId().toHex());
    QVector<PlateCentre> centres;
    for(int r=0;r<7;++r)for(int c=0;c<7;++c){
        const QPointF p{20.+100*c,20.+20*r};
        grid.pixels.append(p);grid.worldMillimetres.append({c*widthMm/4,r*widthMm/4});
        centres.append({r,c,p,{}});
    }
    const auto raw=RawPlateObservation::create(grid,centres,
        {PlateOrientation::TriangleCorner,0,false,QPointF(5,5)},"integration-fixture/1");
    return CalibrationModelRecord::create(MeasurementService{}.teachHomography(
        seed.model().dependency(),PlaneMappingService::fit(raw)),seed.setup(),seed.image());
}
}

