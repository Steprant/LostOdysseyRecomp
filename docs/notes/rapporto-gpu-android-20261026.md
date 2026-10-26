# Lost Odyssey Recomp — interventi GPU Android del 26 ottobre 2026

Implementati tre interventi indipendenti, basati sul `main` ufficiale
`f9da0ae9` successivo alla pubblicazione della 1.1.0. Le tre proposte sono nel
fork Steprant e sono aperte come PR draft upstream. Nessuna modifica è stata
integrata nel ramo principale ufficiale o nel `main` del fork.

| PR | Lavoro eliminato | Condizioni di sicurezza |
|---|---|---|
| [731](https://github.com/freefrank/LostOdysseyRecomp/pull/731) | Transizioni verso COPY_DEST e ritorno per resolve già riutilizzabili | Stessa copia consecutiva, nessun clear pendente, invalidazioni originali conservate |
| [732](https://github.com/freefrank/LostOdysseyRecomp/pull/732) | Draw di sola profondità incapaci di scrivere un attachment | Modalità 5, scritture Z e stencil disabilitati, nessuna query di occlusione attiva |
| [733](https://github.com/freefrank/LostOdysseyRecomp/pull/733) | Rettangoli di clear sovrascritti o già coperti | Copertura completa dimostrata, aspetti colore/profondità/stencil indipendenti, valori confrontati per bit |

La risoluzione, gli effetti, le texture, i formati e la precisione degli shader
restano quelli configurati dall'utente. Non sono state introdotte euristiche
basate sull'apparente invisibilità degli oggetti. I draw con query attive o
stencil abilitato rimangono nel percorso originale.

## Risparmio verificato e limiti

| Verifica | Prima | Dopo | Significato |
|---|---:|---:|---|
| 4.000 resolve ripetuti nel test di registrazione | 8.000 transizioni della destinazione | 0 | La copia era già evitata dal codice originale. Non si attribuiscono nuove copie risparmiate alla PR 731 |
| Ogni draw ammesso dal filtro della PR 732 | 1 draw inviato | 0 | Il numero di questi draw nelle scene reali è ancora da misurare |
| 4.000 clear identici di un rettangolo 4×4 | 4.000 rettangoli, 64.000 scritture di pixel nel modello | 1 rettangolo, 16 scritture | Il codice precedente li riuniva già in una sola chiamata API |
| Stesse 10.000 sequenze casuali contro la coda già ottimizzata upstream | 682.576 rettangoli | 680.254 | 2.322 rettangoli ulteriormente eliminati |
| Scritture di aspetti nelle medesime sequenze | 23.037.022 | 22.996.037 | 40.985 scritture ulteriormente eliminate, con gli stessi bit finali |

Questi sono conteggi verificati di comandi o del modello di immagine. Non sono
misure di bandwidth, frametime o frequenza dei casi nel gioco. Non è stato
misurato un numero di render pass eliminati. Tempi GPU/CPU, incremento FPS,
consumi e temperature sullo Xiaomi 14 restano sconosciuti. L'obiettivo
1080p/60 FPS non è stato dimostrato.

## Test effettuati

Le fixture specifiche passano 33.409 controlli per i resolve, 1.179.666 per il
filtro dei draw e 1.751.629 per i clear. Sono passate in Release e con
AddressSanitizer/UndefinedBehaviorSanitizer. Tutti e tre i nuovi casi mirati
falliscono con il rispettivo codice upstream originale. La verifica dei clear
confronta ogni componente colore, profondità e stencil per bit anche alle
letture intermedie, includendo zero con segno e valori NaN.

Il test Vulkan dei resolve FP16 ha eseguito sei letture complete delle immagini,
con zero pixel differenti, comprese copie parziali, nuove allocazioni e
invalidazioni dopo clear. È stato usato Mesa lavapipe 25.2.8 su CPU. Non è una
prova su Adreno o Mali e non è un benchmark del gioco.

Per ogni PR il workflow Runtime review regressions termina con 49 test passati
e uno saltato, su 50 casi registrati. Passa anche il controllo Windows che
compila i veri renderer, video e bridge e collega gli adapter selezionati.
Riferimenti:

| PR | Test CI | Compilazione Windows |
|---|---|---|
| 731 | [49 passati, 1 saltato](https://github.com/Steprant/LostOdysseyRecomp/actions/runs/40275532936) | [Passata](https://github.com/Steprant/LostOdysseyRecomp/actions/runs/40275532916) |
| 732 | [49 passati, 1 saltato](https://github.com/Steprant/LostOdysseyRecomp/actions/runs/40276842545) | [Passata](https://github.com/Steprant/LostOdysseyRecomp/actions/runs/40276842550) |
| 733 | [49 passati, 1 saltato](https://github.com/Steprant/LostOdysseyRecomp/actions/runs/40276848672) | [Passata](https://github.com/Steprant/LostOdysseyRecomp/actions/runs/40276848638) |

Sono passati anche i controlli FG CPU previsti per le prime due PR. In locale,
la combinazione dei tre interventi supera 49 CTest. I due test SDL sono stati
omessi localmente. Un test di importazione basato sui namespace Linux è stato
escluso dal confronto combinato perché l'ambiente lo impedisce e il primo
tentativo era andato in timeout. Lo stesso test risulta saltato anche in CI.
I due test SDL passano in CI.

Il renderer completo, con le tre modifiche insieme, compila con GCC 13 e con
Android NDK r28c/Clang 19 per arm64-v8a/API 26. Il percorso dei draw è stato
compilato anche con Clang 18 sul sistema ospite. Le tre fixture compilano e
collegano eseguibili Android AArch64. Le interfacce grafiche usate sono quelle
reali del progetto. Le verifiche del renderer sostituiscono i nomi guest/PCH
generati tramite la fixture di compilazione già esistente nel repository.

Non è stato prodotto o eseguito un APK completo del gioco: qui mancano i
sorgenti PPC generati richiesti da `LostOdysseyRecompLib/CMakeLists.txt`.
Non sono stati eseguiti test di gioco su Android, macOS o GPU fisiche.

## Compatibilità e ricognizione

Le tre modifiche si applicano insieme senza conflitti. Sono state controllate
le PR aperte riguardanti conservazione dei resolve, descriptor, hash pipeline e
ombre. Non vengono modificati decoder XMA, filmati, selezione delle ombre,
occlusione adattiva/history, trasferimenti delle costanti o cache persistenti.
La PR 733 estende la coda EDRAM esistente delle PR 414/445. Non ripropone la
sua ottimizzazione dei clear completi.

La ricognizione ha incluso command processor, DrawImpl, attachment e alias EDRAM,
resolve, transizioni Plume, shader/post-processing, costanti, descriptor,
cache e backlog. Non sono state conservate riscritture matematiche degli shader:
non c'erano misure che giustificassero una modifica del calcolo o della
precisione. La riduzione delle scritture dei clear affronta invece un costo di
memoria dimostrabile senza cambiare il calcolo dell'immagine.

Il backlog contiene misure storiche del TB321FU Adreno 750 in cui il limite a
caldo passava a CPU e scheduling termico. Queste misure non descrivono lo Xiaomi
14 a 1080p nella versione corrente. Confermano che percentuale GPU e numero di
draw non bastano a individuare la causa. Non è stato riaperto il vecchio piano
di fusione generalizzata dei pass che il manutentore aveva chiuso.

## Confronto ripetibile sul dispositivo

Il ramo del fork `perf/android-gpu-validated-20261026` raccoglie i tre interventi
per il confronto combinato. Le singole PR permettono di valutarli separatamente.
Le procedure dettagliate sono in [resolve](vulkan-resolve-barrier-reuse.md),
[draw](read-only-depth-draws.md) e [clear](pending-clear-coverage.md).

Usare build con gli stessi input e toolchain, stesso driver Turnip, risoluzione,
effetti, AA/upscaling e limite FPS. Scaldare le cache e alternare A/B/B/A nello
stesso punto di Uhra, in una battaglia fissa e nella stessa transizione CGI →
gioco, partendo da condizioni termiche comparabili. Registrare nelle due build
`LO_RENDER_TIMING=1`, `LO_RECORD_TIMING=1`, `LO_FRAME_TIMING=1` e
`LO_FRAME_STATS=10`. I contatori dei resolve e dei draw omessi mostrano se le
scene presentano davvero i casi ottimizzati. Per i clear confrontare i rettangoli
registrati tramite una cattura Vulkan.

Confrontare GPU queue timestamps, CPU e attese, intervalli p50/p95/p99, immagini,
memoria e almeno dieci minuti di frequenze/temperature. I timestamp della coda
non comprendono presentazione e compositor. I timer CPU di parete possono
sovrapporsi. Non sommare quei timer né mediare i percentili di finestre diverse
come se fossero il percentile dell'intera sessione. Tenere le catture dettagliate
fuori dalle misure prolungate e usare la stessa diagnostica nei due confronti.

Il passo successivo utile è identificare i pass dominanti sul dispositivo reale.
Il probe già esistente `LO_GPU_DRAW_TIMING_FRAME=N` misura gruppi di quattro draw
in un frame, includendo copie/barriere intermedie. Può indirizzare un'analisi di
shader, rasterizzazione o bandwidth senza dedurre il collo di bottiglia dalla
sola percentuale GPU. Vanno poi controllati Adreno/Mali, gli altri backend,
schermate con ombre, occlusione, HDR e ripresa dopo CGI prima di promuovere le PR.
I rallentamenti dei filmati e l'audio che gratta non sono dichiarati risolti.
