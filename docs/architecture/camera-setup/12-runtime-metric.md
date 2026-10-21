# CCUX-12 — Deneysel Runtime metrik snapshot ve kayıt sözleşmesi

Durum: planlandı. Önkoşul: CCUX-07, CCUX-08, CCUX-10. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C4–C5; [deneysel Runtime](../experimental-runtime.md); [çok kaynaklı terminal](../multi-source-runtime-terminal.md) snapshot/terminal bölümleri.
Kaynak: `src/application/ExperimentalRuntime.hpp/.cpp`, `src/application/MultiSourceRuntimeService.hpp/.cpp`, `src/application/MeasurementWorkspace.hpp`, `src/application/ProductionRecipeService.cpp`.

## Uygula

1. Raw/px-only admission kuralını model bazlı kontrollü deneysel metrik desteğe genişlet. Her required source için exact model content/version, frame geometri/köken ve edinim ayarlarını dondur.
2. Yürütmede snapshot/registry/model/sidecar değişimini reddet; source→model bağı ve farklı kaynakların birimleri ayrı kalsın. Karma raw/metrik graph'ı destekle.
3. Deneysel completion/digest ve kalıcı codec'i sürümleyerek model/koordinat alanı/birim kanıtını taşı. Eski experimental-runtime1 Raw/px kayıtları aynı anlamla okunmalı.
4. mm scalar gerçek hesaplamadan gelir; parça kararı/production admission hâlâ deneysel sözleşmeye uygun, PLC/robot blocked. Trusted terminal decode yeni deneysel kaydı da reddetsin.
5. Retry aynı frozen payload'ı yazar; Stop/timeout/late completion/restart eski sonucu canlı sonuç gibi yaymaz. Değişen experimental-runtime sözleşmesini güncelle.

## Kabul

İki farklı model/kaynak, raw+mm, missing/wrong model, farklı geometri, değiştirilmiş sidecar/model, forged trusted flag, eski kayıt replay, write retry/no reacquisition, Stop/restart. Metrologi birim/port doğrulaması.

**CTest:** multi_source_production_contract, multi_source_terminal_trace, industrial_runtime_admission_contract, metrology_runtime_contract, runtime_persistence

## Sınır ve kapanış

UI/CSV sunumu sonraki kart. Kayıt limitleri, idempotency ve production/experimental ayrımı gevşetilmez. Saf servis kartı; fixture'la tamamlanabilir.
Kanıt: `build/ccux/ccux-12/`. AGENTS ve ortak sözleşme C6 ile kapat.

