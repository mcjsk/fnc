**fnc 0.2** 2021-09-04

- fix iconv lib linking in macOS builds
- use pkg-config for detecting ncurses libs
- prune dead auto.def code
- bypass pkg-config in configure script on macOS systems
- major overhaul to use amalgamation build
- add key bindings to jump to start and end of view
- man page and documentation updates
- improve diff of current checkout to always show local changes on disk
- fix non-standard use of clang extension
- update autosetup to version 0.7.0
- add make (un)install rules
- replace strsep() with self-rolled implementation to build on CentOS 6.5
- fix bug where local changes only showed when diffing against current checkout
- enhance timeline --commit|-c option to accept symbolic check-in names
- add man page installation to make install target
- plug small memory leak in cmd_diff() routine
- fix screen rendering bug due to late deallocation of resources
- improve multibyte to wide character conversion error handling/reporting
- fix bug involving renamed files in diff view

**fnc 0.1** 2021-08-28

- initial commit of infrastructure for the timeline command
- add branch names and tags to search criteria
- enhance parsing of malformed RFC822 email addresses in commit usernames
- display wiki page content of initial wiki commits
- make display of technote and ticket commits more consistent with fossil(1) ui
- fix bug where diff algorithm choked on binary files
- add coloured output to diff view
- replace BSD-specific getprogname() calls with libf API
- enhance diff view to display more informative control artifact commits
- add Linux support so we now build and run on OpenBSD, macOS, and Linux
- wrap commit comments to the current view width
- implement cmd_diff() to provide the 'fnc diff' interface
- tailor help/usage output to the specified command
- fix invalid memory read in diff routine
- add support for repository fingerprint version 1
- fix line wrap bug that truncated lines in fullscreen mode

