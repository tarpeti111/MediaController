# Magas Szintű Tervezet (HLD) {#ch:hld}

A fejlesztett rendszer egy beágyazott hardveres vezérlőből
(`Silicon Labs Target HW`) és egy PC-oldali alkalmazásból
(`PC Companion App`) áll. A két entitás aszinkron módon, Bluetooth Low
Energy (`BLE`) hálózati rétegen keresztül kommunikál egymással egy
hálózati koprocesszor (*Network Co-Processor -- NCP*) segítségével.

A rendszer elsődleges célja, hogy a mikrovezérlőn található fizikai
bemeneti eszközökkel (joystick, nyomógombok) távvezéreljen a
számítógépen futó komplex modulokat (jelenleg a `Spotify Web API`
integrációt), miközben a PC-oldali alkalmazás dinamikusan
állapot-visszajelzéseket küld a hardver kijelzőjére.

## Modulok

### Mikrovezérlő (Target Hardver) Oldal

A mikrovezérlő szoftveres felépítése a `Simplicity SDK` komponenseire
épül, és egy nem-blokkoló, kooperatív Super Loop (főciklus)
architektúrát követ.

- **Bemeneti Kezelő** (`input.c/.h`): Folyamatosan mintavételezi a
  fizikai joystick és a nyomógombok (Start, Select) állapotát. Az
  észlelt állapotokat egy 2 bájtos tömbbe sűríti (`input_states`), ahol
  bitmaszkok segítségével reprezentálja a lenyomott irányokat és
  gombokat.

- **Target Állapotgép** (`model.c/.h`): A mikrovezérlő lokális állapotát
  vezérli (kapcsolódás, alkalmazás-választás, aktív futás). Kezeli a
  kurzorpozíciót és a memóriában tárolt eszköz- és alkalmazáslistákat.

- **Kijelző Vezérlő** (`display.c/.h`): A Silicon Labs hardverspecifikus
  `DMD` (*Dot Matrix Display*) és `GLIB` (*Graphics Library*)
  könyvtáraira épül. Feladata a menük, a MAC-címek, az alkalmazások
  ikonjainak és a futó média-interfész elemeinek (Play/Pause, Skip
  szimbólumok) kirajzolása.

- **BLE NCP Kliens** (`ble.c/.h`): Megvalósítja a GATT procedúrákat
  (szolgáltatás- és karakterisztika-felfedezés), kezeli a kapcsolat
  felépülését, és aszinkron módon fogadja az alkalmazások adatait.

### PC Companion Application Oldal

A PC-s alkalmazás feladata a logikai absztrakció, a külső API-kkal való
hálózati kommunikáció és a mikrovezérlő kiszolgálása.

- **BLE NCP Host** (`ble.c/.h`): A Silicon Labs NCP architektúráját
  használja. VCOM porton keresztül vezérli a PC-hez csatlakoztatott BLE
  hardvert, megvalósítva a GATT szervert.

- **Központi App Menedzser** (`model.c/.h`): Nyilvántartja a regisztrált
  alkalmazásokat, kezeli a BLE felé történő szinkronizációt, és a
  hardver felől érkező felhasználói választások alapján dinamikusan
  átkapcsolja a futási környezetet.

- **Képkonverzió és Interfész-definíció** (`app_builder.c/.h`):
  Tartalmazza a generikus alkalmazás-struktúrákat, valamint a
  hardverspecifikus ikonkonverziós algoritmust.

- **Spotify App Modul** (`spotify.c/.h`): Tartalmazza a teljes
  Spotify-specifikus logikát: beágyazott HTTP szervert indít az OAuth2
  autorizációs kód fogadására, kezeli a PKCE titkosítást, a tokenek
  frissítését (*Refresh Token*), a `cJSON` alapú szintaktikai elemzést
  és a lejátszás-vezérlést.

