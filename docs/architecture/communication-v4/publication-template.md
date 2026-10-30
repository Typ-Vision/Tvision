# COMM4 sabit publication domain modeli

COMM4-11. Ürün davranış otoritesi [contract S3](contract.md#s3--reçete-sabit-template-ve-publish),
byte düzeni [wire W4/W5](wire.md#w4--poseblock-128-byte-göreli-offset).
Kaynak `src/domain/PublicationTemplate.hpp/.cpp`; UI/Qt Widgets bağımlılığı yok.

`PublicationTemplate` reçeteden bağımsız UUID+revision taşır. Dört sabit array
16 Number,32 Flag,4 Text,1 whole Pose kapasitesini korur. Slotun adı ve required
bayrağı değişebilir; indeks byte düzenini belirler. Adlar 1–64 ASCII harf/rakam/
`_.-`, ilk karakter harf, template genelinde benzersizdir. Number encoding Int32,
rounding tiesToEven olarak sabittir; signed32 rational scale/offset, pozitif
denominator, sıfır olmayan scale ve sıralı signed32 aralık doğrulanır. Conversion
`roundTiesToEven(source*scaleNumerator/scaleDenominator + offsetNumerator/offsetDenominator)`
olarak tanımlıdır. Değer/overflow değerlendirmesi COMM4-12'ye aittir.

Text limiti 1..64 UTF-8 byte'dır. `validatePublicationText` empty string'i kabul
eder, 65 byte veya eşleşmeyen UTF-16 surrogate'ı reddeder; kesme/replacement yoktur.
Pose her zaman mm SpatialTransform'dur; target/source geometric frame UUID'leri,
frame revision UUID'leri ve açık adapter contract UUID'si zorunludur. Eski
`FrameRef` integer revision'ından wire UUID revision'ına örtük dönüşüm yapılmaz;
catalogue/composition bu kimlikleri açıkça çözmelidir.

`PublicationAssignments` exact template UUID+revision ve en çok53 `SlotAssignment`
taşır. Variable source canonical variable UUID'sidir, port boş/revision0;
tool source canonical tool UUID+typed port+pozitif config revision'dır. Value,
constant, expression veya assignment içinden gelen type authority alanı yoktur.
Caller'ın authoritative `PublicationSourceDefinition` kataloğuna karşı exact
source/revision çözülür. Duplicate source authority/slot ve atanmamış required
slot reddedilir. Optional slot atanmadan kalabilir. Number yalnız typed numeric
kaynak, Flag Boolean, Text String, Pose SpatialTransform kabul eder; exact unit
ve Pose frame/revision/adapter eşleşmesi aranır. Metre/mm dönüşümü veya projection
gerekiyorsa açık typed adapter gerekir; Point2D/Pose2D whole Pose olamaz.
Cycle/DAG/provenance, missing değer, signedness/overflow ve publication validity
COMM4-12 evaluator'ının sorumluluğudur. Bu kart değer üretmez.

Template ve assignment JSON şemaları version1, exact key/type/count doğrular;
bilinmeyen alan/schema reddedilir. Recipe opt-in `publicationV4` alanında yalnız
assignment referansları saklanır. `recipePublicationAssignments` alan yoksa boş
döner ve tanımı değiştirmez; `withRecipePublicationAssignments` tekrarı byte no-op'dur.
V3 `publication`/OutputGroup/PublishConfig ve Editor schema'sı değişmez. Bu saf
codec API'si otomatik migration veya mevcut Runtime'a publication etkinleştirmesi
yapmaz; snapshot/evaluator entegrasyonu sonraki karttadır.

`validatePublicationTemplateReplacement` aynı revision metadata değişimini
reddeder. `validatePublicationActivation` active template snapshot'ıyla exact
eşleşme ister; farklı revision eski aktivasyon altında kullanılamaz. Bunlar domain
kapılarıdır; listener/peer approval/drain ve station activation composition'ının
yerine geçmez. Yeni snapshot açık activation işleminde kurulmalı, eski snapshot
ve terminal geçmişi korunmalıdır. Kalıcı template store ve canlı station/manifest
composition bu kartın kabulü değildir.

Kabul: `publication_template_contract`; legacy publication/assignment/spatial,
COMM4 codec ve station activation komşu testleriyle Release ON/OFF hedefli profil C.
