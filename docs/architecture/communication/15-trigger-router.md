# COMM-15 — Ortak input snapshot ve Trigger Router

Durum: başlamadı. Önerilen düzey: **medium**. Önkoşul: **COMM-14** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C5, C6**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/plc-socket.md: admission; docs/architecture/multi-source-runtime-terminal.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Typed immutable InputSnapshot store ve TriggerRequest/AdmissionResult servis sınırını kur.
2. Manual/legacy entegrasyon davranışını koruyarak yeni adapter'ların tek admission yolunu kullanmasını sağla.
3. Required input freshness/quality, duplicate/conflict ve tek-cycle arbitration'ı existing coordinator guards'a bağla.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/application/RuntimeCoordinator.*; RuntimeService.*; öneri TriggerRouter.*; tests/trigger_router_test.cpp`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- İki endpoint eşzamanlı tetik verdiğinde tek cycle; diğerine açık BUSY.
- Bad/stale/partial snapshot ve yanlış recipe/config acquisition=0.
- Editor acquisition kullanımında, NOT_RUNNING/Stop ve delivery pending'de eski admission gerekçeleri korunur.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Wire protokolü ve yeni Wait tool yok; native kamera iznini gevşetme.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-15/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-15.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-15: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-16**.
