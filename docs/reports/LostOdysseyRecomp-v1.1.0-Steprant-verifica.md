# Lost Odyssey Recomp 1.1.0 — verifica integrazione Steprant

Verifica del 3 novembre 2026. Branch: `integration/v1.1.0-steprant-all`.

**Esito aggiornato: APK completo del runtime di gioco compilato, firmato e verificato. Tutte le 19 PR interne e le quattro già incluse nella v1.1.0 risultano integrate. Mancano la prova d’installazione e il gameplay sullo Xiaomi 14; le verifiche statiche non li sostituiscono.**

## Base e provenienza

Il tag annotato ufficiale `v1.1.0` risolve al commit `fcfda1311f6e87a196124865afd2343508fb043f`. Il commit è antenato del ramo integrato. Tutte le 19 teste delle PR originarie sono antenate del ramo. Le quattro correzioni già accettate sono incluse nel tag tramite i rispettivi commit di merge/squash.

Un ramo importava anche `f9da0ae`, il commit successivo di Frank che documenta la release. Non aggiunge codice runtime. I suoi cinque file documentali/metadati sono stati riportati esattamente ai blob della v1.1.0. Il commit resta nella cronologia dei merge, conservata senza riscriverla. `main` resta sullo SHA `f9da0ae9bdd481a5ea4fe6431f3a4724d8261147`.

Il [manifest dei confronti](steprant-v1.1.0-pr-audit.json) registra per ogni file lo SHA del blob originale e di quello integrato. Nessun file dei contributi risulta mancante. I file condivisi differiscono perché riuniscono più contributi e sono stati controllati tramite diff e fixture di produzione. Le ulteriori modifiche sono verifiche e correzioni realizzate in questa attività.

## Le 19 PR interne

