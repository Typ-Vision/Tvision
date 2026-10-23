#include "CalibrationGrid.hpp"
#include <QSet>
#include <cmath>
#include <stdexcept>
namespace visionflow {
namespace {void require(bool b){if(!b)throw std::runtime_error("Invalid/unsupported calibration grid descriptor");}}
void CalibrationGridTarget::validate() const {
    require(pattern==CalibrationGridPattern::Chessboard || pattern==CalibrationGridPattern::SymmetricCircles || pattern==CalibrationGridPattern::FramedCircles);
    require(framed.has_value()==(pattern==CalibrationGridPattern::FramedCircles));
    if(framed){framed->validate();require(columns==7 && rows==7);}
    require(!id.trimmed().isEmpty() && id.size()<=128 && columns>=3 && rows>=3 && columns<=32 && rows<=32 && columns*rows<=512 &&
        std::isfinite(squareMillimetres) && squareMillimetres>0 && squareMillimetres<=1000000);
}
QJsonObject CalibrationGridTarget::toJson() const {
    validate();const bool circles=pattern==CalibrationGridPattern::SymmetricCircles;
    if(framed){
        QJsonObject geometry{{"polarity","dark_on_light"},{"orientation","triangle_corner"}};
        auto add=[&](const char* k,std::optional<double> v){if(v)geometry[k]=*v;};
        add("circleDiameterMm",framed->circleDiameterMm);add("frameThicknessMm",framed->frameThicknessMm);
        add("innerWidthMm",framed->innerWidthMm);add("innerHeightMm",framed->innerHeightMm);
        return {{"schemaVersion",2},{"id",id},{"kind","framed_circle_grid"},{"columns",columns},{"rows",rows},
            {"spacingMm",squareMillimetres},{"origin","marked_centre_1"},{"axes","columns_rows"},{"geometry",geometry}};
    }
    return {{"schemaVersion",1},{"id",id},{"kind",circles ? "symmetric_circle_grid":"chessboard"},{"columns",columns},{"rows",rows},
        {"spacingMm",squareMillimetres},{"origin",circles ? "centre_1":"corner_1"},{"axes","columns_rows"}};
}
CalibrationGridTarget CalibrationGridTarget::fromJson(const QJsonObject& o){
    if(o["schemaVersion"]==2){
        const QSet<QString> fields{"schemaVersion","id","kind","columns","rows","spacingMm","origin","axes","geometry"};
        require(o.size()==fields.size());for(auto i=o.begin();i!=o.end();++i)require(fields.contains(i.key()));
        require(o["kind"]=="framed_circle_grid" && o["columns"]==7 && o["rows"]==7 && o["id"].isString() && o["spacingMm"].isDouble() && o["geometry"].isObject());
        const auto g=o["geometry"].toObject();const QSet<QString> allowed{"polarity","orientation","circleDiameterMm","frameThicknessMm","innerWidthMm","innerHeightMm"};
        for(auto i=g.begin();i!=g.end();++i)require(allowed.contains(i.key()));
        require(g["polarity"]=="dark_on_light" && g["orientation"]=="triangle_corner");
        FramedPlateGeometry geometry;
        auto read=[&](const char* k)->std::optional<double>{if(!g.contains(k))return {};require(g[k].isDouble());return g[k].toDouble();};
        geometry.circleDiameterMm=read("circleDiameterMm");geometry.frameThicknessMm=read("frameThicknessMm");
        geometry.innerWidthMm=read("innerWidthMm");geometry.innerHeightMm=read("innerHeightMm");
        CalibrationGridTarget t{o["id"].toString(),7,7,o["spacingMm"].toDouble(),CalibrationGridPattern::FramedCircles,geometry};
        t.validate();require(t.toJson()==o);return t;
    }
    const QSet<QString> fields{"schemaVersion","id","kind","columns","rows","spacingMm","origin","axes"};require(o.size()==fields.size());for(auto i=o.begin();i!=o.end();++i)require(fields.contains(i.key()));
    require(o["schemaVersion"].isDouble() && o["schemaVersion"].toDouble()==1 && o["id"].isString() && o["kind"].isString());
    const auto kind=o["kind"].toString();require(kind=="chessboard" || kind=="symmetric_circle_grid");
    auto integer=[&](const char* k){require(o[k].isDouble() && o[k].toDouble()==o[k].toInt() && o[k].toInt()>=3 && o[k].toInt()<=32);return o[k].toInt();};
    require(o["spacingMm"].isDouble());CalibrationGridTarget t{o["id"].toString(),integer("columns"),integer("rows"),o["spacingMm"].toDouble(),
        kind=="chessboard" ? CalibrationGridPattern::Chessboard:CalibrationGridPattern::SymmetricCircles};t.validate();require(t.toJson()==o);return t;
}
}

