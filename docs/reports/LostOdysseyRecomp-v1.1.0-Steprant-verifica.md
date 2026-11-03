# Lost Odyssey Recomp 1.1.0 — verifica integrazione Steprant

Verifica del 3 novembre 2026. Branch: `integration/v1.1.0-steprant-all`.

**Esito: integrazione verificata tramite confronti dei file e test disponibili, correzioni pubblicate. APK completo non prodotto: mancano input PPC accessibili e le Actions del fork sono sospese. Non è una certificazione di funzionamento sullo Xiaomi 14.**

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

Il workflow APK accetta ora un input privato autorizzato del fork e si ferma con un errore esplicito quando non è configurato. Il ramo d’integrazione seleziona il pacchetto release `1.1.0-Steprant` e il nome richiesto. Dopo una build riuscita sono previsti controllo firma con apksigner, allineamento a 16 KiB, integrità ZIP, manifest/DEX, librerie ELF ARM64, assenza dei formati di input gioco e SHA-256. Queste verifiche sul pacchetto non sono state eseguite perché nessun APK è stato generato.

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
| Gradle/lint | Bloccata |
| full Android runtime/APK | Bloccata |

Dettagli, conteggi e limiti nel [registro dei risultati](steprant-v1.1.0-validation.json). La prova HDR principale comprende 151 casi e 185.922 confronti RGB senza errori su Mesa lavapipe 25.2.8. Sono passate 32 compilazioni HDR/SDR DXIL/SPIR-V e otto varianti SGSR1. Gli otto binari SDR congelati sono identici alla release ufficiale. I readback Vulkan usano una GPU software: non misurano prestazioni Adreno o luminanza di uno schermo HDR reale.

Java: risorse compilate/collegate con aapt2 ufficiale, 38 sorgenti Java/SDL/R compilati con SDK 35 e JDK 17 e 82 test JUnit superati. La patch HID SDL ufficiale è stata riprodotta. Il comando Gradle/lint resta bloccato dalla rete della JVM, anche usando la configurazione proxy dell’ambiente.

I sei fallimenti ambientali restano dichiarati: un test Linux arriva al timeout dopo aver segnalato l’impossibilità di creare namespace, cinque test Discord falliscono creando socket privati. Non sono stati rimossi o modificati per ottenere un verde. I test di questa tabella possono sovrapporsi tra suite.

## Stato effettivo della build Android

Configurazione reale con NDK ufficiale `28.2.13676358` (r28c), `arm64-v8a`, API 26 e runtime Android abilitato. CMake riconosce `android-aarch64` e si ferma in `LostOdysseyRecompLib/CMakeLists.txt:8` con `No recompiled sources in LostOdysseyRecompLib/ppc; run XenonRecomp first`. Nella cartella PPC pubblica è presente soltanto il segnaposto. Il controllo ufficiale `fetch_build_input.py` fallisce perché nell’ambiente non è configurato alcun input XEX.

Il repository `freefrank/LostOdysseyRecomp-build-inputs` restituisce 404 con il collegamento disponibile. Anche l’[ultima build APK storica](https://github.com/Steprant/LostOdysseyRecomp/actions/runs/40409780694) fallisce nel recupero di quel repository e salta generazione PPC e compilazione Android. Non dimostra lo stato odierno dei Secrets, che non sono stati letti.

La [pagina Actions del fork](https://github.com/Steprant/LostOdysseyRecomp/actions/workflows/android-apk.yml) mostra inoltre: «Workflows are not currently being run on this fork due to the scale of GitHub Actions usage. A repository maintainer can choose to re-enable these.» Il browser disponibile non ha una sessione autenticata e il connettore non espone la riattivazione/dispatch. Il nuovo push è pubblicato, ma non ha prodotto una nuova esecuzione remota.

**APK e link di download: non disponibili.** Non è stata pubblicata un’app diagnostica al posto del gioco, né rinominato l’APK ufficiale.

## Cosa manca per procedere

Un manutentore deve riattivare GitHub Actions sul fork. Per generare il codice di gioco serve una copia autorizzata del `default.xex` del disco 1: il workflow può riceverla tramite il Secret `LO_BUILD_XEX_URL`, oppure da un repository privato autorizzato con `LO_BUILD_INPUT_KEY` e le variabili `LO_BUILD_INPUT_REPOSITORY`/`LO_BUILD_INPUT_REF`. La compilazione locale potrebbe procedere anche senza Actions quando sia disponibile il medesimo input.

La libreria Android DXC può essere costruita dal sorgente pubblico quando manca il precompilato verificato. La chiave di firma ufficiale è facoltativa per una nuova installazione, ma una firma diversa non aggiorna in posto un APK già firmato con un’altra chiave. I passaggi sono descritti nelle [istruzioni per gli input privati](../notes/android-fork-build-inputs.md).

Non sono disponibili telefono, controller, TV o pannello HDR fisico: prestazioni 60 FPS, throttling, ascolto XMA, codec hardware, migrazione reale della superficie HDMI, salvataggio/caricamento delle abilità e accettazione sullo Xiaomi 14 restano da provare. I test sintetici non chiudono queste verifiche.

## Pubblicazione e PR

Prima correzione pubblicata: [ad0fb6e](https://github.com/Steprant/LostOdysseyRecomp/commit/ad0fb6e69f8d5aebcfcb2d4610d6495c518ac958). Tutte le correzioni e questo rapporto sono nel [ramo di integrazione](https://github.com/Steprant/LostOdysseyRecomp/tree/integration/v1.1.0-steprant-all).

Correzione generale proposta a Frank, su ramo indipendente da `main`: [PR #764 — ci(android): support authorized private inputs on forks](https://github.com/freefrank/LostOdysseyRecomp/pull/764), commit [c215e8d](https://github.com/Steprant/LostOdysseyRecomp/commit/c215e8dd78a59402b848f254d2f042131d168036). Le PR originarie verso Frank restano aperte e indipendenti. Nessun commit è stato applicato al suo `main`, né al `main` del fork.
