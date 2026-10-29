# COMM4-18 — Robot optik ve 6D gözlenebilirlik araştırma kapısı

Durum: planlandı, başlanmadı. Model: **GPT-6.1 Sol / medium**.
Tek oturum kapsamı ve ortak kurallar [indekste](../README.md), kapanış
kuralları [V0](../verification.md) içindedir.

## Başlangıç ve önkoşul

Önkoşul: **01,02** tamamlanmış/kabul edilmiş olmalı. Başlangıç paketi AGENTS,
progress, README ve working-protocol; ardından [plan](../README.md),
[contract](../contract.md), [wire](../wire.md): **contract S6–S8**.
Gerekli bölüm dışında bütün kartları/kalibrasyon geçmişini yükleme.

## Tek sorumluluk ve sahiplik

Robot actual6DOF/frames/TCP ve kamera intrinsics/lens/distortion/plate6D yeteneklerini primary kaynak ve güncel kaynakla araştır. Mevcut2Dmm modelini ayır; her iki kurulum için solver seçimi ve sayısal uygunluk/validation ölçütlerini tanımla.

Sahiplik yeni calibration-readiness.md ve gerekirse contract araştırma kararları. Gerçek CalibrationService/Workflow/Capture/StudyStore içerik audit kapsamı bu kapıda dar tutulur.

Gerçek/adayı kart başında source-map ve V0 coverage kurallarıyla kesinleştir.

## Kapsam dışı

Gerçek donanım hareketi, yeni kamera SDK tahmini veya fiziksel veri eksikken accuracy kabulü.

## Somut davranış kabulü

1. Brand/controller/6DOF/lens/plate/tolerans her alan için mevcut/eksik+kanıt tablosu.
2. 2Dmm→6D dönüşümü olmadığı ve eye-hand/eye-base gözlenebilirlik/pose çeşitliliği koşulları açık.
3. Eksik fiziksel girdide DTO/fixture işleri ilerleyebilir; gerçek activation uygunluğu BLOCKED, sayısal threshold bilinmiyorsa açık bekleyen karar.

## Release ve teslim kabulü

Profil **D**, [verification V0/V1](../verification.md) uygulanır.
Yalnız belge/fixture tasarımı: Release ON/OFF koşulmaz; bu durum raporda açık yazılır. Link/diff/byte boyutu ve bağlam kontrolü zorunludur.

UI davranışı bu kartta değişirse U profili ve normal/küçük ekran görsel kabulü de zorunludur.

Teslim: İki kurulum için algoritma/adapter karar kaydı ve fiziksel blocker listesi.
V0 kapanışı zorunlu: kanıt `build/comm4/COMM4-18/`, tarihli arşiv kaydı,
progress/arşiv indeksi, bağlam kontrolü ve tek **`COMM4-18: …`** commit; push yok.
Sonraki: **COMM4-19**; kullanıcı yeni sıra vermedikçe aynı oturumda başlatma.
