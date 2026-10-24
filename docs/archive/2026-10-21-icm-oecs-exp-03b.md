# ICM-OECS-EXP-03b — fiziksel USB kopma ve manuel yeniden açma

21 Ekim 2026. Uygulayıcı: Codex.
EXP-03'ün ayrı kalan fiziksel kopma/manuel yeniden açma maddesi doğrulandı.
[Önceki teslim](2026-10-21-icm-oecs-exp-03.md) değiştirilmedi; 500 seri kabulü korunur.

## Kapsam ve değişiklik

Kullanıcı önce kabloyu çıkardığını bildirdi; eski test penceresi kapanmıştı.
Kullanıcının yeniden takma onayından sonra yeni Runtime koşumu başlatıldı;
ilk başarılı gerçek çevrimden sonra kullanıcı USB'yi çıkardı. Hata gözlendikten
sonra yeniden takma onayıyla ayrı manuel Run koşumu yapıldı. Otomatik reconnect yok.
`tests/oecs_experimental_probe.cpp` ilk edinim başarısızsa kopma için hazır işareti
üretmeden duracak şekilde düzeltildi. Uygulama/UI davranışı değiştirilmedi.
Progress ve arşiv dizini bu yeni kanıta bağlandı.

## Yeni kanıt

Kök: `build/icm/oecs-exp-03/`.

- Release probe derlemesi: `build-disconnect-followup.log`, exit 0.
- `disconnect-confirmed.json`, exit 0: ilk çevrim ve operatör yanıtını beklerken
  11 ek çevrim tamamlandı; fiziksel çıkarma sonrası sonraki exact open/enumeration
  `OECS kamera bulunamadı` ile ERROR oldu. Hata çevriminde frame=0,
  newCycleDeniedAfterFault=true, coordinator Fault, automaticReconnectAttempted=false.
  13 deneysel kayıt: 12 gerçek DONE ve bir ERROR. Production kayıt=0,
  trusted completion yok; restart pasif.
- Bu yol her çevrimde open/capture/teardown yapar. Gözlem çevrimler arasındaki
  USB yokluğunun yeni edinimi durdurmasıdır; fiziksel command/buffer beklerken
  kopma veya SDK cancel/drain garantisi kanıtı değildir.
- `reopen-confirmed.json`, exit 0: kullanıcı yeniden taktıktan sonra ayrı manuel
  Load/Run, 1/1 DONE ve bir gerçek kare/kayıt; production kayıt=0,
  trustedEvidence=0, restartPassive=true. `reopen-confirmed/runtime-frame-0.pgm.json`
  gerçek seri **02A2220000627**, profile/readback ve false admission/eşleşmeyi korur.
- Son `python scripts/check-context-docs.py` ve `git diff --check` başarılı.
  Önceki 21 CTest/UI/500 seri bu ek adımda tekrarlanmadı; önceki teslimdeki
  tarihli kanıttır. Yeni uygulama/UI değişikliği yok; yeni kanıt gerçek probe'dur.

## Sınırlar ve devam

Deneysel EXP-03 e2e kapsamındaki 500 seri, Stop/deadline, fiziksel kopmada yeni
edinimin kapanması, manuel yeniden açma, kayıt/Logs/CSV ve pasif restart maddeleri
kanıtlandı. Bu sonuç native G1/G3/G4 veya mm/optik doğruluk kabulü değildir.
8/8 eski kare karşı örneği ve tetik/parça eşleşmesinin doğrulanmadığı durum korunur.
Sonraki iş kullanıcı yönlendirmesiyle EXP-02 deneysel kalibrasyon/mm; bağımsız
referans olmadan doğruluk kabulü yok. Tek görev commit'i; push yapılmaz.
