#pragma once
#include <QByteArray>
#include <QString>
#include <QUuid>
#include <QVector>
#include <cstdint>

namespace visionflow {
// Values are the frozen W6 catalogue categories. Keys share one installation namespace.
enum class CatalogueCategory : std::uint32_t {
    Station=1, RecipeRevision, TemplateRevision, Camera, IntrinsicRevision,
    PlateRevision, FrameRevision, AdapterRevision, AcquisitionProfileRevision,
    SolverPolicyRevision, ReferenceSetRevision, ToleranceRevision, RobotClock
};
struct CatalogueIdentity {
    CatalogueCategory category=CatalogueCategory::Station;
    QUuid entity, revision, parent;
    bool operator==(const CatalogueIdentity&) const = default;
};
struct CatalogueEntry {
    std::uint32_t key=0;
    CatalogueIdentity identity;
    QString name;
    bool retired=false;
    bool operator==(const CatalogueEntry&) const = default;
};
struct PeerEpochIdentity {
    QUuid peer, epoch;
    bool retired=false;
    bool operator==(const PeerEpochIdentity&) const = default;
};
struct IdentityCatalogue {
    QUuid installation, revision;
    // uint64 represents the exhausted UINT32_MAX+1 sentinel without wrapping.
    std::uint64_t nextKey=1;
    QVector<CatalogueEntry> entries;
    QVector<PeerEpochIdentity> epochs;
    bool operator==(const IdentityCatalogue&) const = default;
};
QByteArray exportIdentityUuid(const QUuid&);
QUuid importIdentityUuid(const QByteArray&);
void validateIdentityCatalogue(const IdentityCatalogue&);
// Store-side invariant: no lost/rewritten identity history, even for direct port callers.
void validateIdentityTransition(const IdentityCatalogue&, const IdentityCatalogue&);
}
