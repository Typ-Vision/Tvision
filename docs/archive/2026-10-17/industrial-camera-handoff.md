> Tarihsel arşiv — 17 Ekim 2026 taşıması. Güncel devir: [progress](../../progress.md).

# Sol 6.1 medium — ICM uygulama devri

Hedef uygulayıcı **gpt-6.1-sol / medium**. Bir mesajda tüm kamera entegrasyonunu
bitirmeyi istemek yerine [ana plandaki](../../architecture/industrial-camera-plan.md) tek görev seçilir.
Kullanıcı birden çok ID verdiyse bağımlılık sırasıyla ilerlenir. Bu dosya yürütme talimatı
şablonudur; henüz kod yazımı başlatmaz.

## Güncel devam noktası

ICM-00 [başlangıç envanteri](../../architecture/industrial-camera-baseline.md) tamamlandı. Sonraki görev
**ICM-01a**; güncel sonuçlar progress'tedir. Aşağıdaki ICM-00 mesajı ilk devir
şablonu olarak korunur; tamamlanan envanter yeniden başlatılmaz. Devam için sonraki
görevler şablonunda ICM-XX yerine ICM-01a kullanılır.

## İlk görev için kopyalanabilir mesaj (tarihsel)

```text
C:\Projects\Tvision projesinde ICM-00 görevini uygula.
Model: GPT-6.1-sol, reasoning: medium.

Önce AGENTS.md, docs/progress.md ve README.md'yi oku.
Ardından docs/architecture/industrial-camera-plan.md,
industrial-camera-contract-plan.md ve
industrial-camera-tasks-foundation.md içindeki ICM-00 kartını oku.
Doğrulama için industrial-camera-acceptance-plan.md'yi kullan.

Yalnız ICM-00 kapsamını tamamla: mevcut kaynak/şema/hash/ret kapıları ve test
envanterini çıkar, industrial-camera-baseline.md oluştur, progress'i güncelle.
Kamera backend'ini uygulama veya capture/production kapılarını açma.
Mevcut dirty değişiklikleri koru; commit/push yapma.
Önceki test sayılarını yeni koşum gibi yazma. Sonuçta doğrulama kanıtını,
açık sınırları ve sonraki görevi belirt.
```

## Sonraki görevler için ortak mesaj

```text
C:\Projects\Tvision içinde ICM-XX görevini tamamla.
docs/progress.md, README.md, industrial-camera-plan.md ve ilgili görev kartını oku.
Önkoşulların gerçekten tamamlandığını progress ve kanıtla doğrula.
industrial-camera-contract-plan.md sözleşmelerini ve
industrial-camera-acceptance-plan.md kabul ölçütlerini uygula.

Kapsamı bu kartla sınırla; önceden tamamlanan KMC/ICM işlerini yeniden yazma.
UI SDK çağırmasın; webcam/file/legacy codec ve üç applicationType ortak yolu korunsun.
İlgili Release build/CTest yap; UI değiştiyse normal/küçük görselleri açıp incele.
Donanım kanıtı ile fixture kanıtını ayır; unavailable/ERROR yerine değer/PASS üretme.
Progress'e kapsam, dosyalar, doğrulama, sınırlama ve sonraki ID'yi yaz.
README ve mimariyi yalnız uygulanan durumla güncelle. Commit/push yapma.
```

`ICM-XX` gerçek ID ile değiştirilir. Chat metninde model yazılması uygulamanın model
ayarını otomatik değiştirdiği anlamına gelmez; kodlama chat'inde seçili model ayrıca ayarlanır.

## Çalışma kuralları

- Teknik rutin seçimler bu plan içinde yapılabilir; her dosya için kullanıcıdan tekrar
  izin istenmez. Model/transport biliniyor: COE-U USB3/OECS. ITALA desteği varsayılmaz.
- Donanım gerektiğinde actual exact serial eşlemesi kullan. Device ID=1 veya ilk listelenen
  kamera ile sessiz devam etme. SDK/producer yoksa build veya UI startup'ı çökertme.
- Gerekli fiziksel adım (reboot/kablo/plaka) yapılamıyorsa ilgili H/G kapısını açık kaydet;
  bağımsız yazılım görevi varsa sıraya göre ilerleyebilirsin. Fiziksel kabulü fake ile kapatma.
- Mevcut test sorunu çıkarsa önce kaynak/test beklentisini incele; sadece hata metnini
  değiştirmek veya whitelist'i gevşetmek acceptance düzeltmesi değildir.
- Yeni şema numaralarını baseline üzerinden seç ve migration tablosunu güncelle;
  aynı schema sürümüne zorunlu yeni alan ekleme. Old record replay ve hash korunur.
- SDK kütüphanelerini Git'e koyma. C API seçilirse vendor C++ `.lib` uyumunu varsayma;
  header ABI/struct/export ve runtime dependency kanıtıyla ilerle.
- Test dosyası/config/görseller build altında; `config/local.ui.ini` kullanıcı ayarıdır.
- Açık EXE build'i kilitliyorsa kullanıcı uygulamasını zorla kapatma; ayrı build hedefi
  kullan veya gerekli kapanışı bildir. Eski binary testini yeni kaynak kabulü diye yazma.
- Her kart sonunda sonraki bağımlılık açıkça yazılır. Kapsam genişlerse ilgili ID altında
  küçük alt görev tanımlanır; tek turda tüm source/execution/terminal katmanlarını değiştirme.

## İlk karar noktaları

| Karar | Kim verir / kanıt |
| --- | --- |
| GenTL + GenApi mi, OECS C API mi? | ICM-02 uygulayıcısı, gerçek ABI/feature deneyi ve lisans/dependency envanteri |
| Fiziksel seri ve bağlantı | ICM-01b actual discovery + OECS; kullanıcı Device ID'si tek başına yetmez |
| Timeout/exposure/latency hedefi | İlk güvenli profile ICM-03b/04b; üretim hedefi kullanıcı/hat gereksinimiyle |
| Ölçüm toleransı ve bağımsız referans | ICM-10 öncesinde kullanıcı/uygulama gereksinimi; otomatik varsayılmaz |
| Lens T15 öne alınsın mı? | ICM-10 alan bazlı gerçek hata/tekrar raporu |
| Çoklu gerçek kamera/senkronizasyon | G3 sonrası ayrı donanım ve kapasite kapsamı; dosya fixture sayısı yetmez |

## Görev tamamlanma kontrolü

1. Kartın tek teslimi uygulanmış ve kapsam dışı işler eklenmemiş.
2. Gerekli testler gerçekten yeni binary ile çalışmış; hatalar ve tekrarlar görünür.
3. İlgili yeni/legacy codec ve negative path kontrolleri geçmiş.
4. UI varsa gerçek görsel kontrol; fiziksel gerekiyorsa uygun kanıt veya açık durum.
5. progress, README/mimari uygulanan durumu anlatıyor; yalnız planlanan özelliği çalışan diye yazmıyor.
6. Sonraki görev ID'si ve donanım/karar bağımlılığı belirtilmiş.
