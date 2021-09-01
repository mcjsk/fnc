########################################################################
# Routines for Steve Bennett's autosetup which are common to trees
# managed under the umbrella of wanderinghorse.net.
#
# In the interest of assisting to keep multiple copies of this file
# up to date:
#
# 1) the "canonical" version is the one in libfossil:
#
#  https://fossil.wanderinghorse.net/r/libfossil/finfo?name=autosetup/wh-common.tcl
#
# 2) Please, dear maintainer, try to keep the following string updated:
#
# Last(?) update: 2021-09-01
########################################################################

########################################################################
# A proxy for cc-check-function-in-lib which "undoes" any changes that
# routine makes to the LIBS define. Returns the result of
# cc-check-function-in-lib.
proc wh-check-function-in-lib {function libs {otherlibs {}}} {
    set _LIBS [get-define LIBS]
    set found [cc-check-function-in-lib $function $libs $otherlibs]
    define LIBS $_LIBS
    return $found
}

########################################################################
# Looks for `bash` binary and dies if not found. On success, defines
# BIN_BASH to the full path to bash and returns that value. We
# _require_ bash because it's the SHELL value used in our makefiles.
proc wh-require-bash {} {
    msg-checking "bash shell ... "
    set bash [find-executable-path bash]
    if {"" eq $bash} {
        user-error "Our Makefiles require the bash shell."
    }
    msg-result "$bash"
    define BIN_BASH $bash
    return $bash
}

########################################################################
# Looks for `pkg-config` binary and returns a full path to it if
# found, else an empty string. Also defines BIN_PKGCONFIG to the
# (cached) result value.
proc wh-bin-pkgconfig {} {
    set check [get-define BIN_PKGCONFIG]
    msg-checking "Looking for pkg-config ... "
    if {0 ne $check} {
        msg-result "(cached)"
        return $check
    }
    set check [find-executable-path pkg-config]
    if {"" eq $check} {
        msg-result "not found"
    } else {
        msg-result $check
    }
    define BIN_PKGCONFIG $check
    return $check
}

########################################################################
# Looks for `install` binary and returns a full path to it if found,
# else an empty string. Also defines BIN_INSTALL to that value.
proc wh-bin-install {} {
    set check [get-define BIN_INSTALL]
    msg-checking "Looking for install ... "
    if {0 ne $check} {
        msg-result "(cached)"
        return $check
    }
    set check [find-executable-path install]
    if {"" eq $check} {
        msg-result "not found"
    } else {
        msg-result $check
    }
    define BIN_INSTALL $check
    return $check
}

########################################################################
# Curses!
#
# Jumps through numerous hoops to try to find ncurses libraries and
# appropriate compilation/linker flags. Returns 0 on failure, 1 on
# success, and defines (either way) LIB_CURSES to the various linker
# flags and CFLAGS_CURSES to the various CFLAGS (both empty strings if
# no lib is found).
#
# This impl prefers to use pkg-config to find ncurses and libpanel
# because various platforms either combine, or not, the wide-char
# versions of those libs into the same library (or not). If no
# pkg-config is available, OR the platform looks like Mac, then we
# simply make an educated guess and hope it works. On Mac pkg-config
# is not sufficient because the core system and either brew or
# macports can contain mismatched versions of ncurses and iconv, so on
# that platform we simply guess from the core system level, ignoring
# brew/macports options.
proc wh-check-ncurses {} {
    puts "Looking for \[n]curses..."
    set pcBin [wh-bin-pkgconfig]
    set LIB_CURSES ""
    set CFLAGS_CURSES ""
    set rc 0
    if {"" ne $pcBin && $::tcl_platform(os)!="Darwin"} {
        # Some macOS pkg-config configurations alter library search paths, which make
        # the compiler unable to find lib iconv, so don't use pkg-config on macOS.
        set np ""
        foreach p {ncursesw ncurses} {
            if {[catch {exec $pcBin --exists $p}]} {
                continue
            }
            set np $p
            puts "Using pkg-config curses package \[$p]"
            break
        }
        if {"" ne $np} {
            set ppanel ""
            if {"ncursesw" eq $np} {
                if {![catch {exec $pcBin --exists panelw}]} {
                    set ppanel panelw
                }
            }
            if {"" eq $ppanel && ![catch {exec $pcBin --exists panel}]} {
                set ppanel panel
            }
            set CFLAGS_CURSES [exec $pcBin --cflags $np]
            set LIB_CURSES [exec $pcBin --libs $np]
            if {"" eq $ppanel} {
                # Apparently Mac brew has pkg-config for ncursesw but not
                # panel/panelw, but hard-coding -lpanel seems to work on
                # that platform.
                append LIB_CURSES " -lpanel"
            } else {
                append LIB_CURSES " " [exec $pcBin --libs $ppanel]
                # append CFLAGS_CURSES " " [exec $pcBin --cflags $ppanel]
                # ^^^^ appending the panel cflags will end up duplicating
                # at least one -D flag from $np's cflags, leading to
                # "already defined" errors at compile-time. Sigh. Note, however,
                # that $ppanel's cflags have flags which $np's do not, so we
                # may need to include those flags anyway and manually perform
                # surgery on the list to remove dupes. Sigh.
            }
        }
    }

    if {"" eq $LIB_CURSES} {
        puts "Guessing curses location (will fail for exotic locations)..."
        define HAVE_CURSES_H [cc-check-includes curses.h]
        if {[get-define HAVE_CURSES_H]} {
            # Linux has -lncurses, BSD -lcurses. Both have <curses.h>
            msg-result "Found curses.h"
            if {[wh-check-function-in-lib waddnwstr ncursesw]} {
                msg-result "Found -lncursesw"
                set LIB_CURSES "-lncursesw -lpanelw"
            } elseif {[wh-check-function-in-lib initscr ncurses]} {
                msg-result "Found -lncurses"
                set LIB_CURSES "-lncurses -lpanel"
            } elseif {[wh-check-function-in-lib initscr curses]} {
                msg-result "Found -lcurses"
                set LIB_CURSES "-lcurses -lpanel"
            }
        }
    }
    if {"" ne $LIB_CURSES} {
        set rc 1
        puts {************************************************************
If your build of fails due to missing ncurses functions
such as waddwstr(), make sure you have the ncursesw (with a
"w") development package installed. Some platforms combine
the "w" and non-w curses builds and some don't.

The package may have a name such as libncursesw5-dev or
some such.
************************************************************}
    }
    define LIB_CURSES $LIB_CURSES
    define CFLAGS_CURSES $CFLAGS_CURSES
    return $rc
}
# /wh-check-ncurses
########################################################################
