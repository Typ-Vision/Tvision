# ICM-BAS-03 — Basler Calibration–Editor–Runtime ürün kabulü

Durum: Planlandı. Bağlı: [ICM-BAS-02b](ICM-BAS-02b.md)/G1 ve mevcut ortak G2.
Sonraki: kamera fazı değerlendirmesi → VF-08; bu kart VF-08'i başlatmaz.

## Oku

- [Ortak kabul](README.md), BAS-01/02a/02b kapanışları.
- [Eski backend kartı](../industrial-camera-tasks-backends.md) BAS-03,
  [entegrasyon kartı](../industrial-camera-tasks-integration.md) ICM-09b,
  [kabul matrisi](../industrial-camera-acceptance-plan.md) H01–H08/G3.
- [CCUX sözleşmesi](../camera-setup/contract.md) C2a/C3/C4/C5;
  [ana plan](../camera-backend-activation-plan.md) kapsam sınırları.

## Davranış ve sahiplik

Mevcut Camera Manager, Calibration, Editor, Runtime application/provider
composition ve kaynak hazırlığı; persistence/Logs/CSV kanıt akışı, ilgili
integration/UI testleri ve fiziksel kabul raporu. Ortak motor yeniden yazılmaz;
yalnız kabulün açığa çıkardığı gerekli dar entegrasyon eksikleri düzeltilir.

1. Kayıt/ayar/preview → kalibrasyon çekimi → Editor kaynak bağı → immutable
   Runtime hazırlık/çevrim → kalıcı sonuç/Logs/CSV zinciri exact kamerayı korur.
   Üç applicationType aynı Editor/Runtime kullanır. Stop/çıkış lease'i bırakır.
2. Gerçek 1000 cycle/teardown; kopma, Viewer contention, late completion,
   source/model/geometri değişimi, disk retry ve restart denetlenir. Retry yeni
   capture üretmez; restore üretimi veya kamera bağlantısını kendiliğinden açmaz.
3. Basler+dosya/webcam gibi desteklenen mixed-source senaryoları gerçek kaynak
   kökeniyle sınanır. Aynı reçetede OECS+Basler sınırı anlaşılır retle korunur.
   Aynı PC'de OECS/Basler keşfi ve ayrı oturumları birbirini bozmaz.
4. Raw/px kalibrasyonsuz kullanılabilir; mm gerekiyorsa gerçek uygun model, lens/
   düzlem ve bağımsız referans gerekir. Eksik kanıtla PASS/skor/pose üretilmez.

## Kabul

H01–H08'in her satırı kanıt/komut/sonuç veya açık engelle eşlenir; fixture ve
fiziksel ayrı tutulur. Başlangıç regexi:
`^(camera_setup_acceptance|industrial_calibration_capture|industrial_scoped_calibration|camera_acquisition_sessions_contract|runtime_camera_sessions_contract|industrial_execution_contract|industrial_runtime_admission_contract|ui_camera_manager|ui_editor_camera_sessions|ui_runtime_camera_sessions)$`.
Integration matrisinin gerektirdiği diğer mevcut/yeni testleri envanterden ekle;
bu regex tek başına H01–H08 yerine geçmez. Ortak tam Release ON/OFF ve normal/
küçük UI yanında native operatör akışı, gerçek 1000 cycle ve teardown kanıtı.
Kanıt: `build/icm/bas-03/`; G3 yalnız exact backend/model kapsamında kapanır.

Kapsam dışı: G4 bağımsız mm kabulünü varsayma, robot/PLC fiziksel devreye alma,
kurulum paketi, GigE/başka model, eşzamanlı çok kamera, VF-08 yürütmesi.
Eksik fiziksel satırlar tamamlanmadan ürünün fiziksel G3'ü tamam denmez.