::: center
[]{#fig:rendszer_architektura label="fig:rendszer_architektura"}
:::

## Állapotgép

A mikrovezérlő egy belső állapotgép (`state_t`) szerint működik, ami
meghatározza a kijelző aktuális nézetét és az eszköz viselkedését.

- **state_init**: Kezdeti inicializálás (kijelző, joystick indítása),
  majd azonnali átlépés a kapcsolat-választásba.

- **state_connection_select**: A környezetben található BLE eszközök
  pásztázása és listázása. A bal oldali (`BTN_SELECT`) gomb
  megnyomásakor a kiválasztott hardverhez kapcsolódik (`ble_connect()`),
  és állapotot vált. A felhasználónak lehetősége van váltani egy olyan
  nézetre, ami MAC-címek szerint listázza az eszközöket a joystick
  középső (`JOY_C`) gombjának megnyomásával.

- **state_app_select**: Sikeres kapcsolódás után a PC elkezdi
  továbbítani a regisztrált alkalmazások adatait. A felhasználó a
  joystick és a gombok segítségével görgeti a listát, majd a
  `ble_write_app_choice()` meghívásával elindítja a kiválasztott
  alkalmazást.

- **state_app_running**: Az alkalmazás aktív futási fázisa. Ebben az
  állapotban a mikrovezérlő nem végez lokális logikai döntéseket, csupán
  a bemeneti események maszkját küldi folyamatosan értesítésként
  (`ble_notify_input_states`) a PC felé.

## Moduláris Alkalmazás-Architektúra

A PC Companion alkalmazás tervezésénél kiemelt szempont volt az
objektumorientált bővíthetőség biztosítása C nyelven, struktúrák és
függvénymutatók (*callbacks*) segítségével.

### Az extended_app_t Absztrakciós Réteg

Minden alkalmazásnak kötelezően implementálnia kell az `app_builder.h`
fájl által definiált interfészeket:

``` {title="Kibővített alkalmazásstruktúra"}
typedef struct {
  app_t app;                         // Nyers BLE adatok: ID, nev, nyers bitmap ikon
  app_on_app_launch_t on_app_launch; // Callback: Alkalmazas inditasakor
  app_on_input_t on_input;           // Callback: Hardveres gombnyomaskor
  app_on_app_process_action_t on_app_process_action; // Callback: Minden ciklusban
} extended_app_t;
```

### Az add_app Mechanizmus és a Fejlesztői Minták

Új szolgáltatás hozzáadása esetén a fejlesztőnek nem szükséges
módosítania a `model.c` állapotgépét vagy a BLE kommunikációs réteget.
Az új alkalmazás integrációja a `spotify.c` mintájára, lazán kapcsolt
módon történik:

- **Callbackek implementálása:** Az új modulban (pl. `youtube.c`) meg
  kell írni az életciklus-függvényeket (pl. hogyan reagáljon a YouTube
  modul a `JOY_E` léptetésre).

- **Szöveges ikon tervezése:** Az ikon emberi szemmel olvasható,
  textuális formátumban készíthető el a forráskódban:

  ``` {title="Szöveges ikon"}
  char youtube_image[] = "0000000011111111..."; // 0: fekete, 1: hatter
  ```

- **Regisztráció (`model_add_app`):** Az alkalmazás saját inicializáló
  függvényében regisztrálja magát a rendszer felé:

  ``` {title="Alkalmazás felvételi példa"}
  model_add_app("YouTube", youtube_image, youtube_launch, youtube_input, youtube_process);
  ```

### Az app_builder Valódi Felelőssége

A modul valódi feladata egy képkonverziós segédprogram biztosítása. A
`model_add_app` belsőleg meghívja az `app_builder_bitstringToBytes`
függvényt, ami:

- A szöveges `"0101"` karakterláncot tömörített, 128 bájtos nyers
  bináris tömbbé alakítja a BLE átvitel optimalizálásához.

- Elvégzi a bitek helyi tükrözését (`mirror_byte`). Erre azért van
  szükség, mert a PC-oldali memória-elrendezés és a beágyazott Target HW
  kijelzővezérlőjének (`GLIB/DMD`) hardveres bit-sorrendje (bájt-sorrend
  / *Endianness*) eltér.

## Kommunikációs Protokoll és Interfészek

A rendszerben két fő kommunikációs interfész működik: a lokális BLE és a
külső Web API.

### BLE GATT Karakterisztika Specifikációk

A PC és a mikrovezérlő közötti adatcsere dedikált `gatt_db` azonosítókon
keresztül zajlik:

- **gattdb_app_info_data** (Notification): A PC ezen keresztül küldi el
  a regisztrált alkalmazások listáját. Az adatcsomag szerkezetét a
  tömörített (*packed*) `app_t` struktúra határozza meg, így a
  mikrovezérlő oldalon a beérkező csomag típusmódosítással (*casting*)
  közvetlenül struktúrára képezhető, megkönnyítve a név és ikon
  értelmezését.

- **gattdb_app_choice** (User Write): Amikor a hardveren kiválasztanak
  egy alkalmazást, az MCU beírja az App ID-t ebbe a karakterisztikába. A
  PC-s BLE rétegben ez egy `sl_bt_evt_gatt_server_user_write_request_id`
  eseményt vált ki. Kritikus tervezési pont, hogy a PC-nek kötelező
  azonnal visszaküldenie a `sl_bt_gatt_server_send_user_write_response`
  nyugtát (`SL_STATUS_OK`), különben a hardveres BLE protokollverem
  időtúllépés miatt bontja a kapcsolatot.

- **gattdb_input_states** (Notification): Futás közben a hardver ezen a
  csatornán küldi a joystick és gombok aktuális bitmaszkját a PC-nek,
  ami azonnal továbbításra kerül az éppen aktív alkalmazás `on_input`
  callbackjének.

::: center
[]{#fig:ble_komm label="fig:ble_komm"}
:::

### Külső Spotify Web API és OAuth 2 Interfész

A PC-s `spotify.c` modul a külső felhőalapú szolgáltatással szabványos
HTTPS REST API-n keresztül tartja a kapcsolatot, a biztonságos `libcurl`
könyvtárat használva.

#### OAuth 2 + PKCE folyamat: {#oauth-2-pkce-folyamat .unnumbered}

Mivel a Companion App kliens-titka nem tárolható biztonságosan egy
lokális asztali alkalmazásban, a rendszer az **Authorization Code Flow
with PKCE** (*Proof Key for Code Exchange*) protokollt alkalmazza.

- Indításkor a modul generál egy titkos kódot (`Code Verifier`) és annak
  SHA-256 hash-ét (`Code Challenge`).

- A PC-n megnyílik a gyári böngésző a Spotify beléptető oldalával. A
  háttérben a szoftver elindít egy könnyű, lokális TCP socket szervert a
  3000-es porton (`http://127.0.0.1:3000/callback`).

- A sikeres böngészős bejelentkezés után a Spotify visszairányítja a
  tokent a lokális portra, amit a beágyazott szerver elkap, majd leáll.
