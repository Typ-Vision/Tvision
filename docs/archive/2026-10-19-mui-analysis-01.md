# MUI-ANALYSIS-01 — Ölçüm araçları ön analizi

19 Ekim 2026. Uygulayıcı: Codex.

Kapsam: kullanıcının pixel modunda Line Fit/Circle sonuç alamaması, yaklaşık
“insufficient point” mesajı, parametre sadeleştirme ve NOK çözüm önerileri isteği.
[Analiz ve önerilen MUI-01–04 sırası](../architecture/measurement-usability-analysis.md).
Yalnız belge teslimi; algoritma, UI, reçete, yerel INI ve ürün sözleşmesi değişmedi.

Doğrulanan kod bulguları: Circle mm→px geçişinde sayısal fit/şerit ayarları
korunuyor; tarama sayısı ile max gap bağımlılığı; Line raw/world çift kapısı;
tarama destek sınırları; EdgeFinder aday ret nedenlerinin Line/Circle yolunda
kaybı; exception sonrası yalnız hata metni. Yaklaşık hata, found-point yetersizliği
ile consensus yetersizliğini ayırt etmeye yetmiyor. Kullanıcı görüntüsü/ROI ve tam
hata mevcut olmadığından tek kök neden doğrulanmadı.

Değişen dosyalar: `docs/architecture/measurement-usability-analysis.md`, bu
kapanış, `docs/progress.md`, `docs/archive/README.md`. README ve gereksinimler
uygulama değişmediği için güncellenmedi. Progress'teki ayrıntılı EUI linkleri
yerine mevcut EUI-08/kullanıcı kabulü ve arşiv dizini korundu.

Yeni koşum: mevcut 18 Ekim Release binary'lerinde 19 Ekim seçili CTest **6/6**,
6.14 sn; `build/measurement-analysis/ctest.txt`. Testler: line_fit, circle_fit,
circle_metrics, radial_scan, edge_point, calibrated_circle contract.
Yeni derleme/UI görsel kontrolü yok; kullanıcı rasterinde tekrar üretim yok.
Bağlam kontrolü: `python scripts/check-context-docs.py`; çıktı
`build/measurement-analysis/context-check.txt`. `git diff --check` kontrolü.

Kararlar: günlük panel sadeleşebilir; güvenlik/kalite kapıları ve eski reçete
değerleri sessizce kaldırılmaz. NOK önerisi kanıta ve aşamaya bağlı olmalı.
Fit başarısızlığında gerçek aday tanısı korunabilir, geçerli ölçüm üretilemez.
Ürün toleransı aşımı ile ölçülememe ayrılır. Öneriler bu turda uygulanmadı.

Sonraki önerilen görev MUI-01; ardından MUI-02/03 ve gerçek raster kabulü MUI-04.
ICM-09b G0/G1/native ve diğer fiziksel kabul engelleri değişmedi.
