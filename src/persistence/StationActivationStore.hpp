#pragma once
#include "application/StationActivationService.hpp"

namespace visionflow {
class FileStationActivationStore final:public StationActivationStore {
public:
    explicit FileStationActivationStore(QString path):path_(std::move(path)){}
    StationActivationCatalogue load() const override;
    void create(const StationActivationCatalogue&) const override;
    void compareExchange(const QUuid&,const StationActivationCatalogue&) const override;
private:
    void write(const StationActivationCatalogue&) const;
    QString path_;
};
}
