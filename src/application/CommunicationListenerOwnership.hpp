#pragma once
#include <QTcpServer>
namespace visionflow {
// Process-local protocol selection only. Existing v2/v3 fleet semantics are preserved.
// All listener composition runs on the Qt application owner thread.
class CommunicationListenerOwnership final {
public:
    static void selectV4(QTcpServer&);
    static void releaseV4(QTcpServer&);
    static void requireLegacyAllowed();
    static void observeLegacy(QTcpServer&);
};
}
