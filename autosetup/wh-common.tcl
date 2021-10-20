########################################################################
# Routines for Steve Bennett's autosetup which are common to trees
# managed under the umbrella of wanderinghorse.net.
#
# In the interest of assisting to keep multiple copies of this file
# up to date:s
#
# The "canonical" version is the one in libfossil:
#
#  https://fossil.wanderinghorse.net/r/libfossil/finfo?name=autosetup/wh-common.tcl
########################################################################

array set whcache {} ; # used for caching various results.

########################################################################
# wh-lshift shifts $count elements from the list named $listVar and
# returns them.
#
# Modified slightly from: https://wiki.tcl-lang.org/page/lshift
#
# On an empty list, returns "".
proc wh-lshift {listVar {count 1}} {
    upvar 1 $listVar l
    if {![info exists l]} {
        # make the error message show the real variable name
        error "can't read \"$listVar\": no such variable"
    }
    if {![llength $l]} {
        # error Empty
        return ""
    }
    set r [lrange $l 0 [incr count -1]]
    set l [lreplace $l [set l 0] $count]
    return $r
}

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
# This caches the result for a given $binName/$defName combination, so
# calls after the first for a given combination will always return the
# same result.
#
# If defName is empty then "BIN_X" is used, where X is the upper-case
# form of $binName with any '-' characters removed.
proc wh-bin-define {binName {defName {}}} {
    global whcache
    set cacheName "$binName:$defName"
    set check {}
    if {[info exists whcache($cacheName)]} {
        set check $whcache($cacheName)
    }
    msg-checking "Looking for $binName ... "
    if {"" ne $check} {
        set lbl $check
        if {" _ 0 _ " eq $check} {
            set lbl "not found"
            set check ""
        }
        msg-result "(cached) $lbl"
        return $check
    }
    set check [find-executable-path $binName]
    if {"" eq $check} {
        msg-result "not found"
        set whcache($cacheName) " _ 0 _ "
    } else {
        msg-result $check
        set whcache($cacheName) $check
    }
    if {"" eq $defName} {
        set defName "BIN_[string toupper [string map {- {}} $binName]]"
    }
    define $defName $check
    return $check
}

########################################################################
# Looks for `bash` binary and dies if not found. On success, defines
# BIN_BASH to the full path to bash and returns that value. We
# _require_ bash because it's the SHELL value used in our makefiles.
proc wh-require-bash {} {
    set bash [wh-bin-define bash]
    if {"" eq $bash} {
        user-error "Our Makefiles require the bash shell."
    }
    return $bash
}

########################################################################
# Internal impl for wh-opt-bool-01 and wh-opt-bool-01-invert.
#
# args = {optName defName invert {descr {}}}
proc wh-opt-bool-01-impl {args} {
}


########################################################################
# Args: [-v] optName defName {descr {}}
#
# Checks [opt-bool $optName] and does [define $defName X] where X is 0
# for false and 1 for true. descr is an optional [msg-checking]
# argument which defaults to $defName. Returns X.
#
# If args[0] is -v then the boolean semantics are inverted: if
# the option is set, it gets define'd to 0, else 1. Returns the
# define'd value.
proc wh-opt-bool-01 {args} {
    set invert 0
    if {[lindex $args 0] eq "-v"} {
		set invert 1
		set args [lrange $args 1 end]
	}
    set optName [wh-lshift args]
    set defName [wh-lshift args]
    set descr [wh-lshift args]
    if {"" eq $descr} {
        set descr $defName
    }
    set rc 0
    msg-checking "$descr ... "
    if {[opt-bool $optName]} {
        if {0 eq $invert} {
            set rc 1
        } else {
            set rc 0
        }
    } elseif {0 ne $invert} {
        set rc 1
    }
    msg-result $rc
    define $defName $rc
    return $rc
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
    set pcBin [wh-bin-define pkg-config]
    msg-checking "Looking for \[n]curses... "
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
            msg-result "Using pkg-config curses package \[$p]"
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


########################################################################
# Opens the given file, reads all of its content, and returns it.
proc wh-file-content {fname} {
    set fp [open $fname r]
    set rc [read $fp]
    close $fp
    return $rc
}

########################################################################
# Returns the contents of the given file as an array, with the EOL
# stripped from each input line.
proc wh-file-content-list {fname} {
    set fp [open $fname r]
    set rc {}
    while { [gets $fp line] >= 0 } {
        lappend rc $line
    }
    return $rc
}

########################################################################
# Checks the compiler for compile_commands.json support. If passed an
# argument it is assumed to be the name of an autosetup boolean config
# option to explicitly DISABLE the compile_commands.json support.
#
# Returns 1 if supported, else 0. Defines MAKE_COMPILATION_DB to "yes"
# if supported, "no" if not.
proc wh-check-compile-commands {{configOpt {}}} {
    msg-checking "compile_commands.json support... "
    if {"" ne $configOpt && [opt-bool $configOpt]} {
        msg-result "explicitly disabled"
        define MAKE_COMPILATION_DB no
        return 0
    } else {
        if {[cctest -lang c -cflags {/dev/null -MJ} -source {}]} {
            # This test reportedly incorrectly succeeds on one of
            # Martin G.'s older systems.
            msg-result "compiler supports compile_commands.json"
            define MAKE_COMPILATION_DB yes
            return 1
        } else {
            msg-result "compiler does not support compile_commands.json"
            define MAKE_COMPILATION_DB no
            return 0
        }
    }
}

########################################################################
# Uses [make-template] to creates makefile(-like) file $filename from
# $filename.in but explicitly makes the output read-only, to avoid
# inadvertent editing (who, me?).
#
# The second argument is an optional boolean specifying whether to
# `touch` the generates files. This can be used as a workaround for
# cases where (A) autosetup does not update the file because it was
# not really modified and (B) the file *really* needs to be updates to
# the please build process. Pass any non-0 value to enable touching.
#
# The argument may be a list of filenames.
proc wh-make-from-dot-in {filename {touch 0}} {
  foreach f $filename {
    catch { exec chmod u+w $f }
    make-template $f.in $f
    if {0 != $touch} {
      puts "Touching $f"
      catch { exec touch $f }
    }
    catch { exec chmod u-w $f }
  }
}

########################################################################
# Checks for the boolean configure option named by $flagname. If set,
# it checks if $CC seems to refer to gcc. If it does (or appears to)
# then it defines CC_PROFILE_FLAG to "-pg" and returns 1, else it
# defines CC_PROFILE_FLAG to "" and returns 0.
#
# Note that the resulting flag must be added to both CFLAGS and
# LDFLAGS in order for binaries to be able to generate "gmon.out".  In
# order to avoid potential problems with escaping, space-containing
# tokens, and interfering with autosetup's use of these vars, this
# routine does not directly modify CFLAGS or LDFLAGS.
proc wh-check-profile-flag {{flagname profile}} {
  if {[opt-bool $flagname]} {
    set CC [get-define CC]
    regsub {.*ccache *} $CC "" CC
    # ^^^ if CC="ccache gcc" then [exec] treats "ccache gcc" as a
    # single binary name and fails. So strip any leading ccache part
    # for this purpose.
    if { ![catch { exec $CC --version } msg]} {
      if {[string first gcc $CC] != -1} {
        define CC_PROFILE_FLAG "-pg"
        return 1
      }
    }
  }
  define CC_PROFILE_FLAG ""
  return 0
}
