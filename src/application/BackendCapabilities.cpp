#include "BackendCapabilities.hpp"
#include <stdexcept>
#ifdef VISIONFLOW_WITH_ZXING
#include <ZXing/ReadBarcode.h>
#include <ZXing/Barcode.h>
#include <ZXing/Version.h>
#ifndef ZXING_READERS
#error Pinned reader support required
#endif
#ifdef ZXING_WRITERS
#error Writer support is outside the product dependency contract
#endif
static_assert(ZXING_VERSION_MAJOR==2 && ZXING_VERSION_MINOR==3 && ZXING_VERSION_PATCH==0);
static_assert(sizeof(void*)==8);
#endif
namespace visionflow {
BackendCapability BackendCapabilities::get(const QString& id) {
    if(id!="zxing-reader")return {id,{},{},"Backend not compiled or unknown",false};
#ifdef VISIONFLOW_WITH_ZXING
    // Exercise a real reader symbol once, so available means linked and runnable.
    static const bool ready=[] {
        const unsigned char blank[64]={};
        ZXing::ReadBarcodes(ZXing::ImageView(blank,8,8,ZXing::ImageFormat::Lum));
        return true;
    }();
    return {id,"2.3.0",VISIONFLOW_ZXING_CONTENT_ID,{},ready};
#else
    return {id,{},{},"Build configured without ZXing reader",false};
#endif
}
void BackendCapabilities::require(const QString& id,const QString& version,const QString& contentId) {
    const auto c=get(id);
    if(!c.available || c.version!=version || c.contentId!=contentId)
        throw std::runtime_error(("Backend missing or identity differs: "+id).toStdString());
}
}
