# README

# fnc 0.7

## An interactive ncurses browser for [Fossil][0] repositories.

`fnc` uses [libfossil][1] to create a [`fossil ui`][2] experience in the
terminal.

Tested and confirmed to run on the following amd64 systems (additional platforms
noted inline):

1. OpenBSD 6.8-, 6.9-, and 7.0-release
2. macOS 10.15.7 (Catalina) and 11.5.2 (Big Sur)
3. Linux Mint 20.2 (32- and 64-bit ARM)
4. Ubuntu 18.04 running Linux kernel 5.11 (32-bit ARM)
5. Debian GNU/Linux 8, 9, and 10
6. CentOS 6.5 (32-bit)

Alpha development notwithstanding, the `timeline`, `diff`, `tree`, `blame`, and
`branch` commands are relatively stable; however, there is no commitment to
refrain from breaking changes.

# Install

* **OpenBSD**
  - `doas pkg_add fnc`
* **macOS**
  - `sudo port install fnc`
* **[Download](/uv/download.html)** and install the binary on your path

# Build

1. clone the repository
  - `fossil clone https://fnc.bsdbox.org`
2. move into the repository checkout
  - `cd fnc`
3. build fnc
  - `make`
4. install the `fnc` binary (*requires privileges*)
  - `doas make install`
5. move into an open Fossil checkout, and run it:
  - `cd ~/museum/repo && fossil open ../repo.fossil && fnc`

This will install the `fnc` executable and man page into `/usr/local/bin` and
`/usr/local/share/man/man1`, respectively. Alternatively, cryptographically
signed tarballs of the source code and binaries for some of the abovementioned
platforms are available to [download][3].

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

![diff vsplit](https://fnc.bsdbox.org/uv/resources/img/fnc-diff-vsplit.png "diff vertical split")
![diff hsplit renamed](https://fnc.bsdbox.org/uv/resources/img/fnc-diff-hsplit-renamed.png "diff horizontal split file renamed")
![diff vsplit added](https://fnc.bsdbox.org/uv/resources/img/fnc-diff-vsplit-added.png "diff vertical split file added")
![diff vsplit removed](https://fnc.bsdbox.org/uv/resources/img/fnc-diff-vsplit-removed.png "diff vertical split file removed")
![blame vsplit](https://fnc.bsdbox.org/uv/resources/img/fnc-blame-vsplit.png "blame vertical split")
![tree vsplit](https://fnc.bsdbox.org/uv/resources/img/fnc-tree-vsplit.png "tree vertical split")
![branch hsplit](https://fnc.bsdbox.org/uv/resources/img/fnc-branch-hsplit.png "branch horizontal split")
![in-app help](https://fnc.bsdbox.org/uv/resources/img/fnc-inapp_help.png "fnc in-app help")
![timeline help](https://fnc.bsdbox.org/uv/resources/img/fnc-timeline-help.png "fnc timeline help")

# Trivia

**fnc** [fɪŋk]  
*noun* (n.)  
1. an interactive ncurses browser for [Fossil][0] repositories  
*verb* (v.)  
2. to inform  
etymology  
From the German word *Fink*, meaning "finch", a type of bird.

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
