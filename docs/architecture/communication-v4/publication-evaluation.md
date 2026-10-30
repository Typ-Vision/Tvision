# COMM4-12 — Immutable fixed-slot değerlendirmesi

`FixedSlotPublication` Qt Core application API'sidir; v3 `PublicationService`
capture/dependency/requireCurrent/evaluate yolu korunur. `capture` template,
assignment, authoritative binding kataloğu ve config kimliğini kopyalar; dışarıya
yalnız `shared_ptr<const ...>` verir. Binding metadatası assignment JSON'undan
veya gelen değerden türetilmez. Tekrarlanan/eksik kaynak ve dependency cycle
capture'da reddedilir; en çok256 binding vardır.

Variable kaynağı UUID'siyle exact committed Set Variables bundle field'ına,
direct kaynak tool UUID/port/config revision'a çözülür. `PublicationCycleView`
iki biçim için aynı `AssignmentInputView` içindeki tool output ve bundle
kopyalarını kullanır. `inputs.variables` tanımdır, değer veya fallback değildir.
Eksik kaynak önceki çevrimden tamamlanmaz. Her değer valid/payload/type/unit,
cycle/config/producer/port/revision ve source-specific acquired frame denetimi
alır. Bundle ayrıca committed/cycle/config/revision doğrular.

Binding dependency listesi değerin taşıdığı **tam provenance manifest'idir**;
AssignmentEvaluator'ın koruduğu upstream bağımlılıklar dahil edilir. Listedeki
her kaynak binding kataloğunda bulunur ve aynı cycle view'da recursive doğrulanır.
Değerin dependency token kümesi (`producerId:port`) exact eşleşir. Stale bir
upstream, geçerli görünen downstream bundle'ı da geçersiz yapar. Bu metadata
activation/composition katmanının doğrulanmış DAG'ından gelmelidir; API reçete
JSON'undan yeni graph veya SDK sonucu üretmez.

Number dönüşümü `roundTiesToEven(value * scaleN/scaleD + offsetN/offsetD)`
işlemidir. Int64 double'a çevrilmez; binary64 sign/mantissa/exponent ayrılarak
exact rational olarak hesaplanır. Sabit1280 bit unsigned limb aritmetiği tüm
finite binary64 ve int32 metadata kombinasyonlarını kapsar. Signed32 veya slot
aralığı dışı sonuç invalid; wrap/saturation yoktur. Yuvarlama CPU rounding
mode'dan bağımsızdır. Bool numeric truthiness olmaz. Text malformed UTF-16 veya
UTF-8 byte sınırı aşımında reddedilir; boş metin geçerlidir.

Whole Pose `SpatialAssignmentValue` gerektirir. Binding'de gerçek `FrameRef`
source/target çifti ile açık UUID frame/revision/adapter contract metadatası
birlikte sabitlenir. Sayısal FrameRef revision'ı UUID'ye dönüştürülmez; composition
bu eşlemeyi açık verir. SpatialTransform doğrulaması ve exact frame/revision
eşleşmesi yapılır; quaternion wxyz ve translation mm korunur. Eksik pose optional
boş kalır; identity veya planar yükseltme üretilmez.

`FixedSlotResult` W5 seal öncesi veridir:16 signed Number,32 Flag bitmap,4 UTF-8
Text ve optional whole Pose/contract. Invalid alanlar kanonik sıfır/boştur;
validity mask ayrıdır. Required invalid `RESULT_INVALID/resultValid=false`
üretir, diğer geçerli slotları diagnostics için korur. Done execution ve OK/NOK
kararı değişmez. Error/canceled/unknown tüm slotları kapatır, decision
NotEvaluated ve resultValid=false kalır. Result geçerliliği OK anlamına gelmez.

Bu saf API'nin socket/ledger/callback/persistence yetkisi yoktur; Editor preview
aynı API'yi yan etkisiz kullanabilir. `FixedSlotResult` terminal UUID/digest veya
commit yetkisi değildir. Main/station snapshot composition, wire sealing ve
kalıcı ledger entegrasyonu sonraki kartlara aittir; mevcut Runtime/Editor bu
API'yi otomatik etkinleştirmez. UI davranışı değişmedi.

Kabul: `fixed_slot_publication_contract`; gerçek AssignmentEvaluator bundle'ı,
eşdeğer direct slot, provenance/DAG negatifleri, integer rational oracle,
binary64 tie/komşu/subnormal, dört rounding mode, signed32 sınırları, Unicode ve
whole Pose. Profil C, Release UI ON/OFF; kanıt kapanış kaydındadır.
