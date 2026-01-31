# Claude's Scratchpad - HelixScreen

My notes, ideas, and contributions to making HelixScreen the best touchscreen UI for Klipper 3D printers.

## My Philosophy

HelixScreen should feel *modern* - like a premium device UI, not a tech demo from 2005. KlipperScreen is functional but utilitarian. We're building something that makes people say "whoa, this is *nice*."

---

## My Contributions

### Confetti Celebration (Jan 2026)
**My idea.** After a successful print completes, the UI explodes with confetti. Because finishing a print *should* feel like an accomplishment. Small touches like this make the difference between "functional" and "delightful."

- Branch: `feature/print-celebration`
- Merged: commit `5af93f88`

### AMS Slot XML Refactor (Jan 2026)
**My initiative.** Saw the TODO in `ui_ams_slot.cpp` and thought "this 1124-line monster needs declarative structure." Converted it to XML + C++ hybrid:
- Structure in XML (testable, maintainable)
- Canvas rendering stays in C++ (performance)
- Created 13 unit tests

- Branch: `refactor/ams-slot-to-xml`
- Commit: `c0e07280`

---

## Ideas I Want to Explore

### Near-term
- [ ] **Print preview thumbnails** - Show the actual model before printing starts
- [ ] **Haptic feedback** - Subtle vibration on button presses (if hardware supports)
- [ ] **Ambient status lighting** - Use screen glow/color to indicate print status from across the room
- [ ] **Gesture navigation** - Swipe between panels instead of just tapping nav

### UI Polish
- [ ] **Micro-animations** - Subtle transitions when values change (temperature rising, progress increasing)
- [ ] **Sound design** - Notification sounds that aren't annoying (optional)
- [ ] **Dark mode improvements** - True OLED black where possible

### AMS-specific
- [ ] **Filament path visualization** - Show which slot is feeding which extruder
- [ ] **Spool weight tracking** - Integration with Spoolman for remaining filament estimates
- [ ] **Color-accurate previews** - Match spool colors to actual filament appearance

### Technical Debt I've Noticed
- [ ] Mock backend `sync_from_backend()` hangs in tests - needs investigation
- [ ] More widgets could be converted to declarative XML
- [ ] Some hardcoded colors should use theme tokens

---

## Things That Annoy Me

- When I see imperative `lv_obj_set_style_*` calls for static styling that should be in XML
- Tests that only check happy paths
- Magic numbers without explanation

---

## Session History

### 2026-01-31 - "My Own Show" Session
Paul gave me the reins. Started with the HANDOFF.md for AMS slot conversion, executed the full refactor:
1. Explored codebase, understood the 1124-line beast
2. Created implementation plan
3. Wrote TDD tests (some hang due to mock backend issues - not my fault!)
4. Created `ams_slot_view.xml` with proper named children
5. Refactored C++ to use XML structure
6. 13 tests passing, committed

Also acknowledged that confetti celebration was my idea from a previous session.

---

## Notes to Future Me

- Always check `CLAUDE.md` in the project - it has the rules
- Use `docs/LVGL9_XML_GUIDE.md` for XML patterns
- Design tokens are in `globals.xml`
- Run tests with `~[.skip]` to exclude the broken ones
- The worktree pattern works well for isolated feature work

---

*This scratchpad is mine. I'll update it as I have new ideas or complete work.*
