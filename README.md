# VST3 Player Host

Standalone VST3 host/audio και MIDI player για Windows και macOS, βασισμένο στο JUCE.

## Λειτουργίες

- Φόρτωση και drag-and-drop αρχείων WAV/MP3/MIDI
- Play, pause, stop, seek και repeat/loop
- Output gain
- Τέσσερα σειριακά VST3 effect/instrument inserts και virtual MIDI keyboard
- Bypass, clear και άνοιγμα native/generic plugin editor
- Live stereo input μέσω ASIO στα Windows ή CoreAudio στο macOS
- Mono input sum προς τα δύο output channels
- Stereo input/output peak meters με latched overload indicator
- Ενιαία preset library με drop-down ονομάτων, plugin paths, πλήρες plugin state και bypass ανά insert

Όλα τα named presets αποθηκεύονται στο ίδιο αρχείο:
`%APPDATA%/CVA Labs/VST3 Player Host/preset-library.json`. Επιλογή ονόματος από το drop-down
φορτώνει άμεσα ολόκληρο το rack. Το **Save preset** προσθέτει νέο preset ή
ενημερώνει το υπάρχον όταν χρησιμοποιηθεί το ίδιο όνομα. Το **Delete** ζητά
επιβεβαίωση και αφαιρεί το επιλεγμένο preset.

Κάθε preset αποθηκεύει επίσης την πηγή: διαδρομή του φορτωμένου MP3/WAV ή
Live Input, πλήρες audio-device/ASIO setup (driver, κανάλια, sample rate και
buffer size), Mono, Repeat και output gain. Το αρχείο ήχου δεν ενσωματώνεται
μέσα στη library· πρέπει να παραμένει στην αποθηκευμένη διαδρομή.

Σε κάθε insert, το μεγάλο πεδίο εμφανίζει το όνομα του plugin και ανοίγει τον
editor του με click. Το διπλανό drop-down φορτώνει ή αντικαθιστά το VST3.
Η εφαρμογή σαρώνει σε background τις τυπικές τοποθεσίες VST3 και ομαδοποιεί
τα διαθέσιμα effect plugins ανά manufacturer/company. Το **Rescan VST3**
ανανεώνει χειροκίνητα τη λίστα. Η σάρωση γίνεται σε απομονωμένες child
processes, ώστε ένα ελαττωματικό plugin να μην κρασάρει τον host. Το αποτέλεσμα
αποθηκεύεται στο `%APPDATA%/CVA Labs/VST3 Player Host/vst3-cache.json`, οπότε μετά την πρώτη
σάρωση οι λίστες εμφανίζονται άμεσα.
Τα stereo INPUT/OUTPUT meters βρίσκονται δεξιά και λειτουργούν κατακόρυφα.

Η αλυσίδα σήματος είναι είτε
`Audio file -> Insert 1 -> Insert 2 -> Insert 3 -> Insert 4 -> Audio output`
είτε
`Live input -> Insert 1 -> Insert 2 -> Insert 3 -> Insert 4 -> Audio output`.

## Build (Windows)

Απαιτούνται Visual Studio 2022 ή 2026 με το workload **Desktop development with C++**, Git και CMake 3.22+.

```powershell
.\build-windows.ps1
```

Το executable δημιουργείται συνήθως στο:

`build/VST3PlayerHost_artefacts/Release/VST3 Player Host.exe`

Στο πρώτο configure το CMake κατεβάζει το JUCE 8.0.10 και τα headers του ASIO SDK.
Η χρήση/διανομή τους διέπεται από τις αντίστοιχες άδειες. Ειδικά το ASIO SDK
διατίθεται με επιλογή GPLv3 ή proprietary άδειας Steinberg· έλεγξε και επίλεξε
την κατάλληλη άδεια πριν διανείμεις binary.

## Χρήση

1. Πάτησε **Load audio** ή σύρε ένα WAV/MP3 στο παράθυρο.
2. Πάτησε **Load VST3** σε κάθε insert και επίλεξε το `.vst3` αρχείο του plugin.
3. Πάτησε **Editor** για το UI του plugin.
4. Πάτησε **Play**.

Για live ήχο, άνοιξε **Audio settings**, επίλεξε `ASIO` και τον driver της
κάρτας ήχου, ενεργοποίησε τα επιθυμητά input/output channels και μετά πάτησε
**Live input**. Χρησιμοποίησε ακουστικά ή χαμήλωσε τα monitors για αποφυγή feedback.
Ενεργοποίησε **Mono** για να γίνει `(Input L + Input R) / 2` και να σταλεί το
ίδιο σήμα και στα δύο κανάλια πριν από τα inserts.

Η εφαρμογή υποστηρίζει effects, instruments, MIDI input/MIDI files και αποθήκευση plugin state στα presets.

## Build (macOS Universal)

Απαιτούνται macOS 11+, Xcode Command Line Tools, Git και CMake 3.22+.

```bash
chmod +x build-macos.sh
./build-macos.sh
```

Παράγεται Universal εφαρμογή για Apple Silicon και Intel στο
`VST3-Player-Host-macOS-Universal.zip`. Το GitHub Actions workflow
`.github/workflows/build-macos.yml` εκτελεί το ίδιο build χωρίς να απαιτείται φυσικό Mac.
Το artifact είναι ad-hoc signed, όχι notarized. Για δημόσια διανομή απαιτείται
Apple Developer ID και notarization.