| PR interna | PR verso Frank | Funzione | Esito |
|---|---|---|---|
| [#1](https://github.com/Steprant/LostOdysseyRecomp/pull/1) | [#717](https://github.com/freefrank/LostOdysseyRecomp/pull/717) | SGSR1 | Presente, merge e file verificati |
| [#2](https://github.com/Steprant/LostOdysseyRecomp/pull/2) | [#718](https://github.com/freefrank/LostOdysseyRecomp/pull/718) | Descrittori texture | Presente, merge e file verificati |
| [#3](https://github.com/Steprant/LostOdysseyRecomp/pull/3) | [#719](https://github.com/freefrank/LostOdysseyRecomp/pull/719) | Telemetria termica | Presente, merge e file verificati |
| [#4](https://github.com/Steprant/LostOdysseyRecomp/pull/4) | [#720](https://github.com/freefrank/LostOdysseyRecomp/pull/720) | Descriptor-buffer probe | Presente, merge e file verificati |
| [#5](https://github.com/Steprant/LostOdysseyRecomp/pull/5) | [#721](https://github.com/freefrank/LostOdysseyRecomp/pull/721) | MediaCodec | Presente, merge e file verificati |
| [#6](https://github.com/Steprant/LostOdysseyRecomp/pull/6) | [#722](https://github.com/freefrank/LostOdysseyRecomp/pull/722) | Hash pipeline CPU | Presente, merge e file verificati |
| [#7](https://github.com/Steprant/LostOdysseyRecomp/pull/7) | [#723](https://github.com/freefrank/LostOdysseyRecomp/pull/723) | Budget ombre | Presente, merge e file verificati |
| [#8](https://github.com/Steprant/LostOdysseyRecomp/pull/8) | [#726](https://github.com/freefrank/LostOdysseyRecomp/pull/726) | Resolve dopo cambio impostazioni | Presente, merge e file verificati |
| [#9](https://github.com/Steprant/LostOdysseyRecomp/pull/9) | [#727](https://github.com/freefrank/LostOdysseyRecomp/pull/727) | Refill XMA | Presente, merge e file verificati |
| [#10](https://github.com/Steprant/LostOdysseyRecomp/pull/10) | [#731](https://github.com/freefrank/LostOdysseyRecomp/pull/731) | Barriere Vulkan resolve | Presente, merge e file verificati |
| [#11](https://github.com/Steprant/LostOdysseyRecomp/pull/11) | [#732](https://github.com/freefrank/LostOdysseyRecomp/pull/732) | Draw depth in sola lettura | Presente, merge e file verificati |
| [#12](https://github.com/Steprant/LostOdysseyRecomp/pull/12) | [#733](https://github.com/freefrank/LostOdysseyRecomp/pull/733) | Copertura clear parziali | Presente, merge e file verificati |
| [#13](https://github.com/Steprant/LostOdysseyRecomp/pull/13) | [#736](https://github.com/freefrank/LostOdysseyRecomp/pull/736) | Trasferimenti EDRAM | Presente, merge e file verificati |
| [#14](https://github.com/Steprant/LostOdysseyRecomp/pull/14) | [#738](https://github.com/freefrank/LostOdysseyRecomp/pull/738) | Confronto/switch guest, indagine 60 FPS | Presente, merge e file verificati |
| [#15](https://github.com/Steprant/LostOdysseyRecomp/pull/15) | [#739](https://github.com/freefrank/LostOdysseyRecomp/pull/739) | TV HDMI | Presente, merge e file verificati |
| [#16](https://github.com/Steprant/LostOdysseyRecomp/pull/16) | [#745](https://github.com/freefrank/LostOdysseyRecomp/pull/745) | Tester Android | Presente, merge e file verificati |
| [#17](https://github.com/Steprant/LostOdysseyRecomp/pull/17) | [#746](https://github.com/freefrank/LostOdysseyRecomp/pull/746) | Indici particellari rect-list | Presente, merge e file verificati |
| [#18](https://github.com/Steprant/LostOdysseyRecomp/pull/18) | [#748](https://github.com/freefrank/LostOdysseyRecomp/pull/748) | Trasferimento HDR | Presente, merge e file verificati |
| [#19](https://github.com/Steprant/LostOdysseyRecomp/pull/19) | [#756](https://github.com/freefrank/LostOdysseyRecomp/pull/756) | Abilità mancanti | Presente, merge e file verificati |

## Quattro PR già presenti nella release

| PR | Correzione | Merge ufficiale |
|---|---|---|
| [#707](https://github.com/freefrank/LostOdysseyRecomp/pull/707) | fix(renderer): bound driver cache exports and contain allocation errors | `57feee5e3987de236d70ed9d760191a1aa245846` |
| [#709](https://github.com/freefrank/LostOdysseyRecomp/pull/709) | fix(vulkan): provide a stage mask for every generic submission wait | `e4dc49b29a719d68975dadfbb0395183c436396b` |
| [#711](https://github.com/freefrank/LostOdysseyRecomp/pull/711) | fix(vulkan): propagate texture image and default-view creation failures | `f26d829f59903e3760d33f0df115aeff9cfd05bb` |
| [#712](https://github.com/freefrank/LostOdysseyRecomp/pull/712) | fix(audio): wrap XMA output ring cursors before space checks and writes | `ca189df1b19f38d95929d93b13227a43af1a885d` |

## Problemi individuati e correzioni

La risoluzione manuale TV/Tester aveva correttamente separato gli ID in produzione, ma lasciava il test Android sul vecchio ordine delle righe e sui vecchi comandi. La fixture originale falliva con «Android diagnostics follows shader preparation». Ora verifica la riga TV 12, la diagnostica 13, il salvataggio 14, l’apertura del dialogo TV e registrazione/arresto/esportazione del Tester. Gli ID Java/native sono rispettivamente `0x8000` cartella, `0x8001` TV, `0x8002` avvio Tester, `0x8003` arresto, `0x8004` esportazione. Anche i target Android/Tester della suite menu ricevono ASan/UBSan quando richiesti.

Il compilatore di test HDR cercava soltanto la vecchia stringa HLSL `source` e falliva con `KeyError` dopo l’integrazione SGSR1, che la suddivide in `sourcePrefix` e `sourceSuffix`. Ora ricostruisce il sorgente predefinito come il runtime. Verificati DXIL/SPIR-V, readback Vulkan e identità binaria SDR rispetto alla v1.1.0.

Aggiunti test per cambio/sgancio TV durante una registrazione a 30/60/90/120 FPS, cinque ID distinti, sei firme JNI generate da javac e trace ZIP Java. Controllate le risoluzioni manuali di renderer, XMA, menu, RuntimeActivity e registrazione CMake: i contributi condivisi restano presenti. Le sorgenti diagnostiche e MediaCodec sono incluse dal GLOB del runtime.

Il workflow APK accetta ora un input privato autorizzato del fork e si ferma con un errore esplicito quando non è configurato. Il ramo d’integrazione seleziona il pacchetto release `1.1.0-Steprant`, il nome richiesto e un’identità installabile accanto all’app ufficiale. Sono stati eseguiti sul vero APK finale: firma con apksigner, allineamento ZIP/ELF a 16 KiB, integrità ZIP, manifest/DEX, otto librerie ARM64, nove esportazioni JNI dell’app e assenza di input/asset di gioco originali. Aggiunte le licenze del progetto, SDL, Plume e DXC al pacchetto. Il messaggio di errore del driver GPU usa il nome del pacchetto effettivo per indicare la cartella dei log.

## Risultati dei test

| Suite | Esito |
|---|---|
| review_regressions | 65/66 superati; 1 timeout per namespace non disponibili |
| settings_menu | 13/13 superati |
| Android Java | 82/82 superati |
| movie | 3/3 superati |
| HDR/presentation Vulkan | 6/6 superati |
| rect_list Vulkan | 2/2 superati |
| SGSR1 | Superata |
| HDR shader baseline | Superata |
| Python discovery | 44/44 superati |
| Android thermal | 9/9 superati |
| TV/touch/layout/pacing | Superata |
| Java/native integration | Superata |
| NDK shadow hooks | Superata |
| guest comparison switch | 1/1 superati |
| frame generation core | 1/1 superati |
| FG CPU contracts | 38/43 superati; 5 fallimenti per socket privati non disponibili |
| fast-forward audio | 1/1 superati |
| private input selection | 4/4 superati |
| Gradle/lint release | Superata; 60 avvisi, nessun errore |
| full Android runtime/APK | Compilazione e verifiche pacchetto superate |

Dettagli, conteggi e limiti nel [registro dei risultati](steprant-v1.1.0-validation.json). La prova HDR principale comprende 151 casi e 185.922 confronti RGB senza errori su Mesa lavapipe 25.2.8. Sono passate 32 compilazioni HDR/SDR DXIL/SPIR-V e otto varianti SGSR1. Gli otto binari SDR congelati sono identici alla release ufficiale. I readback Vulkan usano una GPU software: non misurano prestazioni Adreno o luminanza di uno schermo HDR reale.

Java: risorse compilate/collegate con aapt2 ufficiale, 38 sorgenti Java/SDL/R compilati con SDK 35 e JDK 17 e 82 test JUnit superati. La patch HID SDL ufficiale è applicata. Anche la successiva build Gradle release completa supera gli stessi 82 test (nessun errore, fallimento o test saltato) e lint. I 60 avvisi lint sono conservati nel conteggio: riguardano API/SDK, risorse, accessibilità e possibili riferimenti statici, fra cui SDL; non sono stati disabilitati per completare la build. Il Tester conserva il solo application context.

I sei fallimenti ambientali restano dichiarati: un test Linux arriva al timeout dopo aver segnalato l’impossibilità di creare namespace, cinque test Discord falliscono creando socket privati. Non sono stati rimossi o modificati per ottenere un verde. I test di questa tabella possono sovrapporsi tra suite.

## Stato effettivo della build Android

L’utente ha fornito il `default.xex` necessario. Il digest corrisponde esattamente al disco 1 USA/Europa supportato: `175ae53d109d480a83bebbd186e7b6871f7b03ce80af69ab388db2f747640de3`. L’input è rimasto privato. Il percorso ufficiale `fetch_build_input.py` → `xexdump` → XenonRecomp ha generato le 246 unità PPC, poi compilate e collegate nel runtime effettivo insieme a Vulkan, HDR, audio XMA, MediaCodec, menu TV e Tester. Nessuna funzione è stata rimossa per compilare.

Configurazione nativa Release: NDK ufficiale `28.2.13676358` (r28c), `arm64-v8a`, API 26, `c++_shared`; `LO_BUILD_ANDROID_RUNTIME`, `LO_BUILD_RUNTIME`, `LO_BUILD_RECOMP_LIB` e `LO_BUILD_GPU` abilitati. Tool host generati con Clang 18. Dipendenze e overlay seguono i pin del progetto. La build completa ha terminato con successo; ricompilazione finale e link eseguiti dopo il commit sorgente `1d102312609ed45117a4da28c86624ffa85d6b4f`. La revisione incorporata è `1d102312609e-dirty`: il suffisso segnala gli overlay ufficiali applicati nei submoduli, non sorgenti di progetto rimaste fuori dal commit.

Gradle `8.11.1`, AGP `8.9.3`, JDK 17, SDK/build-tools 35. La rete della JVM impediva i download: gli artefatti Maven esatti sono stati scaricati via HTTPS dai registri ufficiali in un mirror locale, poi risolti e usati dal vero Gradle in modalità offline. Non sono stati sostituiti compilatore Java, D8, aapt2, lint o task del runtime. Configurato su richiesta soltanto il modulo `runtime`, con download automatico SDK disabilitato. Un tentativo incrementale ha trovato copie temporanee duplicate nella cache DEX; la pulizia degli output e la build completa successiva hanno risolto il problema. Il comando finale `:runtime:clean :runtime:assembleRelease :runtime:testReleaseUnitTest :runtime:lintRelease` termina **BUILD SUCCESSFUL**, con 58 task eseguiti e uno aggiornato.

La dipendenza open source DXC ARM64 proviene esclusivamente dall’APK pubblico ufficiale v1.1.0 (SHA-256 `41ec5d557cc78fdc72feb00754a48e7b2851730c27344eaa1619f6478850877a`). È stata estratta solo `libdxcompiler.so`, verificata per ABI, dipendenze, API esportate e allineamento, conservando le relative licenze. Il suo digest distribuito è `f27d9850a3b43c26eac55a33119fdd519daac7b67c3f80515dd79cb1d08a51d7`; non viene dichiarato identico al diverso precompilato non ancora ulteriormente strippato. `libmain.so` e tutto il runtime del gioco sono stati ricostruiti dai sorgenti integrati e dal PPC generato dall’input fornito.

## APK prodotto

| Proprietà | Valore verificato |
|---|---|
| Nome | `LostOdysseyRecomp-v1.1.0-Steprant-Android-arm64.apk` |
| Dimensione | 47,680,318 byte (45.47 MiB) |
| SHA-256 APK | `b84e9d33042acb878f9b8bd2c24ff5f5197430ca2db26fada5746d3f81a0d598` |
| Sorgente del binario | [`1d10231`](https://github.com/Steprant/LostOdysseyRecomp/commit/1d102312609ed45117a4da28c86624ffa85d6b4f) |
| Pacchetto | `io.github.freefrank.lostodyssey.steprant` |
| Nome nell’elenco app | `Lost Odyssey Recomp Steprant` |
| Versione | `1.1.0-Steprant`, codice 1010000 |
| ABI / SDK | solo ARM64; min 26, target 35 |
| Firma | APK Signature Scheme v2, RSA 2048; corrisponde alla chiave privata conservata |
| SHA-256 certificato | `ecaadcdc715ff7e4aeaaf265ec11f1854d4039175dbc7e53fd4f86fd03af6f3f` |
| Integrità | ZIP, manifest, DEX e otto librerie native verificati |
| Pagine 16 KiB | allineamento ELF e verifica zipalign superati |
| Collegamento PPC/JNI | 62,614 entrate guest; 125,235 simboli guest incluse implementazioni/alias; nove esportazioni JNI applicative presenti |
| Asset originali / XEX / ISO | assenti |

Inventario completo delle librerie, digest, dipendenze, licenze e controlli nel [verbale APK](steprant-v1.1.0-apk-verification.json). È il runtime completo del gioco, non l’app diagnostica separata. I dati dei dischi restano necessari, come nell’app ufficiale, e non sono inclusi nel pacchetto.

L’identità distinta consente l’installazione accanto all’app ufficiale senza la sua chiave di firma. Namespace Java, nomi delle Activity e collegamenti JNI restano quelli originali. Selezionare una cartella di gioco condivisa accessibile oppure importare gli stessi dischi; per i salvataggi usare esportazione e importazione fra le due app. Le cartelle private non migrano automaticamente. La chiave del fork resta privata e va riutilizzata per aggiornare questa stessa app. Vedere le [istruzioni di installazione del fork](../notes/android-fork-installation.md).

## Pubblicazione e limiti residui

L’APK firmato è disponibile direttamente nella conversazione. Nessun XEX, sorgente PPC generato, file di gioco o chiave privata è stato pubblicato nel repository.

Le Actions del fork risultano sospese per il volume di utilizzo; l’[ultima build storica](https://github.com/Steprant/LostOdysseyRecomp/actions/runs/40409780694) falliva prima della generazione PPC cercando il repository privato di Frank. Questi ostacoli sono stati superati per la compilazione locale grazie all’input fornito. Il collegamento GitHub disponibile permette i commit e le PR, ma non il caricamento delle Release; il browser chiede l’autenticazione per la prerelease. Non è quindi dichiarato un artifact Actions o un asset Release che non esiste. Per ripetere la build nelle Actions, un manutentore deve riattivarle e configurare un input privato autorizzato come descritto nelle [istruzioni input](../notes/android-fork-build-inputs.md).

Non sono disponibili telefono, controller, TV o pannello HDR fisico: installazione effettiva, avvio e gameplay Android 16, prestazioni 60 FPS, throttling, ascolto XMA, decoder hardware, cambio reale della superficie HDMI e salvataggio/caricamento delle abilità restano da provare sul dispositivo. La verifica di firma, manifest, ABI e allineamento rende il pacchetto strutturalmente installabile, ma non certifica gameplay o prestazioni sullo Xiaomi 14.

Correzioni pubblicate nel [ramo d’integrazione](https://github.com/Steprant/LostOdysseyRecomp/tree/integration/v1.1.0-steprant-all): [`ad0fb6e`](https://github.com/Steprant/LostOdysseyRecomp/commit/ad0fb6e69f8d5aebcfcb2d4610d6495c518ac958), [`af2ff83`](https://github.com/Steprant/LostOdysseyRecomp/commit/af2ff833c645eb09898328488ab544eb9dfa0966), [`1d10231`](https://github.com/Steprant/LostOdysseyRecomp/commit/1d102312609ed45117a4da28c86624ffa85d6b4f). L’aggiornamento di questo rapporto è soltanto documentale: il binario corrisponde al commit sorgente indicato sopra.

Correzioni generali proposte a Frank con PR indipendenti: [#764 — input privati autorizzati nei fork](https://github.com/freefrank/LostOdysseyRecomp/pull/764), commit [`c215e8d`](https://github.com/Steprant/LostOdysseyRecomp/commit/c215e8dd78a59402b848f254d2f042131d168036); [#765 — identità installabile separatamente e licenze](https://github.com/freefrank/LostOdysseyRecomp/pull/765), commit [`6352b0b`](https://github.com/Steprant/LostOdysseyRecomp/commit/6352b0bd6228e324706a324a65027b5824553406). Le 19 PR originarie restano aperte e indipendenti. Entrambi i rami `main` sono stati ricontrollati e restano su `f9da0ae9bdd481a5ea4fe6431f3a4724d8261147`.
