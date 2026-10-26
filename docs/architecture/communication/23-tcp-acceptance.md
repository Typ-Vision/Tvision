# COMM-23 — TCP yazılım ve görsel kabul kapısı

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-22** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0–C7**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/communication/verification.md; önceki COMM kapanışlarından yalnız gerekli arayüz kanıtları.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. İki gerçek reçete fixture'ı kur: ölçüm + accepted ve mevcut gerçek Boolean kontrol sonucu; her biri çoklu variable/publish kullanır.
2. V2 regression ve v3 trigger/ACK/duplicate/restart/disk/Stop/queue matrisini gerçek localhost process'leriyle çalıştır.
3. Kullanıcı TCP v3 girişini yalnız bu kabul geçince etkinleştir; kısa kullanım örneği ekle.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `tests/plc_socket_test.cpp; runtime/production testleri; src/app/plc_simulator.cpp; gerekiyorsa dar fixing diff`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Ölçüm değeri/birim, NOK false, NOT_EVALUATED, correlation ve aynı payload retry trace ile kanıtlı.
- Run Tool/Editor preview 0 external write; selected/draft/loaded snapshot ayrımı.
- Release UI+core ve ilgili CTest, iki boyut; full gate'te başarısız kontroller açık, eski toplam yeni koşum değil.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Kamera native veya fiziksel PLC kabulünü kapatma; başarısız kabulde geniş refactor yerine küçük fix kartı.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-23/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-23.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-23: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-24**.
