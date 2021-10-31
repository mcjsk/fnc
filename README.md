# README

# fnc 0.4

## A read-only ncurses browser for [Fossil][0] repositories in the terminal.

`fnc` uses [libfossil][1] to create a [`fossil ui`][2] experience in the
terminal.

Tested and confirmed to run on the following x64 systems (additional platforms
noted inline):

1. OpenBSD 6.8- and 6.9-release
2. macOS 10.15.7 (Catalina) and 11.5.2 (Big Sur)
3. Linux Mint 20.2 (32- and 64-bit ARM)
4. Ubuntu 18.04 running Linux kernel 5.11 (32-bit ARM)
5. Debian GNU/Linux 8, 9, and 10
6. CentOS 6.5 (32-bit)

Alpha development notwithstanding, the `timeline`, `diff`, `tree`, `blame`, and
`branch` commands are relatively stable; however, there is no commitment to
refrain from breaking changes.

# Build

1. clone the repository
  - `fossil clone https://fnc.bsdbox.org`
2. move into the repository checkout
  - `cd fnc`
3. run the configure script
  - `./configure`
4. build fnc
  - `make`
5. install the `fnc` binary (*requires privileges*)
  - `doas make install`
6. move into an open Fossil checkout, and run it:
  - `cd ~/museum/repo && fossil open ../repo.fossil && fnc`

This will install the `fnc` executable and man page into `/usr/local/bin` and
`/usr/local/share/man/man1`, respectively. To change the install path, use the
`--prefix` option to `configure`; for example, replacing step 3 with
`./configure --prefix=$HOME` will install the executable and man page into
`~/bin` and `~/share/man`, respectively. Alternatively, cryptographically signed
binaries for some of the abovementioned platforms are available to [download][3].

# Doc

See `fnc --help` for a quick reference, and the [fnc(1)][4] manual page for more
comprehensive documentation. In-app help can also be accessed with the `?`,
`F1`, or `H` key binding. The following video briefly demonstrates some of the
key bindings in use.

[![fnc demo][5]][6]

# Why

`fnc` is heavily inspired by [`tog`][7], which I missed when I left [Got][8]
(Git) behind and started using Fossil. The objective is to provide an
alternative to `fossil ui` without leaving the terminal.

# Problems & Patches

Please submit bug reports via [email][9], the [forum][10], or by creating a new
[ticket][11]. As a rule, all reports should include a bug reproduction recipe;
that is, either (1) the series of steps beginning with `fossil init` to create a
new repository through to the `fnc` command that triggers the unexpected
behaviour; or, if possible, (2) a shell script that contains all necessary
ingredients to reproduce the problem.

Patches are thoughtfully considered and can be sent to the [mailing list][12].
While `diff -up` patches are preferred, `fossil patch create` and `fossil diff` 
patches are also welcomed. Please ensure code conforms to the C99 standard,
and complies with OpenBSD's KNF [style(9)][13]. Any patch containing
user-visible code addition, modification, or deletion (i.e., code that impacts
user interfaces) should concomitantly include updating documentation affected
by the change.

# Screenshots

![diff split screen](https://fnc.bsdbox.org/uv/resources/img/fnc-diff-splitscreen.png "fnc diff split screen")
![diff renamed file](https://fnc.bsdbox.org/uv/resources/img/fnc-diff-full-file_renamed.png "fnc diff file renamed")
![diff added file](https://fnc.bsdbox.org/uv/resources/img/fnc-diff-split-file_added.png "fnc diff file added")
![diff removed file](https://fnc.bsdbox.org/uv/resources/img/fnc-diff-split-file-removed.png "fnc diff file removed")
![blame split screen](https://fnc.bsdbox.org/uv/resources/img/fnc-blame-splitscreen.png "fnc blame split screen")
![tree split screen](https://fnc.bsdbox.org/uv/resources/img/fnc-tree-splitscreen.png "fnc tree split screen")
![branch split screen](https://fnc.bsdbox.org/uv/resources/img/fnc-branch-splitscreen.png "fnc branch split screen")
![in-app help](https://fnc.bsdbox.org/uv/resources/img/fnc-inapp_help.png "fnc in-app help")
![timeline help](https://fnc.bsdbox.org/uv/resources/img/fnc-timeline-help.png "fnc timeline help")

[0]: https://fossil-scm.org
[1]: https://fossil.wanderinghorse.net/r/libfossil
[2]: https://fossil-scm.org/home/help?cmd=ui
[3]: https://fnc.bsdbox.org/uv/download.html
[4]: https://fnc.bsdbox.org/uv/resources/doc/fnc.1.html
[5]: https://fnc.bsdbox.org/uv/resources/img/fnc-timeline-fullscreen.png
[6]: https://itac.bsdbox.org/fnc-demo.mp4
[7]: https://gameoftrees.org/tog.1.html
[8]: https://gameoftrees.org
[9]: mailto:fnc@bsdbox.org
[10]: https://fnc.bsdbox.org/forum
[11]: https://fnc.bsdbox.org/ticket
[12]: https://itac.bsdbox.org/listinfo/fnc
[13]: https://man.openbsd.org/style.9

