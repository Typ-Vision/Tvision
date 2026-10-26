# COMM-01 — Kaynak/kapsama envanteri ve baseline

Durum: tamamlandı; [kaynak haritası](source-map.md) ve [kapanış](../../archive/2026-10-26-comm-01.md). Önerilen düzey: **medium**. Önkoşul yok; ilk karttır.
Bir oturumda yalnız bu kart; paralel uygulayıcı yok.

## Oku

- AGENTS, progress ve kök README ardından [plan indeksi](README.md).
- [Ortak sözleşme](contract.md): yalnız **C0**.
- [Doğrulama rehberi](verification.md); `source-map.md` oluştuysa yalnız bu kartın satırları.
- Dar ek okuma: docs/architecture/plc-socket.md; docs/architecture/multi-source-runtime-terminal.md; docs/architecture/editor-definition.md.
- Kaynakta önce ilgili sembolü bul; tüm mimari/arşiv veya bütün kartları yükleme.

## Tek hedef ve işler

1. Codebase-memory list_projects/index_status ile eski C:/Projects/Tvision kökünü ve actual C:/OpenCode/TVision checkout ilişkisini doğrula; seçili yolların coverage'ını kontrol et.
2. ToolValues/Registry/Graph, ProjectService/Repository, ExecutionService/EditorSession, RuntimeService/Coordinator, ProductionStore/ResultCodec ve PlcRuntimeSocket sorumluluklarını dar sembol aramasıyla eşle.
3. source-map.md içine actual dosya/sembol/test/şema tablosu, önerilen extension noktaları ve exact baseline komutlarını yaz; ürün kodunu değiştirme.

## Değişiklik sınırı

Mevcut/önerilen dosyalar: `src/domain/ToolValues.*; ToolRegistry.*; ToolGraph.*; src/application/{ProjectService,ExecutionService,EditorSession,RuntimeService,RuntimeCoordinator,ProductionStore,ProductionResultCodec,PlcRuntimeSocket}.*; src/persistence/ProjectRepository.*; CMakeLists.txt`.
COMM-01 actual symbol/path eşlemesini esas al; yeni isimler öneridir. Mevcut eşdeğer
servisi genişlet; beklenmeyen çapraz katman refactor'ını bu karta katma.

## Kabul

- İndeks kökü/coverage ve kaynak incelemesi kanıtlı; bağlantı hatası boş sonuç sayılmadı. Araç engelinde kart açık kalır.
- Mevcut Release UI/OFF baseline gerçek komutla kaydedildi; eski toplamlar kullanılmadı.
- Mevcut production admission/native engelleri ve new schema bump noktaları kaynakla doğrulandı.
- İlgili Release/test komutlarını çalıştır; UI değişmişse 1400×900 ve 850×600 görsel kontrolü.
- `python scripts/check-context-docs.py` ve `git diff --check` geçer.

## Kapsam dışı

İndeks/coverage eksikse baseline/dosya envanteri yapılabilir; yapısal inceleme tamamlanmaz ve COMM-02 başlamaz. Yeni framework veya kod refactor yok.

## Kapanış ve devir

Gerçek komut/sonuç/kanıtları `build/comm/COMM-01/` altında tut. Koşulmayan test ve
fiziksel/native engelleri açık yaz. progress özetini ve arşiv indeksini güncelle;
`docs/archive/YYYY-MM-DD-comm-01.md` kaydı oluştur, uygulayıcıyı belirt.
Tamamlanınca `COMM-01: ...` tek yerel commit; push yok. Kabul geçmeden tamam yazma.
Sonraki tek kart: **COMM-02**.
