# COMM4-11 — Sabit template ve typed assignment modeli

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **04** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S3; wire W4,W5**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

16Number/32Flag/4Text/1Pose şablonunu recipe dışı revisionla tanımla. Variable veya typed tool output kaynak seçimi ve özel slot adları/required kuralları doğrulansın.

Gerçek communication values/Publish config domain yolları01 haritasından; yeni aday PublicationTemplate ve SlotAssignment. UI yok.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Arbitrary expression, limitsiz custom slot veya network yazımı.

## Somut davranış kabulü

1. Number15/Flag31/Text3/Pose ataması; duplicate slot, incompatible unit/type/frame validation reject.
2. A/B aynı template sabit byte slot düzenini korur; template revision açık yeni activation ister.
3. WholePose source Point2D olamaz; String65byte için no-truncation sözleşmesi korunur.

## Release ve teslim kabulü

Profil **C**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: Template/assignment DTO, validation ve schema/no-op testleri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-11/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-11: …`** commit; push yok.
Sonraki: **COMM4-12**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
