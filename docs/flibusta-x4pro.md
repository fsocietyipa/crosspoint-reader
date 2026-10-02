# CrossPoint Flibusta for Xteink X4 Pro

## What is included

The foundation is XPoint `b110d3e` (the exact source revision is recorded in Git
history), itself a CrossPoint fork. This branch adds:

- A token-free Flibusta OPDS preset, using `https://flibusta.is/opds`.
- One-time migration that retains existing catalogs, names, and credentials.
  The preset remains editable/deletable under Settings → OPDS. Deleting it is
  remembered. An existing `.is` or `.site` preset is retained without duplication.
- Correct handling of book entries containing related author links: unsupported
  PDF/FB2-only entries do not appear as misleading author-navigation rows.
- X4 Pro as the default build target and a distinct firmware version.
- SD-card-only firmware updates for this branch.

If all eight catalog slots are already occupied, migration leaves them alone.
Free one slot and restart to add the preset. An unreadable existing `opds.json`
is preserved instead of overwritten.

## Install on a reader already running CrossPoint

1. Back up the SD card, including the hidden `.crosspoint` directory (settings,
   reading position, and statistics). Keep your currently working X4 Pro image.
2. Copy the **X4 Pro** application image to the SD card as `firmware.bin`.
3. In CrossPoint, open Settings → System → SD Card Firmware Update, choose the image,
   and confirm the update. Keep the reader charged while updating.
4. After restarting, open OPDS Browser on Home. If the Plugins menu contains
   the catalog entry in your selected home layout, use OPDS Browser there.
5. Connect to Wi-Fi when prompted. Select Flibusta if a server picker appears.

Use the firmware's SD update menu; this file is an application image, not a
merged bootloader/partition image. Device flashing and physical testing are not
performed by the build process.

## Download books

The public catalog provides recent additions, authors, series, genres, and search.
Search first offers author/title categories; select the category and then a book.
Select Fetch to download an EPUB. PDFs, FB2, and MOBI are not converted on-device.

The normal OPDS download-folder setting controls the destination (SD root by
default). The filename-format setting supports author/title ordering. Downloaded
EPUBs are added to the library index and can be opened from the library or file
browser. Cancel/retry and network errors use the existing downloader UI.

To change the catalog URL, use Settings → OPDS → Flibusta → URL, or the web
settings OPDS Servers card. Username and password should remain empty for the
public catalog. The catalog's personal shelf requires a Flibusta login.

Flibusta may be unavailable from some networks. Live requests during development
included both successful XML feeds and intermittent HTTP 500/503 responses.
The firmware reports a fetch failure and offers Retry; it does not bypass a
network block or turn off TLS certificate verification.

## Reading statistics

Tracking is enabled by default; an existing saved opt-out is respected. Turn on
Track Reading Statistics in reader settings if needed. Open Reading Stats from
Home or the reader's stats action:

- This book: progress, reading time, pages turned, sessions, average session,
  reading speed, start/finish dates, and completion state.
- This device: totals across books, sessions, reading time, pages, books finished,
  and reading habits.
- Reading Rhythm: daily activity intensity, weekly minutes, monthly reading days,
  and current/best streaks.
- Finished Books: completed books grouped by month, with dates and reading time.
- Completion celebration: appears when finishing a book through the end-of-book
  flow. A jump near the end requests confirmation rather than silently counting it.
- Chapter/book time-left estimates: use the inherited WPM tracker and status-bar
  configuration. Speed estimates need real reading samples to settle.

Stats are collected prospectively; past reading time cannot be recovered from
ordinary CrossPoint progress alone. Stats tracking covers EPUB reading (including
TXT converted through the EPUB flow), not every image/XTC viewer.

Memory-bounded history has limits: precise daily minutes cover the latest 91 days,
730 days of activity-day history drive the longer calendar display, and the finished-book
index holds 32 entries. This is the inherited complete XPoint stats feature set,
not unlimited storage or every achievement system from every CrossPoint fork.

Set the reader's clock/timezone correctly for calendar stats. The stats live on
SD under `.crosspoint` (`global_stats.bin`, per-book versioned stats files, and the
finished-book index). Keep that directory in backups. Clearing all reading caches
or moving books can affect per-book state; do not delete it to install this build.

## Build and host checks

Initialize all submodules and use the PlatformIO Core version pinned by CI
(pioarduino 6.2.0 or compatible). Build with:

```sh
pio run -e x4pro-gh_release -j 2
cmake -S test -B build/tests
cmake --build build/tests --target FlibustaTest ReadingStatsStoreTest OpdsFilenameTest -j 2
ctest --test-dir build/tests -R 'Flibusta|ReadingStats|BookStats|GlobalStats|FinishedBooks|OpdsFilename' --output-on-failure
```

Flibusta tests exercise the actual server store and streaming XML parser, including
migration, credential preservation, deletion, a full store, failed SD writes,
Cyrillic text split across transport chunks, pagination, and EPUB selection.
`FLIBUSTA_TEST_FEED=/path/to/fetched-books.xml` enables the optional live-feed test.
Inherited stats tests cover binary persistence/recovery and accounting edge cases.

## Device acceptance checks

After installing, fetch a known book over Wi-Fi and open it. Read several pages
at normal speed, check This Book and This Device, then sleep/restart and confirm
the totals persist. Check the calendar after the date changes, mark a book as
finished, and verify the finished-books list. Test your usual orientation and the
other three orientations. A debug build can report heap with serial monitoring;
check that repeated catalog/reader transitions do not leak memory.

Online firmware checks intentionally return no update. Install future builds of
this branch through the same SD menu. The upstream signed-OTA implementation
remains in the source but is inactive for this custom distribution.

## Validation performed for this branch

Live checks on 2026-10-02 verified the anonymous HTTPS root catalog, author
index, Cyrillic search, and EPUB acquisition. A 4,581,437-byte EPUB of Pushkin's
*Eugene Onegin* was fetched without credentials; ZIP CRC validation passed,
`mimetype` was `application/epub+zip`, and the EPUB container was present.
Some other requests returned HTTP 500/503, so this does not establish continuous
service availability or validate the device's Wi-Fi/TLS path.
