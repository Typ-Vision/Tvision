#include "RobotCalibrationWorkspace.hpp"
#include <stdexcept>
namespace visionflow {
void RobotCalibrationWorkspace::owner() const {if(owner_!=QThread::currentThread())throw std::runtime_error("Calibration owner thread required");}
void RobotCalibrationWorkspace::bind(Read read,Execute execute){owner();if(!read || !execute)throw std::runtime_error("Calibration read/command ports required");read_=std::move(read);execute_=std::move(execute);}
RobotCalibrationView RobotCalibrationWorkspace::view(const QUuid& station,HandEyeMode mode) const {
    owner();
    if(read_){auto v=read_(station,mode);if(v.station!=station || v.mode!=mode)throw std::runtime_error("Calibration projection context mismatch");return v;}
    RobotCalibrationView v;v.station=station;v.mode=mode;
    v.frameReadiness="BLOCKED • Robot base/flange frame ve revision eksik";
    v.tcpReadiness="BLOCKED • Actual flange veya açık TCP offset/revision gerekli";
    v.lensReadiness="BLOCKED • Lens, intrinsic/distortion ve bağımsız referans eksik";
    v.actualPose="Actual pose yok; commanded pose kullanılamaz";
    v.leaseOwner="Kalibrasyon lease alınmadı";
    v.drain="Bekleyen kalibrasyon edinimi yok";
    v.physical="PHYSICAL BLOCKED • Controller, frame/TCP, lens, native drain ve bağımsız fiziksel kabul bekleniyor";
    return v;
}
QString RobotCalibrationWorkspace::disabled(const RobotCalibrationView& v,RobotCalibrationAction a){
    using A=RobotCalibrationAction;
    if(v.station.isNull())return "Önce kayıtlı station seçin";
    if(a!=A::Abort && a!=A::Recover){
        if(v.recoveryRequired)return "RECOVERY_REQUIRED • Yeniden bağlantı otomatik capture başlatmaz";
        if(v.draining)return "DRAINING • Gerçek worker/kamera kapanışı bekleniyor";
        if(!v.connected)return "Robot actual-pose bağlantısı hazır değil";
    }
    if((a==A::SampleSolve || a==A::SampleHoldout) && !v.actualPoseReady)return "Actual pose yok veya stale; yeni SAMPLE için exact actual pose gerekli";
    if(a!=A::Abort && a!=A::Recover && !v.fixture && !v.physicalReady)return "PHYSICAL BLOCKED • Fiziksel uygunluk kapısı kapalı";
    if(!v.commands.contains(a))return "PHYSICAL BLOCKED • Application komutu için uygun adapter / bağımlılık yok";
    return v.commands.value(a);
}
QString RobotCalibrationWorkspace::command(const RobotCalibrationCommand& c){
    owner();auto current=view(c.expected.station,c.expected.mode);
    if(current.revision!=c.expected.revision || current.session!=c.expected.session || current.candidate!=c.expected.candidate || current.candidateRevision!=c.expected.candidateRevision || current.validation!=c.expected.validation || current.activeRevision!=c.expected.activeRevision)throw std::runtime_error("STALE_CONFIG • Görüntülenen kalibrasyon değişti; yenileyin");
    const auto reason=disabled(current,c.action);if(!reason.isEmpty())throw std::runtime_error(reason.toUtf8().constData());
    if(!execute_)throw std::runtime_error("Calibration application port unavailable");
    return execute_(c); // application service owns durable dedup, lease and physical gate
}
}
