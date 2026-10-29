# COMM4-08a — PLC reçete seçimi ve Auto/Stop

Durum: tamam; [port sözleşmesi](../station-control.md), [kapanış](../../../archive/2026-10-29-comm4-08a.md). Model: **GPT-6.1 Sol / medium**.
Tek oturum, tek uygulayıcı. Önkoşul COMM4-08. Başlangıç paketi ve
[indeks](../README.md), [contract S1/S4](../contract.md), [wire W2/W3/W5](../wire.md)
okunur; diğer kartlar topluca yüklenmez.

## Sorumluluk ve sahiplik

SELECT_RECIPE/SET_AUTO/STOP dispatcher'ı UI ile aynı application portlarına bağla.
Gerçek Coordinator/recipe preparation kaynakları COMM4-01 haritasından;
yeni aday StationControlService. Sahiplik ve coverage kart başında doğrulanır.
Yerel Remote Enable, controlling peer, expected activation ve immutable kayıtlı
recipe revision doğrulanır. Editor draft seçimi çalıştırılmaz. Hazırlık async,
terminal READY gerçek tamamlanma sonrasıdır; SELECT_RECIPE sonrası Auto kapalıdır.
STOP admission'ı derhal kapatır, pending ACK sırasında da sınırlı control terminal
rezerviyle işler; gerçek drain sonrası tamamlanır. Henüz13 ledger uygulanmadığı
için durable port contract ve fake store testleri kurulur; gerçek disk entegrasyonu
13'te tamamlanır. Durable port yoksa üretim admission açılmaz.

## Kapsam dışı

Robot hareketi, yeni UI, worker scheduler ve kayıtlı olmayan reçete importu.

## Somut kabul

1. PLC A seç → SET_AUTO → READY → tetik; A terminal ACK → B seç → yeni activation.
   Socket session değişmez; A'ya dönüş eski activation'ı canlandırmaz.
2. Remote disabled/yanlış peer/stale revision reject; eksik kamera prep READY vermez.
3. Local Stop ile remote SET_AUTO yarışı: geç callback Auto açamaz.
4. Pending üretim ACK sırasında STOP kabul edilir, eski terminal korunur;
   control ACK başka sonucu kapatmaz. S2/S3 çalışmaya devam eder.
5. Aynı komut retry yeni preparation/load üretmez; SELECT_RECIPE terminal ACK'i
   yeni activation header'ıyla eşleşir, eski request fingerprint korunur.

## Teslim

[V0/V1](../verification.md) C profili: ilgili Release ON/OFF hedefleri ve CTest;
UI değişirse U. Kanıt build/comm4/COMM4-08a/, tarihli kapanış ve progress/arşiv
indeksi, bağlam kontrolü, tek `COMM4-08a: …` commit; push yok.
Sonraki COMM4-09; aynı oturumda otomatik başlatma.
