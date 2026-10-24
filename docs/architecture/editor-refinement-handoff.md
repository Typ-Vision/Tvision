# Sol 6.1 medium — Editor iyileştirme devri

Hedef uygulayıcı gpt-6.1-sol / medium; ajanlar sıralı çalışır.
[EUI planı](editor-refinement-plan.md). Bu belge modeli otomatik değiştirmez veya
yeni chat başlatmaz. İlk görev EUI-01; sonrakiler progress'e göre seçilir.

```text
C:\Projects\Tvision içinde EUI-01 görevini tamamla.
Önce AGENTS.md, docs/progress.md ve README.md'yi oku.
Ardından docs/architecture/editor-refinement-plan.md içindeki ortak kuralları,
seçili görev kartını ve yalnız o kartın gerektirdiği sözleşme/kod bölümlerini oku.
EUI-PLAN-01 planlaması tamam; uygulama sırası EUI-01 → 02 → 03 → 04 → 05 → 06 → 07 → 08.
Tek görev kapsamında ilerle; önceki U/KMC/ICM işlerini yeniden başlatma.
Kullanıcı beş Editor talebini fiziksel kamera kabulünden önce istedi.
7 Ekim GUI koruma kararı bu kapsam için 18 Ekim talebiyle güncellendi.

UI algoritma/SDK çağırmasın. Gerçek sonuç olmadan PASS/skor/pose üretme.
Eski reçete no-op/hash, çok kaynak, reference/pose/world ve Runtime snapshot
kurallarını koru. Edge yeni katalogdan çıkacak; eski tür/codec/executor kalacak.
Yerel görünüm tercihleri reçete semantiğini değiştirmesin.
İlgili Release build ve CTest yap; UI değiştiyse normal/küçük ekran görsellerini
açıp incele. Yeni gesture'ları gerçek mouse/klavye olaylarıyla doğrula.
Test ayarları ve kanıtlar build/eui/01/ altında; config/local.ui.ini'yi koru.
Kapanış kaydına Uygulayıcı: Codex yaz; kapsam, dosyalar, doğrulama, açık sınırlar
ve sonraki ID'yi kaydet. Progress ve arşiv dizinini güncelle.
python scripts/check-context-docs.py ve git diff --check çalıştır.
Tamamlanan görevi EUI-01: ... başlıklı tek commit olarak kaydet; push yapma.
Planlama aşamasını uygulama başarısı, eski test sonucunu yeni koşum gibi sunma.
```

Sonraki kartlarda mesajdaki ID ve `build/eui/XX/` yolu birlikte değiştirilir.
Bir kartın kapanışını görmeden diğerine atlanmaz. ICM-09b önkoşulları bu iş sırasında
açık kalır; test adaptörü normal ürüne veya native production yoluna açılamaz.
