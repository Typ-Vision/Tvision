# CCUX-02 — Kamera ayarı ve readback application servisi

Durum: planlandı. Önkoşul: CCUX-01. Uygulayıcı hedefi: GPT-6.1 Sol / medium.
[Plan ve ortak çalışma kuralları](README.md). Güncel durum progress'tedir.

## Oku

[Ortak sözleşme](contract.md) C1–C2; [entegrasyon kartları](../industrial-camera-tasks-integration.md) ICM-05b.
Kaynak: `src/application/IndustrialCameraConfiguration.hpp/.cpp`, `src/application/IndustrialCameraProvider.hpp/.cpp`, `src/application/CameraLocalConfiguration.hpp/.cpp`, `src/application/MeasurementWorkspace.hpp`; `src/app/CameraManager.cpp` içindeki apply/editProfile akışını sembolle bul.

## Uygula

1. Kamera ID'sinden varsayılan kayıt ve uygun yerel rotayı çözen application girişini ekle; UI'nın profil JSON'u kurmasına gerek kalmasın.
2. Desteklenen exposure/gain aralıkları, birimleri ve başlangıç/readback değerlerini DTO olarak sun. Mevcut backend okuma desteği yetersizse hata/unsupported sun, sabit başarılı değer uydurma.
3. Ayar uygulama → gerçek readback → kayıt güncellemesini mevcut worker/lease/gate ile bağla. Donanım/disk arasında atomiklik iddia etme; rollback başarısızlığını açık unusable durumla ele al.
4. Auto/manual exposure/gain davranışını cihaz desteğine göre belirle. Profil değişimi ile kritik geometri değişimini ayrı bildir.
5. Exact hatırlanan/tek rotayı otomatik seç; çoklu belirsiz rotayı kullanıcı çözümüne bırak. Backend admission kapıları değişmesin.

## Kabul

Aralık dışı/NaN, unsupported gain, readback uyuşmazlığı, disconnect/apply failure, rollback failure, stale registry ve Runtime busy negatifleri. Aynı ayarda no-op; değişen kamera dışındaki kayıtlar korunur.

**CTest:** industrial_configuration, industrial_provider_contract, camera_transport, industrial_oecs_experimental

## Sınır ve kapanış

Sade ekran CCUX-03'te; kalibrasyon geçerliliği CCUX-06'da. Mevcut invalidation henüz gevşetilmez.
Kanıt: `build/ccux/ccux-02/`. AGENTS ve ortak sözleşme C6 ile kapat.

