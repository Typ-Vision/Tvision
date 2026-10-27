# COMM-22 — Trigger/delivery diagnostics

Durum: yazılım/görsel kabulü tamamlandı; [kapanış](../../archive/2026-10-27-comm-22.md). Önerilen düzey: **low**. Önkoşul: **COMM-21** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C3, C5, C6**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/requirements/02_machine_vision_ui_gereksinimleri.md: bölüm 27,35–36.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Endpoint/Ready/Busy, request/cycle/result ID ve field validity/decision'ı okunur göster.
2. Pending/retry/ACKED/recovery-required durumlarını mevcut Runtime logs ile ilişkilendir; secret redaction uygula.
3. Retry Delivery/recovery inspect komutlarını mevcut immutable payload'a bağla; ölçümü tekrar başlatmasın.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/app/MainWindowCommunication.cpp; MainWindowRuntime.cpp; RuntimeDiagnostics.*; uygun küçük DeliveryDiagnosticsWidget`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- ACKTIMEOUT ile vision NOK ayrı, missing değer yerine eski sayı yok.
- Retry tek teslimi tekrarlar; unknown request'i OK veya ACKED yapacak düğme yok.
- İki boyut ve uzun packet/error string; raw log bounded.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Yeni genel log sistemi veya otomatik retention UI yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-22/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-22.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-22: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-23**.
