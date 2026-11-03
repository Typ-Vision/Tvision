#pragma once
#include "CommunicationPeerSessionService.hpp"

namespace visionflow {
// Test/PLC client profile. It carries connection information derived from the
// machine listener and the single active controlling peer/epoch. It is not a
// credential, a production permission or a licence to skip HELLO.
struct CommunicationClientProfile {
    QString label;
    QUuid installation, peer, epoch, contractRevision;
    QString address;
    quint16 port=0;
    bool operator==(const CommunicationClientProfile&) const = default;
};
QString communicationClientProfileSchema();
// Export requires exactly one active epoch and a usable address/port.
QByteArray exportCommunicationClientProfile(const IdentityCatalogue&,const CommunicationClientProfile&);
// Strict reader: unknown keys, wrong schema/contract, null UUIDs and unusable
// connection values fail closed. Installation ownership is checked separately
// because a profile alone cannot know which machine it will be used on.
CommunicationClientProfile readCommunicationClientProfile(const QByteArray&);
// Rejects another installation, no active identity, or a retired/stale peer.
void validateCommunicationClientProfile(const CommunicationClientProfile&,const IdentityCatalogue&);
}
