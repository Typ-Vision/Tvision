# COMM4-26 — Entegre faz kapanışı ve fiziksel kabul raporu

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **25** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **verification V0–V3; contract S0–S8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Son kaynakta tam Release ON/OFF ve UI görsel kabulü birleştir. Implemented/software-verified/physical-blocked/unsupported capability raporuyla yeni tek UX ve migration devrini tamamla.

Sahiplik bu dizin kabul raporu, kullanıcı kullanım README ilgili bölümü, progress/archive; gerçek davranış açığı varsa yalnız dar gerekli düzeltme. Tarihsel arşivi yeniden yazma.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Eksik robot/lens/native/independent reference verisini fixture ile kapatma, CUDA/kurulum iddiası veya otomatik yeni faz.

## Somut davranış kabulü

1. Tam ON/OFF sayıları ve skipped nedenleri son kaynak/config/inputla eşleşir; hedefli test tam suite diye sunulmaz.
2. En az3station tek bağlantı, A/B, publish, ACK recovery ve calibration state kanıtları çapraz izlenir.
3. Gerçek fiziksel veri yoksa yazılım kapanışı ve fiziksel BLOCKED ayrılır; ilerideki fiziksel kabul işi açık ID/önkoşulla tanımlanır.

## Release ve teslim kabulü

Profil **F**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.
Ortak sözleşme/faz etkisi: hedefli kabulden sonra son kaynakta tam Release ON/OFF CTest zorunlu; skipped ayrı raporlanır.
UI Release ve normal/küçük ekranda gerçek görsel kontrol zorunlu; offscreen sonucu native/DPI kabulü değildir.

Teslim: Faz kabul raporu, güncel kullanım/migration rehberi ve açık fiziksel kapıların devri.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-26/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-26: …`** commit; push yok.
Sonraki: **Fiziksel engeller için yalnız kullanıcı yönlendirmesiyle yeni kabul kartı**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
