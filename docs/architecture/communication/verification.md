# Kart doğrulaması ve kapanış

## Seçili kart

Kök README'nin yerel Qt/MinGW/OpenCV PATH ve Release komutlarını kullanın.
COMM-01 güncel CMake target/CTest adlarını ve baseline'ı
`docs/architecture/communication/source-map.md` içine kaydeder; sonraki kartlar
bu dosyanın yalnız ilgili tablosunu okur. Henüz oluşturulmuş değildir.

Yeni test, hata/uyumluluk/state sınırını kanıtlamalı; implementation satırlarını
tekrarlayan test yazmayın. İlgili mevcut regression'ları koruyun. Yeni network
testleri gerçek localhost simulator kullanır; mocked başarı saha kanıtı değildir.
Yeni fixture/görsel/settings/trace `build/comm/COMM-NN/` altında; makineye özel
`config/local.ui.ini` test fixture'ı değildir.

- Domain/codec kartı: Release core ve ilgili test; UI OFF link korunur.
- Runtime/DAG/store/transport kartı: Release, ilgili execution/runtime/production/
  socket regression'ları ve hata/Stop/restart/duplicate senaryoları.
- UI kartı: Release UI, ilgili CTest, 1400×900 ve 850×600 ekran; uzun hata metni,
  klavye, scroll ve disabled state kontrolü. Native pencere testi yapılamazsa
  offscreen kanıtı native kabul diye sunulmaz.
- SDK kartı: clean probe Release link/run, dependency absent/OFF yapılandırması,
  açık lisans/kurulum sınırı. İnternetten doğrulanan güncel resmi SDK belgeleri.
- Birleşik kapı: COMM-23 TCP; COMM-28 Modbus; COMM-33 OPC; COMM-37 UI ON ve
  OFF Release, ilgili tüm yeni testler + mevcut full CTest. Yeni failure çözülmeden
  eski başarılı test toplamını yeni koşum gibi raporlamayın.

Çalıştırılacak komut modeli (actual test regex'i COMM-01 envanterinden seçilir):

```powershell
cmake --build build/reference-ui -j 6
ctest --test-dir build/reference-ui -R '<ilgili-test-regex>' --output-on-failure
python scripts/check-context-docs.py
git diff --check
```

## Faz bazında kabul

Temel sıra COMM-01–23 → COMM-36 → COMM-37'dir. COMM-36 iki TCP simulator
hedefi ve genel pose paketiyle kabul edilir; COMM-34/35 veya Modbus/OPC SDK'sı
istenmez. COMM-37 temel kapsamın full regresyonudur. İhtiyaca bağlı alt fazlar
seçilince kendi kabul kapıları ve COMM-36/37'nin ilgili genişletme/regresyon
kontrolleri yeni kanıtla uygulanır; tarihsel başarı yeni koşum diye sunulmaz.
COMM-38 cihaz bazında ayrıdır; yazılım kabulünü bekletmez ve yerine geçmez.

## Zorunlu hata matrisi

Eksik/stale kaynak, false Boolean'ın geçerli NOK olması, invalid değer, tip/birim
uyuşmazlığı, dirty draft vs loaded snapshot, Run Tool dışarı yazma=0, disk yazma
hatası, Stop/close/reconnect, duplicate/conflict, kaybolan ACK, process restart,
queue dolması, kısmi wire payload, farklı cycle bileşenleri ve endpoint down.
Her kart kendine ilişkin alt kümesini, birleşik kartlar tüm matrisi uygular.

## Kapanış

Kart ID/durum, yapılan değişiklikler, dosyalar, gerçek komut/sonuç/kanıt yolları,
koşulmayan kontroller, açık engeller ve sonraki tek kart kapanışta yazılır.
progress özetinde önemli kalıcı engeller korunur. Context checker'ın byte/link
sınırlarını sağlayın; ayrıntıyı kapanışa taşıyın. Tamamlanmış kart için görev ID'li
tek commit; yarım/engelli kartı tamamlandı yazmayın. Donanım yokluğu veri/codec
yazılım kabulünün yerine geçmez; fiziksel kabul açık kalır.
