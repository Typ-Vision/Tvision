#pragma once
#include "CommunicationServerLifecycle.hpp"
#include "domain/PublicationTemplate.hpp"
#include <memory>

namespace visionflow {
// An explicit, reviewed destination. No transport or execution is started here.
struct CommunicationMigrationInput {
    QString projectPath, profilesPath, binaryPath, registryPath, archiveDirectory;
    QUuid station;
    PublicationTemplate publication;
    CommunicationServerProfile server; // must be disabled during migration
};
struct CommunicationMigrationPlan {
    QJsonObject report;
    QMap<QString,QByteArray> files;
    bool supported() const;
    QByteArray digest() const;
};
class CommunicationMigrationPlanner final {
public:
    explicit CommunicationMigrationPlanner(CommunicationMigrationInput input):input_(std::move(input)){}
    // Reads the real serializers and both legacy/v4 journals; creates no files.
    CommunicationMigrationPlan analyze() const;
private:
    CommunicationMigrationInput input_;
};
// Owns the production archive write lock for its entire lifetime. Construct only
// after stopping Runtime/listeners. Existing archive writers fail closed while
// this lease is held. Main/UI composition is a separate explicit integration.
class CommunicationMigrationTransaction final {
public:
    using Fault=std::function<void(const QString&)>;
    CommunicationMigrationTransaction(CommunicationMigrationInput,QString workspace,Fault={});
    ~CommunicationMigrationTransaction();
    CommunicationMigrationTransaction(const CommunicationMigrationTransaction&)=delete;
    CommunicationMigrationTransaction& operator=(const CommunicationMigrationTransaction&)=delete;
    // Backup+manifest even for unsupported plans. Never overwrites a package.
    QByteArray prepare(const QString& package,const QByteArray& reviewedPlanDigest);
    // CAS over exact pointer bytes; empty means pointer absent. Explicit apply.
    void activate(const QString& package,const QByteArray& reviewedManifestDigest,
                  const QByteArray& expectedPointerDigest);
    // Keeps the live ledger, including new ACKed v4 history. Requires current,
    // reviewed ledger digest and a drained state; never copies an old ledger.
    void rollback(const QByteArray& expectedPointerDigest,const QByteArray& reviewedLedgerDigest);
    QJsonObject active() const;
    QByteArray pointerDigest() const;
    QByteArray ledgerDigest() const;
private:
    struct Lease;
    CommunicationMigrationInput input_;
    QString workspace_;
    Fault fault_;
    std::unique_ptr<Lease> lease_;
    void boundary(const QString&) const;
    QJsonObject manifest(const QString&,const QByteArray&) const;
    void swap(const QJsonObject&,const QByteArray&);
};
}
