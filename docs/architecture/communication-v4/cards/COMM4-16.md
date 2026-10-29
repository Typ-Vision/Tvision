# COMM4-16 — Tek Communication yüzeyi

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **05,10,14,15** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S5; verification V1,V2**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

V2/V3 ayrı sayfalarını tek listener/station/delivery/migration görünümüne dönüştür. Station Run/Auto/Stop, aktif recipe revision, camera owner ve pending ACK açıkça gösterilsin.

Gerçek src/app/MainWindowCommunication.cpp ve ilgili panel/composition yolları01 ile doğrulanır; yeni aday station table/view model. Legacy sayfa sahipliği bu kartta kaldırılır, geçmiş veri silinmez.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Publish assignment satır editorü17, robot solver UI24 veya SDK/socket doğrudan UI çağrısı.

## Somut davranış kabulü

1. Üç station durumu aynı anda, selected station değişince port/listener değişmez.
2. Connected+STOPPED görünümünde Ready/PASS ima edilmez; pending ve migration unsupported nedenleri erişilir.
3. Normal/küçük ekran gerçek görsel, klavye/focus/scroll; legacy sekme yok; read-only geçmiş erişimi var.

## Release ve teslim kabulü

Profil **U**, [verification V0/V1](../verification.md) uygulanır.
İlgili Release ON ve OFF hedeflerini derle ve gerçek CTest adlarıyla hedefli kabulü yap. OFF başarısı ON davranışının yerine geçmez.

UI Release ve normal/küçük ekranda gerçek görsel kontrol zorunlu; offscreen sonucu native/DPI kabulü değildir.

Teslim: Yeni tek UI, UI automation/CTest ve iki viewport görsel kanıtı.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-16/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-16: …`** commit; push yok.
Sonraki: **COMM4-17**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
