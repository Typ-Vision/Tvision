# COMM4-02 — Wire freeze kararı

29 Ekim 2026. Uygulayıcı: Codex. GPT-6.1 Sol / medium.
**PASS — D tasarım/fixture kabulü.** Contract revision
`00434634-0002-4000-8000-202610290001` (opaque identity, hash'ten türetilmez).
Wire/fixture değişimi yeni revision ve freeze ister; bu revision çalışan ürün,
controller veya fiziksel doğrulama değildir. Sonraki kart COMM4-03'tür.

## Sahiplik ve çözülen kararlar

[Wire W0–W8](wire.md) byte otoritesidir; [verification](verification.md) D kabulünü,
[fixture manifest](../../../tests/fixtures/communication-v4/manifest.json) her
bin/hex SHA256, length, expected outcome ve digest'i tutar. Manifest SHA256:
`84785ed33367d9838696bf884b5c09291a16572a65f30dea2ad3dc0e91460c22`. Yeni dosya sahipliği
`tests/fixtures/communication-v4/` ile sınırlandı. C++/CMake/üretim codec yoktur.
S0 güncel kaynak haritası önceki kartta kaldı; yapısal üretim kodu keşfi gerekmedi.

- Exact512 request/1024 response ve128 header. W1/W4/W5 tablo satırları otomatik
  width/gap/overlap kontrolünden geçer. W3 her16 kind için bağımsız body width
  toplamı384, HELLO/CATALOG/operational body896; metadata256 + tail384 =640.
- Digest alan sırası önceki sözel listenin belirsizliğinden çıkarılıp W7'de exact
  slice olarak sabitlendi: station, request, peer, epoch, recipe, template,
  activation. Request prefix/kind dahil, session hariç. ACKED yeniden hash üretmez.
  SELECT_RECIPE yeni snapshot terminali ayrı seal/ACK eder; eski fingerprint kalır.
- W6a bütün13 category için version1 sabit256byte metadata ve reserved0 tanımlar.
  Tam model/template/tolerans yerel immutable content/dependency digest'i ile bağlıdır;
  fiziksel değerler uydurulmaz. Bilinmeyen enum/bit/schema reject, sessiz extension yok.
- Norm toleransı1e-6, sıralı Float64 normalization, q/-q işaret ve signed zero
  sabitlendi. SAMPLE raw fingerprint ile terminal canonical payload ayrı kalır.
- Remote commands, READY, active STATUS snapshot ve ayrı STOP/ABORT rezervi
  sabitlendi. Tek normal+tek control pending sınırı, global64, disk headroom ve
  rezerv dolu ret açık; Stop admission kapatma eylemi pending ACK'ye takılmaz.
- Holdout role2 SAMPLE ve origin/camera/frame terminal kimlikleri fixed layout'a
  sığar. uint64 MAX geçerli;0 geçersiz, rollover drained. UUID/GUID word order,
  signed32, UTF8 byte count, mask/reserved, pose frame/revision örnekleri kaydedildi.

## Dar robot protokol ön kontrolü

Marka/controller/firmware ve erişilebilir actual pose bilinmiyor. Bu yüzden ilk
wire yalnız **generic Float64 quaternion-wxyz, mm, right-handed column-vector**
adapter'ını kapsar. Euler/rotation-vector alanı eklenmedi. COMM4-19 robot entegrasyon
sınırında açık controller convention adapter'ını kurar; peer wire'a girmeden
çevirir. Dönüşüm yapamayan controller bu revision'da unsupported'dur; yeni Euler
wire alanı yeni freeze ister. Generic adapter controller uyumluluğu iddiası değildir.

Dar araştırma kaynakları29 Ekim 2026'da kontrol edildi: ROS
[REP103](https://github.com/ros-infrastructure/rep/blob/master/rep-0103.rst)
right-handed sistem ve quaternion tercihinin gerekçesini verir; bu wire ROS'un
birim veya quaternion bellek sırasını devralmaz. Universal Robots'un
[orientation açıklaması](https://www.universal-robots.com/articles/ur/application-installation/explanation-on-robot-orientation/)
axis-angle/rotation vector ile RPY'nin farklı olduğunu ve rotasyon sırasının
önemini açıklar. [Actual pose kaynağı](https://www.universal-robots.com/articles/ur/programming/read-a-single-coordinate-or-axis-rotation/)
metre ve radyan axis-angle tanımlar. Buradan çıkan tasarım kararı: RX/RY/RZ
adlarıyla sessiz Euler/mm varsayımı yapılamaz; controller-specific adapter gerekir.
UR ürünü seçilmedi veya test edilmedi. Kapsamlı optik/solver araştırması18'de kalır.

## Kanıt ve sınır

Yerel kanıt `build/comm4/COMM4-02/`: fixture-validation.log, dotnet-hashes.log,
validation.json, source-identity.json, context-check.log, diff-check.log.
Tüm82 frame,16 request/9 response kind,13 metadata kategori, bağımsız PLC tabloları,
negative oracle ve sayısal vektörler doğrulanır. Manifest SHA256 ve dosya SHA listesi
kapanış kanıtında tutulur. Profile D; Release ON/OFF, CTest, UI görsel ve fiziksel
koşum yapılmadı. Stream/timing/reservation kayıtları gelecek implementasyonun
beklentileridir; gerçek framer/ledger/yarış testi olarak raporlanmaz.

Native OECS correlation/drain, lens/intrinsics, actual6DOF/TCP/frame, plaka ve
bağımsız fiziksel reference/tolerance hâlâ açıktır. Fixture üretim izni doğurmaz.
Robot hareketi, Pickit drop-in, CUDA veya dağıtım kabulü bu karta dahil değildir.
