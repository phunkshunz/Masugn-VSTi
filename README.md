# MASUGN

Bred, glödande poly-synth som VST3 och AU för macOS, och VST3 för Windows. Phunkshunz AUDIO, serien **MASKINHALLEN**. Syskon till BETONG, ARMERING, FUKT, KOLV, SMUTS, VALV och SMIDET.

VALV är basmaskinen. MASUGN är ugnen: supervågor, rave-stabs, hoovers, mattor som andas, granulära moln och klockor. Allt är byggt från grunden: egna vågtabeller, egen motor.

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

## Det som gör MASUGN speciell

- **SMÄLT:** den stora ratten i mitten. Den glider från ljudet på panelen (A) till ljud B. Alla 48 ljud har ett eget B. Perfekt att automatisera i Bitwig.
- **BINAURAL:** varje ton har två halvor, en för vänster och en för höger öra, som svävar isär. Ljudet blir stort och levande direkt.
- **MUTATOR:** varje oscillator kan förvränga vågen med FM, Synk, Veck, Super eller Fasdiff.
- **HETTA:** ljudet matas tillbaka in i filtret. Tjockt, varmt och smutsigt.
- **SHIMMER:** rumsklang som växer en oktav uppåt, som glöd ur en ugn.

## Så funkar den

**Spela:** med MIDI från Bitwig eller ett keyboard, eller klicka på tangenterna längst ned. C3 = mitten-C, som i Bitwig.

Vila musen på en ratt eller väljare så visas en förklaring.

**GJUTFORMAR** (till vänster): 48 ljud i sex grupper. Klicka på en grupp, klicka på ett ljud. Du kan också bläddra med ◀ ▶ uppe i mitten. Ljuden syns också i Bitwigs preset-lista. Alla är volymjusterade så att de låter ungefär lika starkt.

| Grupp | Stil |
|---|---|
| RAVE | supervågor, stabs och hoovers |
| YTA | breda mattor som andas |
| MOLN | granulära moln och partiklar |
| KLANG | klockor, plock och glas |
| DRÖN | långa, glödande drönare |
| LEAD | leads som skär och sjunger |

**Uppe till höger:** SPELSÄTT (Poly, Mono, Bunden), NIVÅ-mätaren (blir röd när säkerhetsventilen tar i), GLID och VOLYM.

### SMÄLT
- **Ratten:** 0 % = ljud A (det du ser på panelen). 100 % = ljud B.
- **A → B:** kopierar panelens ljud till B. Skruva sedan på panelen och smält mellan det gamla och det nya.
- **BYT A/B:** B hamnar på panelen så att du kan skruva på det.
- **SLUMPA B:** gör ett nytt B nära ljudet på panelen.

### TÄRNINGAR
- **Stora tärningen** uppe i mitten: blandar två ljud ur samma grupp till ett nytt (B blandas också).
- **Vid varje panelrubrik** (OSC, FILTER, KUVERT, MOD och EFFEKT): slumpar bara den panelen.
- Tärningarna har gränser: resonans, hetta och rum kan aldrig gå så långt att det skriker eller skenar.
- **ÅNGRA:** backar senaste slaget, bytet eller ljudet, upp till 30 steg.

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

**MUTATOR** (Vågtabell, och Veck även på Analog) och **MUTATION** (hur mycket):

| Mutator | Låter |
|---|---|
| FM | skarp och metallisk |
| Synk | skrikig synk, svep den med kuvert 3 |
| Veck | vikt våg, varm och tjock |
| Super | sju vågor isär stämda, som en supersåg |
| Fasdiff | pulsbredd av vilken våg som helst |

**NIVÅ, OKTAV, TON och FIN** finns för varje oscillator. **BRUS**, **SPRIDNING** (ackord i stereo) och **BINAURAL** gäller alla tre.

Skärmen visar tabellen. Den gula vågen är där POSITION står, med mutatorn på.

### FILTER
- Två filter: **Ladder 24** (fet och rund), **LP 12**, **Band**, **HP**, **Notch**, **Kam** (metalliska, stämda toner) eller **Av**.
- **Serie / Parallell** uppe till höger: filter 1 och sedan 2, eller båda samtidigt.
- **ENV-MOD**, **TANGENT**, **DRIV IN** och **HETTA**.

### KUVERT
Tre rader: röd styr filtret, gul styr volymen, grå är **kuvert 3** som du kopplar vart du vill i MOD.

### LFO
Två LFO:er. Form: sinus, triangel, såg, fyrkant, slump eller mjuk slump. **TAKT** på = följer Bitwigs tempo. Med BINAURAL går LFO:n lite olika i vänster och höger öra.

### MOD
Fyra kopplingar. Välj **KÄLLA** (LFO, kuvert, anslag, modhjul, slump, tangent, aftertouch) och **MÅL** (position, form, mutation, nivå, ton, cutoff, resonans, brus, volym, panorering). Ratten till höger är hur mycket. Vänster om mitten = åt andra hållet.

Tips: **LFO 1 → OSC 1 pos** får tabellen att röra sig. **Kuvert 3 → OSC 1 mutation** med Synk ger klassiska synksvep.

### EFFEKT
**DRIV**, **KÖR**, **SHIMMER**, **RUM** med **STORLEK**, och **BREDD**.

## För utvecklare

`tools/MasugnTest.cpp` spelar alla ljud (både A och B) genom motorn, letar efter fel och mäter volymen. Volymerna i `src/Presets.h` kommer därifrån. `--bench` mäter hur tungt det är med åtta toner.
