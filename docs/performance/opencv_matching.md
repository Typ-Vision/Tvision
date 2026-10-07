# OpenCV matching kararı

7 Ekim 2026. **5 MP resimde <5–10 ms garantisi veren tek bir OpenCV fonksiyonu yoktur.** Şablon boyutu, ROI, kanal sayısı, açı/ölçek taraması, CPU/GPU ve transferler sonucu belirler.

## Projede kaydedilen seçim

CPU doğruluk başlangıcı: `cv::matchTemplate(gray, model, scores, cv::TM_CCOEFF_NORMED)` ve `cv::minMaxLoc`. Kod: `src/vision/matching/TemplateMatcher.cpp`; varsayılan: `config/matching.defaults.json`. Skor ortalama çıkarılarak normalize edilir; bu yöntemin en hızlı olduğu iddia edilmez.

CUDA hız adayı: `cv::cuda::createTemplateMatching(CV_8UC1, cv::TM_CCORR_NORMED)` ardından `matcher->match(...)`. Yerel CUDA OpenCV modülleri olmadığından etkin değildir ve ölçülmemiştir. `TM_CCORR_NORMED` ile `TM_CCOEFF_NORMED` aynı skor değildir; eşikler ayrıca doğrulanır.

CPU benchmark altı yöntemi karşılaştırır: SQDIFF, SQDIFF_NORMED, CCORR, CCORR_NORMED, CCOEFF, CCOEFF_NORMED. SQDIFF için minimum; diğerlerinde maksimum iyi sonuçtur. Template match otomatik angle/scale aramaz. `cv::matchShapes` yalnızca iki şekli karşılaştırır; resimde konum aramaz. `cv::GeneralizedHoughGuil` açı/ölçek araması için başka bir adaydır, latency garantisi vermez.

CUDA için model/tampon tekrar kullanımı, GPU üzerinde sonuç azaltma ve score map yerine yalnız gerekli sonucun taşınması değerlendirilir. CUDA API'de mask parametresi yoktur. Mevcut CPU adapter de mask ve çoklu match uygulamaz. Result filtering mask model maskesinden ayrı tutulmalıdır.

## Ölçüm kapsamı

Yerel benchmark: Release, 2592×1944 = 5,038,848 piksel, CV_8UC1; seed sabit sentetik rastgele görüntü, görüntüden alınmış 32×32/128×128 şablon. Tam kare ve 512×512 ROI. 10 warmup + 100 ölçüm; match ve minMaxLoc birlikte, scratch tamponu tekrar kullanılır. Görüntü üretimi, acquisition, gri dönüşümü ve UI hariç. Doğruluk yalnız bilinen tam kopyanın top-left konumudur; negatif/ışık/açı/gerçek parça doğrulaması değildir. CSV `local_matching.csv` içinde.

Üretim kabulü: gerçek pozitif/negatif örnekler, 32/64/128/256 şablonlar, gerçek ROI'ler, 50 warmup ve en az 1000 ölçüm. Medyan/p95/p99/maksimum, konum hatası ve yanlış kabul birlikte raporlanır. CUDA tamamlanma senkronizasyonuyla kernel ve uçtan uca transferli süre ayrı ölçülür. Hedef p95 <10 ms, sıkı hedef p95 <5 ms; tam kare/ROI ve kernel/cycle ayrı etiketlenir.

## Yerel ölçüm sonucu ve karar

Release derleme ve benchmark başarılı. Intel i5-13500HX, OpenCV varsayılan 20 thread; mevcut işletim sistemi yükü/termal durum sabitlenmedi. Her senaryoda sentetik kopyanın konumu 100/100 doğru bulundu.

| Yöntem / şablon | Arama | Medyan ms | p95 ms |
| --- | --- | --- | --- |
| TM_CCORR / 32 | 5 MP tam kare | 72.58 | 84.76 |
| TM_CCOEFF_NORMED / 32 | 5 MP tam kare | 105.30 | 136.56 |
| TM_CCORR / 128 | 5 MP tam kare | 116.19 | 183.16 |
| TM_CCOEFF_NORMED / 128 | 5 MP tam kare | 143.67 | 158.32 |
| TM_CCOEFF_NORMED / 32 | 512×512 ROI | 7.27 | 9.03 |
| TM_CCOEFF_NORMED / 128 | 512×512 ROI | 5.66 | 6.09 |
| TM_CCORR / 128 | 512×512 ROI | 4.35 | 5.07 |

**Tam kare CPU: altı yöntemin hiçbiri <10 ms hedefini sağlamadı.** 512×512 ROI üzerinde seçilen TM_CCOEFF_NORMED her iki boyutta p95 <10 ms verdi; p95 <5 ms vermedi. En hızlı medyanın 5 ms altında olması p95 hedefinin sağlandığı anlamına gelmez. Aynı görüntünün tekrarlı/sıcak-cache ölçümü üretim jitter ve yanlış kabul testinin yerine geçmez.

Karar: CPU adapter TM_CCOEFF_NORMED ve reçeteden verilen dar search ROI ile başlanır. Tam kare zorunluysa CUDA adayı gerçek hedef donanımında ölçülür veya pyramid/coarse-to-fine tasarımı ayrıca uygulanıp doğrulanır. CUDA hız hedefi sağlanmış olarak kaydedilmez. Angle/scale araması henüz uygulanmadığı için ürünün genel Pattern Match hızına dair sonuç çıkarılmaz.

## Resmi kaynaklar

- [Template matching API ve formüller](https://docs.opencv.org/4.13.0/df/dfb/group__imgproc__object.html)
- [CUDA createTemplateMatching](https://docs.opencv.org/4.13.0/d0/d05/group__cudaimgproc.html)
- [CUDA TemplateMatching](https://docs.opencv.org/4.13.0/d2/d58/classcv_1_1cuda_1_1TemplateMatching.html)
- [Shape API](https://docs.opencv.org/4.13.0/d3/dc0/group__imgproc__shape.html)
- [GeneralizedHoughGuil](https://docs.opencv.org/4.13.0/d3/d20/classcv_1_1GeneralizedHoughGuil.html)

Kaynaklar OpenCV 4.13 API belgeleridir; yerel 5.0 kurulumu için CPU kodu ayrıca derlenerek doğrulanır. CUDA 5.0 API uyumluluğu bu aşamada doğrulanmadı.
