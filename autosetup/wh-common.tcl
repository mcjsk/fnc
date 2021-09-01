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
# Look for binary named $binName and `define`s $defName to that full
# path, or an empty string if not found. Returns the value it defines.
# This caches the result for a given $binName value, but if called multiple
# times with a different defName, the 2nd and subsequent defName will not
# get `define`d (patches to fix that are welcomed).
proc wh-bin-define {binName defName} {
    set check [get-define $defName 0]
    msg-checking "Looking for $binName ... "
    if {0 ne $check} {
        msg-result "(cached) $check"
        return $check
    }
    set check [find-executable-path $binName]
    if {"" eq $check} {
        msg-result "not found"
    } else {
        msg-result $check
    }
    define $defName $check
    return $check
}

########################################################################
# Looks for `bash` binary and dies if not found. On success, defines
# BIN_BASH to the full path to bash and returns that value. We
# _require_ bash because it's the SHELL value used in our makefiles.
proc wh-require-bash {} {
    set bash [wh-bin-define bash BIN_BASH]
    if {"" eq $bash} {
        user-error "Our Makefiles require the bash shell."
    }
    return $bash
}

########################################################################
# Looks for `pkg-config` binary and returns a full path to it if
# found, else an empty string. Also defines BIN_PKGCONFIG to the
# (cached) result value.
proc wh-bin-pkgconfig {} {
    return [wh-bin-define pkg-config BIN_PKGCONFIG]
}

########################################################################
# Looks for `install` binary and returns a full path to it if found,
# else an empty string. Also defines BIN_INSTALL to the (cached)
# result value.
proc wh-bin-install {} {
    return [wh-bin-define install BIN_INSTALL]
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
the "w" and non-w curses builds and some don't. Similarly,
it's easy to get a mismatch between libncursesw and libpanel,
so make sure you have libpanelw (if appropriate for your
platform).

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

########################################################################
# Check for module-loading APIs (libdl/libltdl)...
#
# Looks for libltdl or dlopen(), the latter either in -ldl or built in
# to libc (as it is on some platforms. Returns 1 if found, else
# 0. Either way, it `define`'s:
#
#  - HAVE_LIBLTDL to 1 or 0 if libltdl is found/not found
#  - HAVE_LIBDL to 1 or 0 if dlopen() is found/not found
#  - LDFLAGS_MODULE_LOADER one of ("-lltdl", "-ldl", or ""), noting that
#    the string may legally be empty on some platforms if HAVE_LIBDL is true.
proc wh-check-module-loader {} {
    msg-checking "Looking for module-loader APIs... "
    if {99 ne [get-define LDFLAGS_MODULE_LOADER]} {
        if {1 eq [get-define HAVE_LIBLTDL 0]} {
            msg-result "(cached) libltdl"
            return 1
        } elseif {1 eq [get-define HAVE_LIBDL 0]} {
            msg-result "(cached) libdl"
            return 1
        }
        # else: wha???
    }
    set HAVE_LIBLTDL 0
    set HAVE_LIBDL 0
    set LDFLAGS_MODULE_LOADER ""
    set rc 0
    puts "" ;# cosmetic kludge for cc-check-XXX
    if {[cc-check-includes ltdl.h] && [cc-check-function-in-lib lt_dlopen ltdl]} {
        set HAVE_LIBLTDL 1
        set LDFLAGS_MODULE_LOADER "-lltdl"
        puts " - Got libltdl."
        set rc 1
    } elseif {[cc-with {-includes dlfcn.h} {
        cctest -link 1 -declare "extern char* dlerror(void);" -code "dlerror();"}]} {
        puts " - This system can use dlopen() w/o -ldl."
        set HAVE_LIBDL 1
        set LDFLAGS_MODULE_LOADER ""
        set rc 1
    } elseif {[cc-check-includes dlfcn.h]} {
        set HAVE_LIBDL 1
        set rc 1
        if {[cc-check-function-in-lib dlopen dl]} {
            puts " - dlopen() needs libdl."
            set LDFLAGS_MODULE_LOADER "-ldl"
        } else {
            puts " - dlopen() not found in libdl. Assuming dlopen() is built-in."
            set LDFLAGS_MODULE_LOADER ""
        }
    }
    define HAVE_LIBLTDL $HAVE_LIBLTDL
    define HAVE_LIBDL $HAVE_LIBDL
    define LDFLAGS_MODULE_LOADER $LDFLAGS_MODULE_LOADER
    return $rc
}
