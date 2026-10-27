# COMM-20 — Endpoint bağlantı profili UI

Durum: yazılım/görsel kabulü tamamlandı. [Kapanış](../../archive/2026-10-27-comm-20.md). Önerilen düzey: **low**. Önkoşul: **COMM-19** tamam.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C4, C7**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/requirements/02_machine_vision_ui_gereksinimleri.md: bölüm 26.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Communication ekranında endpoint liste/add/edit ve TCP v3 profili bağla; service API dışında socket çağırma.
2. Enabled, role/address/port, credentialRef görünümü ve timeout ayarlarını profile transaction'ıyla kaydet.
3. Connect/Disconnect/Test yalnız bağlantı durumunu denesin; Run veya gerçek sonuç üretmesin.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/app/MainWindowCommunication.cpp; CommunicationProfiles.*; uygun EndpointProfileEditor.*`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- Cancel/no-op ayarları değiştirmez; yanlış adres/port çakışması görünür.
- Draft profile edit loaded snapshot'ı sessiz değiştirmez; active değişimde Stop/reload kuralı.
- İki boyut, klavye ve secret redaction; eski v2 ayarı korunur.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

Modbus/OPC seçenekleri kurulu değilse aktifmuş gibi gösterme; mapping UI sonraki kart.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-20/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-20.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-20: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-21**.
