## 1. Implement

- [x] 1.1 Add a helper in `Plugin.cpp` that sets `stale` on every entry in `g_bufferStates`,
      returning how many transitioned from not-stale to stale
- [x] 1.2 Call it from the `SCN_MODIFIED` handler instead of `CurrentBuffer().stale = true`
- [x] 1.3 Do not create entries for buffers that have none — a buffer with no cache has nothing
      to invalidate
- [x] 1.4 Keep the breadcrumb on the not-stale-to-stale transition only, and extend it to report
      the number of buffers marked
- [x] 1.5 Confirm `InvalidateIfStale` is unchanged: it still clears only the buffer passed to it,
      and still leaves `matches` / `highlightActive` alone

## 2. Verify

- [x] 2.1 Release|x64 and Debug|x64 build clean
- [ ] 2.2 With debug mode on, typing in one document logs one line naming that document and a
      count matching the number of previously-used buffers
- [ ] 2.3 Continued typing produces no further lines until something rescans
- [ ] 2.4 Open two files, run a report on each, edit one, then run the report on the *other* —
      its breadcrumb reports `stale=true -> caches invalidated`
- [ ] 2.5 Repeat 2.4 with Replace All in All Opened Documents; the untouched-looking buffer also
      reports `stale=true`
- [ ] 2.6 Ctrl+Alt+W still cycles instantly on repeated presses when nothing has been edited

**Build status:** Release|x64 and Debug|x64 both build clean; no new warnings beyond the two
pre-existing `C4312`s in `ProgressDialog.cpp`.

**Confirmed by inspection (task 1.5):** `InvalidateIfStale` is byte-for-byte unchanged — it still
clears only the buffer passed to it and still leaves `matches` / `highlightActive` alone, so the
Overview Panel keeps restoring marks on tab switch. The only remaining write to `stale = true` in
the whole file is inside `MarkAllBuffersStale`.

Section 2 is runtime behaviour inside Notepad++.