namespace visionflow {
void FramedPlateGeometry::validate() const {
    require(polarity==PlatePolarity::DarkOnLight && orientation==PlateOrientation::TriangleCorner);
    for(auto v:{circleDiameterMm,frameThicknessMm,innerWidthMm,innerHeightMm})if(v)require(std::isfinite(*v) && *v>0 && *v<=1000000);
}
void CalibrationGridObservation::validateRaw() const {
    target.validate();const int n=target.columns*target.rows;
    require(pixels.size()==n && worldMillimetres.size()==n);
    require(resolution.width()>0 && resolution.height()>0 && qint64(resolution.width())*resolution.height()<=16000000);
    require(!source.sourceId.isEmpty() && !source.frameId.isEmpty() && !source.cycleId.isEmpty() && source.completedAtUtc.isValid() &&
        (source.coordinateSystemId=="image" || source.coordinateSystemId==source.sourceId+"/image"));
    require(pixelHash.size()==64);for(auto c:pixelHash)require((c>='0' && c<='9') || (c>='a' && c<='f'));
    for(int i=0;i<n;++i){auto p=pixels[i];require(std::isfinite(p.x()) && std::isfinite(p.y()) && p.x()>=0 && p.y()>=0 && p.x()<=resolution.width()-1 && p.y()<=resolution.height()-1);
        require(worldMillimetres[i]==QPointF((i%target.columns)*target.squareMillimetres,(i/target.columns)*target.squareMillimetres));
        for(int j=0;j<i;++j)require(std::hypot(p.x()-pixels[j].x(),p.y()-pixels[j].y())>0.5);
    }
    double orientation=0;
    for(int r=0;r<target.rows-1;++r)for(int c=0;c<target.columns-1;++c){int i=r*target.columns+c;
        const QPointF cell[]{pixels[i],pixels[i+1],pixels[i+target.columns+1],pixels[i+target.columns]};
        for(int k=0;k<4;++k){auto a=cell[(k+1)%4]-cell[k],b=cell[(k+2)%4]-cell[(k+1)%4];double sign=a.x()*b.y()-a.y()*b.x();
            require(std::abs(sign)>1);if(orientation==0)orientation=sign;require(sign*orientation>0);}
    }
}
RawPlateObservation RawPlateObservation::create(CalibrationGridObservation grid,QVector<PlateCentre> centres,PlateOrientationEvidence direction,QString algorithm,QString space){
    grid.validateRaw();require(space=="raw_pixel_centres/1" && !algorithm.trimmed().isEmpty() && algorithm.size()<=128);
    require(centres.size()==grid.pixels.size() && direction.quarterTurns>=0 && direction.quarterTurns<4);
    const bool marked=grid.target.pattern==CalibrationGridPattern::FramedCircles;
    require(direction.kind==(marked ? PlateOrientation::TriangleCorner:PlateOrientation::Unmarked));
    require(direction.triangleRaw.has_value()==marked);
    if(!marked)require(direction.quarterTurns==0 && !direction.mirrored);
    if(direction.triangleRaw){auto p=*direction.triangleRaw;require(std::isfinite(p.x()) && std::isfinite(p.y()) && p.x()>=0 && p.y()>=0 && p.x()<grid.resolution.width() && p.y()<grid.resolution.height());}
    if(marked){
        const auto marker=*direction.triangleRaw;
        auto distance=[&](QPointF p){return std::hypot(p.x()-marker.x(),p.y()-marker.y());};
        const double originDistance=distance(grid.pixels[0]);
        for(int i:{grid.target.columns-1,(grid.target.rows-1)*grid.target.columns,int(grid.pixels.size())-1})require(distance(grid.pixels[i])>originDistance+0.5);
        const auto x=grid.pixels[1]-grid.pixels[0],y=grid.pixels[grid.target.columns]-grid.pixels[0];
        require(direction.mirrored==(x.x()*y.y()-x.y()*y.x()<0));
    }
    for(int i=0;i<centres.size();++i){const auto& p=centres[i];require(p.row==i/grid.target.columns && p.column==i%grid.target.columns && p.rawPixel==grid.pixels[i]);
        if(p.uncertaintyPixels)require(std::isfinite(*p.uncertaintyPixels) && *p.uncertaintyPixels>=0 && *p.uncertaintyPixels<=100);}
    RawPlateObservation result;result.grid_=std::move(grid);result.centres_=std::move(centres);result.orientation_=direction;result.algorithm_=algorithm;result.space_=space;return result;
}
PlateDetectionResult PlateDetectionResult::ready(RawPlateObservation observation){
    PlateDetectionResult r;r.diagnostics_.status=PlateDetectionStatus::Ready;r.observation_=std::move(observation);return r;
}
PlateDetectionResult PlateDetectionResult::failure(PlateDetectionStatus status,QString message){
    require(status!=PlateDetectionStatus::Ready && (status==PlateDetectionStatus::Unsupported || status==PlateDetectionStatus::InvalidInput || status==PlateDetectionStatus::NotFound || status==PlateDetectionStatus::Cancelled));
    PlateDetectionResult r;r.diagnostics_={status,std::move(message),{}};return r;
}
}
