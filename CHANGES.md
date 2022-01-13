**fnc 0.8** 2022-01-10

- fix vertical split view init regression from 0.7 when terminal < 120 cols wide 
- fix DB lock when opening horizontal split that signals the timeline thread

**fnc 0.7** 2022-01-09

- factor out common make(1) and gmake build bits
- make build depend on make file
- make all commands compatible with `-R|--repository` (i.e., no checkout needed)
- implement Vim-like smartcase for commands that filter repository results
- improve timeline arg parsing for more informative output upon invalid input
- plug small memory leak when the timeline command fails to initialise a view
- fix database locking issue when opening multiple tree views from the timeline
- add pledge(2) and unveil(2) support on OpenBSD builds
- add (s)ort keymap (i.e., `--sort` CLI option counterpart) to the branch view
- add (d)ate keymap to toggle display of last modified timestamp in tree view
- allow blame of empty files rather than erroring out
- fix NULL dereference UB when the ".." parent dir is selected in tree view
- merge upstream fix when annotating files with shallow history
- add support for configurable horizontal split view mode (timeline -> diff)
- keep the last/first line in view when paging down/up to stay oriented
- expand horizontal splitscreen support to include tree -> blame views
- improve handling of cached commit builder statements
- plug small memleak of file labels when traversing commits from the diff view
- display message rather than error out when binary files are blamed
- expand horizontal split coverage to include timeline -> tree views
- implement '#' keymap to display line numbers in blame view
- implement (L)ine keymap to navigate directly to the entered line in blame view
- convert diff code to use upstream v2 API
- keep lines on screen in view when using keymaps that modify diff content
- reset diff search state when traversing commits from the diff view
- apply upstream diff fix to render consecutive edited lines as one block
- implement '#' keymap to display line numbers in diff view
- improve `C-f/C-b` paging in the in-app help window
- add user-defined option to configure search highlight colours
- implement navigate to (L)ine keymap in diff view
- enhance timeline and diff view headers
- implement user-defined option to configure line selection cursor in diff view
- implement Vim-like `C-e/C-y` keymaps to scroll down/up one line in diff view
- implement horizontal scrolling in diff and blame views
- improve view request handling and optimise new view initialisation
- implement `C-n/C-p` keymap to navigate to the next/previous file in the diff
- implement (F)ile keymap to navigate directly to a file in the diff
- apply upstream diff v2 fix for rendering untouched lines as changed
- implement Vim-like `C-d/C-u` keymaps to scroll the view down/up half a page
- enhance search behaviour in all views

**fnc 0.6** 2021-11-22

- implement support for user-defined colours in tl, diff, tree and blame views
- make branch view accessible from all other views
- optimise colour scheme initialisation
- implement new 'fnc config' interface to configure per-project settings
- improve build by making make file make(1) compatible
- rename old make file to GNUmakefile to keep gmake support
- more robust error handling of invalid user input to new config command
- fix tree symlink colour regular expression so symlink entries are colourised
- improve colourised header highlighting when in splitscreen mode
- fix display of colourised headers in tmux
- more robust handling of cached statements when switching views
- fix branch-to-tree-to-timeline segv bug when file is missing
- improve const correctness of fcli struct declarations
- enhance diff command with support for arbitrary repository blob artifacts
- quiet error reporting in non-debug builds
- fix commit builder bug that may omit timeline commits when switching views
- improved branch view resource deallocation
- implement new timeline command --filter option and key binding
- enhance timeline --type option with support for tag control artifacts
- general documentation improvements
- assorted updates from upstream libfossil including compiler warning fix
- expand support for user-defined colours to include the branch view

**fnc 0.5** 2021-11-03

- simplify the build system to make-only and remove autosetup (patch from sdk)
- remove invalid fsl_errno_to_rc() call and redundant alloc error check
- reduce noise on app exit by removing redundant error output
- relocate fnc_strlcpy() and fnc_strlcat() routines to the library

**fnc 0.4** 2021-10-31

- resolve database bug in the commit builder logic
- prune dead code leftover from the initial single-threaded blame implementation
- improve error handling of commands that return no records
- update in-app help with blame key bindings
- expand tree command's ability to display trees of versions with missing files
- improve tree command error handling of non-checkin artifacts
- make tree command compatible with the `-R <repository>` option
- add profiling (`--profile`) option to the build system
- notify user of invalid blame _P_ key binding selections with ephemeral message
- add coloured output and associated key binding/options to the timeline view
- ensure the full diff is displayed for non-standard initial check-ins
- initialise variables to squelch gcc-9.3 compiler warnings
- add command line aliases to all currently available commands
- add time-based annotation option to the blame command
- fix colour conflicts between certain views
- enhance diff interface with optional path arg to filter diffs between commits
- improve diff view by displaying full context when diffing consecutive commits
- make diff index parsing more robust when handling removed/missing files
- ensure diff honours the repository's `allow-symlinks` setting
- simplify help output when `-h|--help` is passed to command aliases
- invert diff metadata when `-i|--invert` or the _i_ key binding is used
- fix diff bug when the first commit is opened from the timeline view
- enhance diff command to enable diff of arbitrary file blobs
- optimise parsing of artifacts to determine which diff routine to call
- implement `fnc branch` which displays a navigable list of repository branches
- expand available sort options to the branch command
- simplify usage output on error or `-h|--help` with usage callback
- general man page and documentation improvements
- fix incorrect configure `--prefix` install path in the docs (reported by sdk)

**fnc 0.3** 2021-10-17

- add in-app help with H|?|F1 key binding
- improvements to the build system
- decompose build_commits() to make reusable commit_builder() method
- dynamically size header so metadata is not truncated in timeline view
- fix highlight of coloured search results in tmux with A_REVERSE attribute
- correct install path of man pages with 'make install'
- implement 'fnc tree' command to navigate repository tree
- deduplicate code with utility functions fnc_home() and strtonumcheck()
- implement 'fnc path' convenience form of 'fnc timeline path'
- improve error reporting and cleaner code with rc wrapper
- make -R repo option available to 'fnc timeline' 
- make -R repo option compatible with 'fnc timeline path' invocation
- implement 'fnc blame' command with the new libfossil annotate API
- enable accessing the blame interface from the tree view
- fix handling of artifacts with no comment string
- substantial UX and performance improvements by making 'fnc blame' threaded
- fix bug in 'fnc blame --limit|-n' calls when lines were not annotated
- enable handling of master branches not named 'trunk' in 'fnc blame'
- enable accessing the tree interface from the timeline with new 't' key binding
- add '-C|--no-colour' and 'c' key binding to 'fnc blame'
- enhance and standardise parsing of path arguments for all commands

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

