# MASUGN

Vågtabells-synth som VST3 och AU för macOS, och VST3 för Windows. Phunkshunz AUDIO, serien **MASKINHALLEN**. Syskon till BETONG, ARMERING, FUKT, KOLV, SMUTS, VALV och SMIDET.

Masugnen är där metallen smälts om. Här smälts ljud: vågtabeller som glider, partiklar som sprakar, resonatorer som ringer och FM som glöder. Inspirerad av stora digitala vågtabells-synthar, men allt är byggt från grunden: egna tabeller, egen motor.

Klicka på loggan uppe till vänster för att läsa om båtvarvet Docklands.

## Ladda ner

Mac: https://github.com/phunkshunz/Masugn-VSTi/releases/latest/download/MASUGN-macOS.zip

Windows: https://github.com/phunkshunz/Masugn-VSTi/releases/latest/download/MASUGN-Windows.zip

Ladda inte ner via den gröna knappen **Code**. Den ger bara källkoden, inte pluginet.

## Installera på Mac

Dubbelklicka på zip-filen i Hämtade filer. Öppna sedan Terminal och kör:

```
bash ~/Downloads/MASUGN/installera.command
```

Starta om Bitwig, eller tryck **Rescan** under Settings › Locations › Plug-ins. Sök på MASUGN.

## Installera på Windows

Högerklicka på zip-filen och välj **Extrahera alla**. Dubbelklicka sedan på **installera.bat** i mappen och svara Ja.

## Så funkar den

**Spela:** med MIDI från Bitwig eller ett keyboard, eller klicka på tangenterna längst ned. C3 = mitten-C, som i Bitwig.

**GJUTFORMAR** (till vänster): 55 ljud i sex grupper. Klicka på en grupp, klicka på ett ljud. Du kan också bläddra med ◀ ▶ uppe i mitten. Ljuden syns också i Bitwigs preset-lista. Alla är volymjusterade så att de låter ungefär lika starkt.

| Grupp | Stil |
|---|---|
| BAS | tunga basar ur tabeller och FM |
| LEAD | leads, svep och signaler |
| KLANG | klockor, plock, stabs och klaviatur |
| YTA | mattor, moln och långa ytor |
| RÖRELSE | tabeller som rör sig i takt |
| DJUP | udda, mörka och tunga ljud |

Vila musen på en ratt eller väljare så visas en förklaring.

### TÄRNINGAR
- **Stora tärningen** uppe i mitten: blandar två ljud ur samma grupp till ett nytt.
- **Vid varje panelrubrik** (OSC, FILTER, EFFEKT, KUVERT och MOD): slumpar bara den panelen.
- Tärningarna har gränser: resonans, eko och rum kan aldrig gå så långt att det skriker eller skenar.
- **ÅNGRA:** backar senaste slaget eller ljudbytet, upp till 30 steg.

### OSC
Tre oscillatorer. Byt mellan dem med flikarna **OSC 1, OSC 2, OSC 3**. En prick ● betyder att den hörs.

Varje oscillator har en **TYP**:

| Typ | POSITION-ratten | FORM-ratten |
|---|---|---|
| Vågtabell | glider genom tabellens 32 vågor | BÖJ: trycker ihop vågen |
| Partikel | var i tabellen kornen tas | TÄTHET: glesa långa korn eller täta korta |
| Resonator | MATERIAL: sträng → stav → klocka | KLANG: hur länge det ringer |
| Kärna (FM) | INDEX: hur mycket det vrids | RATIO: modulatorns tonhöjd |
| Analog | VÅG: såg → fyrkant | SKEV: sned, nasal våg |

**TABELL:** 12 egna vågtabeller: Grund, Puls, Synk, Veck, Vokal, Trappa, Glas, Spektrum, Metall, Orgel, Kvint och CZ Reso.

**NIVÅ, OKTAV, TON och FIN** finns för varje oscillator. **BRUS** och **BREDD** (stereo) gäller alla tre.

Skärmen visar tabellen. Den orange vågen är där POSITION står.

### FILTER
- Två filter: **Ladder 24** (fet och rund), **LP 12**, **Band**, **HP**, **Notch**, **Kam** (metalliska, stämda toner) eller **Av**.
- **KOPPLING:** Serie = filter 1 och sedan filter 2. Parallell = båda samtidigt.
- **ENV-MOD** (filterkuvertet), **TANGENT** (filtret följer tonhöjden) och **DRIV IN**.

### KUVERT
Tre rader: cyan styr filtret, orange styr volymen, grå är **kuvert 3** som du kan koppla vart du vill i MOD.

### LFO
Två LFO:er. Form: sinus, triangel, såg, fyrkant, slump eller mjuk slump. **TAKT** på = följer Bitwigs tempo.

### MOD
Fyra kopplingar. Välj **KÄLLA** (LFO, kuvert, anslag, modhjul, slump, tangent, aftertouch) och **MÅL** (position, form, nivå, ton, cutoff, resonans, brus, volym, panorering). Ratten till höger är hur mycket. Vänster om mitten = åt andra hållet.

Tips: **LFO 1 → OSC 1 pos** får tabellen att röra sig. **Kuvert 3 → OSC 1 pos** gör att varje ton sveper genom tabellen.

### SPEL
- **Poly:** ackord, åtta röster. **Mono:** en ton i taget. **Bunden:** överlappande toner glider utan ny attack.
- **GLID** och **VOLYM**.
- **NIVÅ** och **SÄKERHETSVENTIL:** ventilen gör att MASUGN aldrig kan skära sönder öronen.

### EFFEKT
**DRIV**, **KÖR** (bredare och mjukare), **EKO** i takt med tak så att det aldrig skenar, och **RUM**.

## För utvecklare

`tools/MasugnTest.cpp` spelar alla ljud genom motorn, letar efter fel och mäter volymen. Volymerna i `src/Presets.h` kommer därifrån.
