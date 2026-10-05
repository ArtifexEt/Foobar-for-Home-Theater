# Six-height setup and validation: 5.1.6 / 7.1.6 / 9.1.6

## English

### Scope and rendering model

These components process **foobar2000 audio** and generate their own diagnostic tones. They do not change the Windows mixer or audio from other players or games, including Resident Evil 2 and Cyberpunk 2077. Launching a game cannot validate this DSP. Use the output plugin’s one-shot tests, music played through foobar2000, or the bundled standalone diagnostics.

Top Front and Top Back use four static Spatial Audio channels. Top Middle left/right use two stationary dynamic objects. The full DSP offers `5.1.6`, `7.1.6`, and `9.1.6`; `Add Ceiling Speakers` offers `6 speakers (dynamic Top Middle)` while preserving the incoming bed. A 9.1.6 stream also needs two front-wide objects.

The DSP synthesizes missing heights from PCM. The output component submits the resulting channels as static and positioned dynamic objects. The DSP does not decode an Atmos movie or recover the original objects from a flattened mix.

Automated tests verify PCM processing, masks, profile compatibility, and routing decisions. Physical speaker routing must be checked with your endpoint and AVR; software tests do not establish that result. Microsoft exposes arbitrary dynamic positions, and the AVR renderer decides which physical speakers reproduce those positions. A successful test command or an Atmos indicator alone is insufficient evidence of discrete Top Middle routing. [Microsoft Spatial Sound](https://learn.microsoft.com/en-us/windows/win32/coreaudio/spatial-sound) · [Dolby home-theater rendering](https://professional.dolby.com/siteassets/tv/home/dolby-atmos/dolby-atmos-for-home-theater.pdf)

### Install and choose the endpoint

1. Download and extract the latest Windows ZIP from [GitHub Releases](https://github.com/ArtifexEt/Foobar-for-Home-Theater/releases/latest). It includes the components, standalone diagnostics, and configuration profiles used below.
2. Save your existing DSP/output profiles and keep the previous component installers for comparison or rollback. Install **all three matching components** from `components/`: `foo_dsp_spatial`, `foo_dsp_height`, and `foo_out_spatial_audio`. Restart 64-bit foobar2000 2.x.
3. Configure the AVR for your actual 5.1.6, 7.1.6, or 9.1.6 room, with Top Front, Top Middle, and Top Back/Rear pairs assigned correctly. Select the HDMI/eARC endpoint and enable **Dolby Atmos for home theater** in Windows.
4. Make that endpoint the **Windows default audio device**. Preferences **Probe endpoint** and all one-shot tests use the default multimedia device. Select the same device under foobar2000 **Playback > Output**; choosing a named foobar2000 device alone does not retarget the Preferences tests.
5. Start at a low AVR listening volume. Stop normal playback before the one-shot tests; pausing can leave its dynamic objects reserved. For diagnosis, record the AVR sound mode and any upmixer/virtualizer settings, and use a mode that lets you distinguish renderer routing from additional processing.

### Probe before playback

Open **Preferences > Playback > Output > Spatial Audio Output**. Use `48000 Hz` initially. Temporarily select the intended .6 output layout, then press **Probe endpoint** and save the report.

Confirm the displayed endpoint name, support for the requested static bed, and **Max dynamic objects ≥ 2** for 5.1.6/7.1.6, or **≥ 4** for 9.1.6. Object availability may change at runtime. In `Auto`, the probe cannot know the next incoming DSP mask and reports that requirements depend on the stream. Return output to **Auto (follow audio bed)** after probing.

The .6 path requires its static bed and both middle PCM channels. It reports an error if the layout is incomplete, required objects are unavailable, or an active required object becomes unavailable. It does not silently merge the middle channels into the four-height bed. To continue with a smaller layout, explicitly choose .4 in the DSP and keep output on Auto.

### Check the physical middle speakers first

1. In the output **Test** page, run **Top ML** only. Record every speaker that produces the tone: middle left, front/rear ceiling, opposite side, or none.
2. Run **Top MR** separately and record the same observations. Top Middle tests always use a dynamic object, regardless of the `Dynamic object` checkbox. A one-shot needs one object; passing it alone does not prove that a full .6 stream can reserve its entire object set.
3. Run Top Front left/right and Top Back left/right individually as a routing reference. Confirm that the middle results are distinguishable from these four positions.
4. Initial **Top Middle object position (m)** is half-width `0.8`, height `1.4`, front/back `0.0`: left `(-0.8, 1.4, 0)`, right `(0.8, 1.4, 0)`. Positive height is above you; negative front/back is forward, positive is behind. Adjust one field at a time if needed, apply it, and rerun both tests. Playback uses these same coordinates. The values describe virtual object positions, not direct speaker addresses or calibrated distance/delay settings.

The bundled standalone tests allow a named endpoint without changing the Windows default:

```powershell
.\SpatialAudioDiagnostics.exe --list-devices
.\SpatialAudioDiagnostics.exe --device "YOUR AVR NAME" --probe --config .\config\spatial_audio_profile.ini
.\SpatialAudioDiagnostics.exe --device "YOUR AVR NAME" --custom --config .\config\top_middle_left.ini
.\SpatialAudioDiagnostics.exe --device "YOUR AVR NAME" --custom --config .\config\top_middle_right.ini
```

Run the left and right commands separately. These profiles use the initial positions above, a three-second tone, and conservative gain. They are standalone-tool profiles, not foobar2000 DSP presets; changes in the plugin UI do not change these INI files.

### Check music and existing layouts

Choose one processing path:

- **Integrated:** add `Spatial Audio DSP`; select your 5.1.6, 7.1.6, or 9.1.6 layout. Begin with Reference mode, unity channel trims, and the limiter enabled. Stereo and surround sources can generate missing heights. Existing height signals, including silence, are retained before the configured channel gain/delay/limiter processing.
- **Existing upmixer:** use your usual surround DSP, then `Add Ceiling Speakers` set to six speakers, then the matching output. Existing samples remain unchanged; only missing ceiling channels are synthesized. `Top middle trim (dB)` adjusts newly generated middle channels independently of the other heights.

Leave output on **Auto**. Top Middle uses private PCM channel flags understood by these matching components. Put no channel-remapping or third-party DSP after the final spatial DSP that could discard these flags. A fourteen-channel count alone cannot distinguish 7.1.6 from 9.1.4.

Test stereo, 5.1, and 7.1 material. Check that all expected bed channels remain correctly routed, both middles have signal, left/right remain distinct, and no clipping or dropout appears. The full DSP should emit 12/14/16 PCM channels for 5.1.6/7.1.6/9.1.6 respectively. Then test an existing 7.1.4 source: existing heights must stay present while the middle pair is derived. Keep the output report and any console error.

Finally switch back to **7.1.4** or **four ceiling speakers** with output Auto. Confirm the original four-height playback still works and the .6 path no longer requests Top Middle objects. If you use front wides, also check 9.1.4: its two dynamic channels must remain front wides. Reload an old saved profile, restart foobar2000, and verify old layout/gain/delay/mapping settings plus the new saved middle positions.

### Controls, small windows, and listening comparison

- **Spatial Audio DSP > Channels:** Top middle left/right have separate gain and delay sliders, numeric fields, and polarity switches. Start with **0 ms** additional DSP delay; speaker distance compensation belongs in the calibrated AVR setup.
- **Add Ceiling Speakers:** height gain, Top middle trim, front difference, surround/rear feed, and center feed have sliders paired with numeric fields. The middle trim is enabled for six speakers and affects newly generated middle signals. This DSP has no per-channel delay control.
- **Spatial Audio Output > Layout:** half-width, height, and front/back position have sliders paired with numeric fields. **Test** also has gain and frequency sliders. Object coordinates do not replace speaker delay calibration.

Shrink the Preferences window and the resizable Add Ceiling Speakers popup. Scrollbars should appear only where content does not fit; check both axes, wheel scrolling, and Tab/Shift+Tab reaching the last middle controls and OK/Cancel. Enlarge the window again: the controls must return into view and unnecessary scrollbars disappear. Repeat at Windows display scaling 100%, 150%, and 200%, and when moving between monitors. Change a value with its slider, type a precise value, apply/save, and reopen to confirm persistence; Cancel in Add Ceiling Speakers must retain the previous preset.

The Windows CI tests instantiate the real dialog resources with the production scroll helper at native, 150%, and 200% **dialog font sizes**. They check overflow, control visibility and scroll reset. They do not replace the above test inside foobar2000 or a physical monitor DPI transition.

For the listening comparison use short percussion and speech, compare .4/.6 at matched perceived loudness, and try reducing Top Middle gain if the image becomes diffuse. Generated middle signals are related to the other ceiling signals; averaging prevents a simple per-channel sum overload but does not preserve total acoustic energy or eliminate room interference. Report a distinct delayed repeat separately from tonal coloration or a broader image.

### Report results

Include the component version (and build run URL for a source build), Windows version, GPU/HDMI audio driver, AVR model/firmware, connection path (direct HDMI or TV/eARC), physical speaker assignments, chosen DSP chain/layout, Windows spatial format, probe text, object coordinates, and each left/right test result. Say whether other ceiling speakers also played, and whether the symptom occurs in one-shot tests, music, or both. An AVR Atmos badge is useful context but not a routing measurement. If possible, include a short recording or the AVR's channel activity display. PS5 success is a useful comparison; it does not establish that the two software paths use the same rendering method.

## Polski

### Zakres działania i model renderowania

Te komponenty przetwarzają **dźwięk foobar2000** i generują własne sygnały diagnostyczne. Nie zmieniają miksera Windows ani dźwięku innych odtwarzaczy i gier, w tym Resident Evil 2 i Cyberpunk 2077. Uruchomienie gry nie sprawdza działania tego DSP. Użyj testów kierunkowych wtyczki wyjściowej, muzyki odtwarzanej w foobar2000 lub dołączonego narzędzia diagnostycznego.

Top Front i Top Back korzystają z czterech statycznych kanałów Windows Spatial Audio. Lewy i prawy Top Middle są dwoma nieruchomymi obiektami dynamicznymi. Pełny DSP obsługuje `5.1.6`, `7.1.6` i `9.1.6`, a `Add Ceiling Speakers` ma opcję `6 speakers (dynamic Top Middle)`, zachowującą istniejące kanały. Układ 9.1.6 potrzebuje dodatkowo dwóch obiektów front-wide.

DSP tworzy brakujące kanały wysokości z PCM. Komponent wyjściowy wysyła powstałe kanały jako obiekty statyczne i obiekty dynamiczne z określoną pozycją. DSP nie dekoduje filmu Atmos ani nie odzyskuje oryginalnych obiektów ze spłaszczonego miksu.

Testy automatyczne sprawdzają przetwarzanie PCM, maski kanałów, zgodność profili i decyzje routingu. Fizyczny routing trzeba sprawdzić na własnym urządzeniu wyjściowym i amplitunerze; testy oprogramowania nie potwierdzają tego wyniku. O tym, które głośniki zagrają dla danej pozycji obiektu, decyduje renderer amplitunera. Sam brak błędu lub napis Atmos na wyświetlaczu nie dowodzi poprawnego użycia środkowej pary.

### Instalacja i wybór urządzenia

1. Pobierz i rozpakuj najnowszą paczkę Windows ZIP z [GitHub Releases](https://github.com/ArtifexEt/Foobar-for-Home-Theater/releases/latest). Zawiera komponenty, narzędzia diagnostyczne i profile konfiguracji używane poniżej.
2. Zapisz obecne profile i zachowaj poprzednie instalatory. Zainstaluj **wszystkie trzy komponenty z tej samej paczki**: `foo_dsp_spatial`, `foo_dsp_height`, `foo_out_spatial_audio`. Uruchom ponownie 64-bitowy foobar2000 2.x.
3. Ustaw rzeczywisty układ głośników w amplitunerze: Top Front, Top Middle i Top Back/Rear. W Windows włącz **Dolby Atmos for home theater** dla właściwego wyjścia HDMI/eARC.
4. Ustaw amplituner jako **domyślne urządzenie audio Windows** i wybierz to samo urządzenie w **Playback > Output** w foobar2000. Przycisk **Probe endpoint** i testy w Preferencjach korzystają z domyślnego urządzenia multimedialnego Windows, nawet gdy normalne odtwarzanie ma wybrane inne wyjście po nazwie.
5. Zacznij od niskiej głośności i zatrzymaj muzykę przed testami kierunkowymi. Zapisz tryb dźwięku amplitunera oraz ustawienia dodatkowego upmixera/wirtualizacji, aby móc odróżnić ich działanie od routingu obiektów.

### Sprawdzenie możliwości i głośników

W **Preferences > Playback > Output > Spatial Audio Output** wybierz na początek `48000 Hz`. Na czas pomiaru ustaw docelowy układ .6 i użyj **Probe endpoint**. Sprawdź nazwę urządzenia, dostępność wymaganych kanałów statycznych oraz **Max dynamic objects ≥ 2** dla 5.1.6/7.1.6 albo **≥ 4** dla 9.1.6. Zapisz raport, po czym przywróć **Auto (follow audio bed)**. W Auto zapotrzebowanie zależy od strumienia dostarczonego przez DSP; probe nie zna przyszłej maski wejścia.

Jeśli brakuje kanałów statycznych, obu kanałów Top Middle lub wymaganych obiektów, odtwarzanie .6 zgłosi błąd. Utrata wymaganego obiektu podczas odtwarzania również zatrzyma ten strumień. Nie ma cichego złożenia Top Middle do czterech górnych kanałów. Aby wrócić do mniejszego układu, wybierz .4 w DSP i zostaw wyjście Auto.

1. Na stronie **Test** uruchom osobno **Top ML**. Zapisz, czy gra tylko środkowy lewy, także przedni/tylny sufitowy, przeciwna strona, czy żaden.
2. Powtórz osobno **Top MR**. Te dwa testy zawsze korzystają z obiektu dynamicznego. Każdy potrzebuje tylko jednego obiektu, więc udany test pojedynczy nie potwierdza jeszcze zasobów potrzebnych do pełnego układu .6.
3. Osobno sprawdź Top Front L/R i Top Back L/R jako punkt odniesienia. Nie oceniaj środkowej pary wyłącznie na podstawie obecności dowolnego dźwięku nad głową.
4. Domyślne **Top Middle object position (m)**: half-width `0.8`, height `1.4`, front/back `0.0`, czyli `(-0.8, 1.4, 0)` i `(0.8, 1.4, 0)`. Dodatnia wysokość oznacza górę; ujemne front/back — przód, dodatnie — tył. Zmieniaj jedną wartość naraz, zatwierdzaj i powtarzaj oba testy. To pozycje wirtualnych źródeł, nie bezpośrednie adresy głośników ani korekta odległości/opóźnienia. Odtwarzanie używa tych samych współrzędnych.

Możesz też użyć poleceń PowerShell z sekcji angielskiej. Zastąp `YOUR AVR NAME` fragmentem nazwy z `--list-devices`. Uruchamiaj profile `top_middle_left.ini` i `top_middle_right.ini` osobno. Każdy gra przez trzy sekundy z ostrożnym poziomem i domyślnymi współrzędnymi. Są to profile narzędzia diagnostycznego; ustawienia wtyczki nie zmieniają plików INI.

### Odtwarzanie i regresje

Wybierz jedną ścieżkę:

- **Pełny DSP:** `Spatial Audio DSP` z układem 5.1.6/7.1.6/9.1.6. Zacznij od Reference, zerowych korekt kanałów i włączonego limitera. Brakujące wysokości powstają ze stereo lub surround. Istniejące kanały wysokości, również ciche, są zachowane przed ustawioną korektą poziomu/opóźnienia/limiterem.
- **Dotychczasowy upmixer:** najpierw dotychczasowy DSP surround, potem `Add Ceiling Speakers` z sześcioma głośnikami, następnie zgodne wyjście. Istniejące próbki pozostają bez zmian. `Top middle trim (dB)` koryguje tylko nowo tworzoną środkową parę.

Wyjście pozostaw na **Auto**. Za końcowym DSP przestrzennym nie umieszczaj procesora, który przestawia kanały lub usuwa prywatne flagi Top Middle. Wszystkie komponenty muszą pochodzić z tej samej kompilacji. Sama liczba 14 kanałów nie odróżnia 7.1.6 od 9.1.4.

Sprawdź kolejno stereo, 5.1 i 7.1: kanały dolne, obie środkowe wysokości, rozróżnienie lewej/prawej strony, brak przesterowania i przerw. Pełny DSP powinien wysyłać odpowiednio 12/14/16 kanałów PCM dla 5.1.6/7.1.6/9.1.6. Sprawdź również materiał 7.1.4: jego istniejące wysokości mają pozostać, a środkowa para powstać z nich.

Na koniec wróć do **7.1.4** albo **czterech głośników sufitowych** i wyjścia Auto. Potwierdź działanie wcześniejszego układu bez żądania obiektów Top Middle. Jeśli używasz front-wide, sprawdź także 9.1.4: dwa obiekty nadal mają reprezentować szerokie kanały przednie. Wczytaj stary profil, uruchom ponownie foobar2000 i sprawdź zachowanie układu, poziomów, opóźnień, mapowania oraz nowych zapisanych pozycji.

### Suwaki, małe okna i porównanie odsłuchowe

- **Spatial Audio DSP > Channels:** Top middle left/right mają osobne suwaki poziomu i opóźnienia, pola liczbowe oraz przełączniki polaryzacji. Zacznij od **0 ms** dodatkowego opóźnienia DSP; korektę odległości głośników ustaw w skalibrowanym amplitunerze.
- **Add Ceiling Speakers:** poziom wysokości, Top middle trim, front difference, surround/rear feed i center feed mają suwaki połączone z polami liczbowymi. Środkowa korekta jest aktywna przy sześciu głośnikach i dotyczy nowo tworzonego sygnału. Ten DSP nie ma osobnej regulacji opóźnień kanałów.
- **Spatial Audio Output > Layout:** half-width, height i front/back mają suwaki i pola liczbowe. Na stronie **Test** suwaki regulują też poziom i częstotliwość tonu. Pozycja obiektu nie zastępuje kalibracji opóźnienia głośnika.

Zmniejsz okno Preferencji oraz okno Add Ceiling Speakers z możliwością zmiany rozmiaru. Paski przewijania powinny pojawić się tylko tam, gdzie zawartość się nie mieści. Sprawdź obie osie, kółko myszy oraz Tab/Shift+Tab: ostatnie kontrolki Top Middle i przyciski OK/Cancel muszą być dostępne. Powiększ okno: zawartość powinna wrócić na miejsce, a zbędne paski zniknąć. Powtórz przy skalowaniu Windows 100%, 150% i 200% oraz po przeniesieniu między monitorami. Zmień wartość suwakiem, wpisz dokładną liczbę, zapisz i ponownie otwórz ustawienia. Cancel w Add Ceiling Speakers ma zachować poprzedni preset.

Testy Windows CI tworzą rzeczywiste okna z zasobów komponentów i używają produkcyjnego mechanizmu przewijania przy bazowym, 150% i 200% **rozmiarze czcionki okna**. Sprawdzają przepełnienie, dostępność kontrolek i powrót po powiększeniu. Nie zastępują testu wewnątrz foobar2000 ani rzeczywistej zmiany DPI monitora.

Do porównania odsłuchowego użyj krótkich uderzeń perkusji i mowy. Wyrównaj odczuwaną głośność .4/.6; jeśli lokalizacja się rozmywa, zmniejsz poziom Top Middle. Sygnał środków jest powiązany z pozostałymi wysokościami. Uśrednianie ogranicza poziom pojedynczego kanału, ale nie zachowuje całkowitej energii akustycznej i nie usuwa interferencji w pokoju. Zgłoś osobno wyraźne opóźnione powtórzenie, zmianę barwy i poszerzenie sceny.

### Informacje do zgłoszenia

Podaj wersję komponentów (oraz link do uruchomienia dla kompilacji ze źródeł), wersję Windows i sterownika HDMI/GPU, model i firmware amplitunera, połączenie bezpośrednie HDMI lub przez TV/eARC, przypisania głośników, łańcuch DSP, wybrany układ, format przestrzenny Windows, pełny raport probe, współrzędne i wyniki osobno Top ML/Top MR. Napisz, które inne głośniki grały oraz czy problem dotyczy testów, muzyki, czy obu. Dołącz ewentualny błąd konsoli. Pomocne są nagranie lub wskaźniki aktywnych kanałów amplitunera. Działanie PS5 jest przydatnym porównaniem, ale nie dowodzi identycznego sposobu renderowania w Windows.
