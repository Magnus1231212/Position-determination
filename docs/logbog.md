# Logbog – Indendørs Positionsbestemmelse

## Gruppe

Magnus og Asbjørn

## Formål

Projektet går ud på at bestemme positionen af en mobil indendørs ved hjælp af WiFi-signalstyrke (RSSI) og triangulering med 3 ESP32-enheder, uden brug af GPS.

---

## Arkitektur

Systemet består af 3 ESP32-enheder placeret i hjørnerne af et område. Hver ESP32 kører den samme kode, men med forskellige koordinater i `config.h` så vi ved hvilken enhed der har målt hvad.

### Dataflow

1. **Sniffe-fase** – ESP32 sættes i promiscuous mode og lytter efter WiFi-pakker i 10 sekunder. MAC-adresser og RSSI gemmes i hukommelsen og grupperes per enhed
2. **Beregningsfase** – RSSI gennemsnittes per enhed, MAC hashes med MD5, og afstand beregnes
3. **Sende-fase** – ESP32 forbinder til WiFi og sender data til MQTT-brokeren
4. **Genstart** – ESP32 afbryder WiFi og starter forfra med en ny sniffe-fase

### MQTT-besked format

Hver besked indeholder:

```json
{
  "id": "hash af MAC-adresse",
  "rssi": -65,
  "distance": 1.58,
  "x": 0,
  "y": 0,
  "time": "2026-05-27T10:33:51"
}
```

---

## Dataprocess

### Primær dataopsamling

ESP32 kører i WiFi promiscuous mode og opfanger alle WiFi-pakker i nærheden. Fra hver pakke udtrækker vi afsenderens MAC-adresse og RSSI-værdien (signalstyrken).

### Databerigelse

Rå data bearbejdes på ESP32'en inden afsendelse:

- **Gennemsnitlig RSSI** – flere målinger fra samme enhed slås sammen til et gennemsnit for at reducere støj
- **Afstandsestimering** – RSSI konverteres til en estimeret afstand i meter med formlen `distance = 10 ^ ((txPower - rssi) / (10 * n))`
- **Tidsstempel** – hentes fra NTP-server og tilføjes hver besked
- **Hashing** – MAC-adressen erstattes med en MD5-hash inden afsendelse

### Beregning

Når alle 3 ESP32-enheder har målt afstanden til samme enhed, kan positionen beregnes geometrisk med trilaterering. Med kendte koordinater for hver ESP og en estimeret afstand fra hver, kan man opstille et ligningssystem og finde det punkt der passer bedst med alle tre afstande.

### Datalagring

Data sendes til en lokal MQTT-broker og kan herfra udtrækkes af en ekstern applikation til visualisering, fx et heatmap over hvor enheder befinder sig i området.

---

## GDPR og persondata

### Hvilke data indsamler vi?

- MAC-adresser fra WiFi-enheder i nærheden
- RSSI (signalstyrke)
- Tidspunkt
- Estimeret position

### Er det persondata?

MAC-adressen kan identificere en specifik enhed, og dermed indirekte en person. Det er derfor persondata ifølge GDPR. Når MAC-adressen kombineres med tid og sted, beriges dataene og bliver mere følsomme.

### Hvad gør vi for at beskytte det?

- **MAC-adressen hashes med MD5** inden den sendes videre – den rå adresse gemmes aldrig
- Data sendes over **TLS-krypteret MQTT** (port 8883)
- Vi gemmer kun data så længe det er nødvendigt for beregningen

### Datasikkerhed

- Adgang til MQTT-brokeren kræver brugernavn og adgangskode, som ligger i `config.h` der er ekskluderet fra git via `.gitignore`
- MQTT-forbindelsen er krypteret med TLS og verificeres med et CA-certifikat, så trafikken ikke kan aflyttes
- ESP32'en gemmer kun data i RAM under sniffe-fasen og sletter det igen når det er sendt – data gemmes aldrig permanent på enheden

### Bemærkning

Nyere smartphones (iOS siden 2014, Android siden 2021) randomiserer deres MAC-adresse, hvilket betyder at den samme enhed kan optræde med forskellige MAC-adresser. Dette reducerer muligheden for at spore personer over tid.

---

## 26-05-2026 – Opstart og implementering

### Teknologier vi har undersøgt

- **GPS** – ikke egnet indendørs, signal for svagt og upræcist inde i bygninger
- **WiFi RSSI / promiscuous mode** – ESP32 kan lytte til alle WiFi-pakker i luften og måle signalstyrken (RSSI). Dette kræver at WiFi-modulet sættes i "promiscuous mode", hvor det modtager alle pakker uanset modtager
- **MQTT** – letvægtsprotokol til at sende data fra ESP32 til en central broker. Vi bruger en lokal MQTT-broker med TLS-kryptering på port 8883
- **Triangulering/trilaterering** – geometrisk beregning af position ud fra afstand til mindst 3 målepunkter med kendte koordinater

### Valgte teknologier

Vi valgte at bruge WiFi promiscuous mode på ESP32 til at sniffe MAC-adresser og RSSI, og sende data via MQTT til en lokal broker.

### Hvad vi lavede

- Satte PlatformIO og VS Code op med ESP32-projekt
- Implementerede WiFi promiscuous mode – første succesfulde output: MAC-adresser og RSSI i Serial Monitor
- Opdagede at ESP32 ikke kan sniffe og sende WiFi-data samtidig, da de bruger samme radio. Løsningen blev at ESP32 skifter mellem en sniffe-fase (10 sekunder) og en sende-fase
- Fletede sniffing-kode med MQTT-kode fra et gruppemedlem, som håndterede WiFi-forbindelse, TLS-certifikat og tidsstempler
- Tilføjede gennemsnitlig RSSI per MAC-adresse for at reducere støj
- Tilføjede NTP-tidssynkronisering og ISO-tidsstempler på hver MQTT-besked
- Implementerede MD5-hashing af MAC-adresser inden afsendelse, så rå persondata aldrig forlader ESP32'en
- Tilføjede afstandsberegning fra RSSI med formlen:
  `distance = 10 ^ ((txPower - rssi) / (10 * n))`
  hvor txPower = -59 dBm og n = 3.0 (indendørs miljøfaktor)
- MQTT-beskeden indeholder nu: hashet ID, gennemsnitlig RSSI, estimeret afstand, koordinater og tidsstempel

## Næste skridt

- Opsætte alle 3 ESP32-enheder med koordinater
- Implementere trilaterering
- Teste nøjagtighed af positionsbestemmelse

---

## 27-05-2026 – Triangulering og færdiggørelse

### Hvad vi lavede

- Opsatte alle 3 ESP32-enheder med hver deres koordinater i `config.h`:
  - ESP 1: x=0, y=0 (venstre hjørne)
  - ESP 2: x=0, y=0 (højre hjørne)
  - ESP 3: x=0, y=0 (midten øverst)
- Uploadede samme kode til alle 3 ESP'er
- Implementerede trilaterering – beregning af (x,y) position ud fra afstand fra 3 stationer
- Testede nøjagtighed af positionsbestemmelse på bordet

---
