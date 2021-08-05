/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */ 
/* vim: set ts=2 et sw=2 tw=80: */
/**
   s2sh2 - a cleanup of s2sh.

   s2sh2 is functionally *nearly* identical to s2sh, the primary
   difference being that s2sh2 has more conventional/cleaned-up CLI
   argument handling. Many of the flags which are toggles in s2sh have
   only a single form in s2sh2, to turn something on or off, but not
   toggle it.

   Other minor differences:

   - $2 is a UKWD referring to the global s2 object.
*/

#define S2SH_VERSION 2
#include "shell_common.c"

/**
   gcc bug: if this function is marked static here, it does
   not recognize that shell_common.c uses it and declares that
   it's unused.
*/
static CliAppSwitch const * s2sh_cliapp_switches(){
  static const CliAppSwitch cliAppSwitches[] = {
  /* {opaque, dash, key, value, brief, details, callback, reserved} */
  /** Fills out most of a CliAppSwitch entry */
#define F6(IDFLAGS,DASHES,KEY,VAL,BRIEF,PFLAGS)     \
  {IDFLAGS, DASHES, KEY, VAL, BRIEF, 0, 0, PFLAGS}
  /** Continues the help text for the previous entry. */
#define FCONTINUE(LABEL) {SW_CONTINUE, 0, 0, 0, LABEL, 0, 0, 0}
  /** Adds an "additional info" line, not indented. */
#define FINFOLINE(LABEL) {SW_INFOLINE, 0, 0, 0, LABEL, 0, 0, 0}
  /** -short | --long[=VALUE]*/
#define FX(IDFLAGS,SHORT,LONG,VALUE,BRIEF,PFLAGS)   \
  F6(IDFLAGS,2,LONG,VALUE,BRIEF,PFLAGS),            \
  F6(IDFLAGS,1,SHORT,VALUE,0,PFLAGS)
  /** -short, --long on/off switches ...*/
#define F01(IDFLAGS,SHORT,LONG,BRIEF)                       \
  {IDFLAGS+1, 2, LONG,   0, "Enable "  BRIEF, 0, 0, 0},     \
  {IDFLAGS+1, 1, SHORT,  0, "Enable "  BRIEF, 0, 0, 0},     \
  {IDFLAGS,   2, "no-"LONG, 0, "Disable " BRIEF, 0, 0, 0},  \
  {IDFLAGS,   1, "no"SHORT, 0, "Disable " BRIEF, 0, 0, 0}
  /** Start the next group (for --help grouping). */
#define GROUP(GID,DESC) {GID, 0, 0, 0, DESC, 0, 0, 0}

  /***********************************************************************/
  GROUP(SW_GROUP_0,0),

  /**
     Keep these arranged so that flags with short and long forms are
     ordered consecutively, long one first. The help system uses that to
     collapse the flags into a single listing. In such cases, the help
     text from the first entry is used and the second is ignored.
  */
  FX(SW_HELP, "?", "help", 0,
     "Send help text to stdout and exit with code 0.", 0),
  FCONTINUE("The -v flag can be used once or twice to get more info."),

  FX(SW_VERBOSE,"v","verbose",0,
     "Increases verbosity level by 1.", 0),

  FX(SW_VERSION,"V","version",0,
     "Show version info and exit with code 0. ", 0),
  FCONTINUE("Use with -v for more details."),

  FX(SW_ESCRIPT,"e","eval","SCRIPT",
     "Evaluates the given script code in the global scope. "
     "May be used multiple times.", CLIAPP_F_SPACE_VALUE),

  FX(SW_INFILE,
     "f","file","infile",
     "Evaluates the given file. The first non-flag argument is used "
     "by default.", CLIAPP_F_SPACE_VALUE | CLIAPP_F_ONCE),

  FX(SW_OUTFILE, "o", "output", "outfile",
     "Sets cwal's output channel to the given file. Default is stdout.",
     CLIAPP_F_SPACE_VALUE | CLIAPP_F_ONCE),

  /***********************************************************************/
  GROUP(SW_GROUP_1, "Less-common options:"),

  FX(SW_ASSERT_1,"A", "assert", 0, "Increase assertion trace level by one: "
     "1=trace assert, 2=also trace affirm.",0),

  FX(SW_INTERACTIVE,"i", "interactive", 0,
     "Force-enter interactive mode after running -e/-f scripts.",0),

  FX(SW_AUTOLOAD_0,"I","no-init-script", 0,
     "Disables auto-loading of the init script.",0),
  FCONTINUE("Init script name = same as this binary plus \".s2\" "),
  FCONTINUE("or set via the S2SH_INIT_SCRIPT environment variable."),
  
  /***********************************************************************/
  GROUP(SW_GROUP_2, "Esoteric options:"),

  FX(SW_METRICS, "m", "metrics", 0,
     "Show various cwal/s2 metrics before shutdown.",0),
  FCONTINUE("Use -v once or twice for more info."),

  F01(SW_MOD_INIT_0, "mi", "module-init",
      "automatic initialization of static modules."),

  F6(SW_SHELL_API_1,2,"s2.shell",0,
     "Forces installation of the s2.shell API unless trumped by --cleanroom.",0),
  FCONTINUE("By default s2.shell is only installed in interactive mode."),

  F6(SW_TRACE_CWAL,2,"trace-cwal", 0,
     "Enables tremendous amounts of cwal_engine "
     "tracing if it's compiled in.",0),
  
  F01(SW_SCOPE_H0, "hash", "scope-hashes",
      "hashes for scope property storage (else objects)."),

  FX(SW_HIST_FILE_1,
     "h", "history", "filename",
     "Sets the interactive-mode edit history filename.",
     CLIAPP_F_SPACE_VALUE | CLIAPP_F_ONCE),

  F6(SW_HIST_FILE_0,2, "no-history", 0,
     "Disables interactive-mode edit history.", 0),
  F6(SW_HIST_FILE_0,1, "noh", 0, 0, 0),

  F6(SW_DISABLE, 2, "s2-disable", "comma,list",
     "Disables certain s2 API features by name.",0),
  FCONTINUE("Use a comma-and/or-space-separated list with any of "
            "the following entries:"),
  FCONTINUE("fs-stat, fs-read, fs-write, fs-io, fs-all"),
  
  F6(SW_CLEANROOM,2,"cleanroom",0,
     "Only core language functionality with no prototype-level methods.",0),
  FCONTINUE("Disables the s2/$2 global object and auto-loading of the "
            "init script."),

  FINFOLINE("Recycling-related options:"),
  F01(SW_RE_V0,"rv", "recycle-values", "recycling of value instances."),
  F01(SW_RE_C0,"rc","recycle-chunks", "the memory chunk recycler."),
  F01(SW_RE_S0,"si","string-interning", "string interning."),

  FINFOLINE("Memory-tracking/capping options:"),
  FX(SW_MCAP_TOTAL_ALLOCS, "cap-ta", "memcap-total-allocs", "N",
     "* Limit the (T)otal number of (A)llocations to N.",
     CLIAPP_F_SPACE_VALUE),
  FX(SW_MCAP_TOTAL_ALLOCS, "cap-tb", "memcap-total-bytes", "N",
     "* Limit the (T)otal number of allocated (B)ytes to N.",
     CLIAPP_F_SPACE_VALUE),
  FX(SW_MCAP_TOTAL_ALLOCS, "cap-ca", "memcap-concurrent-allocs", "N",
     "Limit the (C)oncurrent number of (A)llocations to N.",
     CLIAPP_F_SPACE_VALUE),
  FX(SW_MCAP_TOTAL_ALLOCS, "cap-cb", "memcap-concurrent-bytes", "N",
     "Limit the (C)oncurrent number of allocated (B)ytes to N.",
     CLIAPP_F_SPACE_VALUE),
  FX(SW_MCAP_TOTAL_ALLOCS, "cap-sb", "memcap-single-alloc", "N",
     "Limit the size of any (S)ingle allocation to N (B)ytes.",
     CLIAPP_F_SPACE_VALUE),
  FINFOLINE("The allocator starts failing (returning NULL) when a capping constraint is violated."),
  FCONTINUE("The 'tb' and 'ta' constraint violations are permanent (recovery requires resetting the engine)."),
  FCONTINUE("The 'ca' and 'cb' constraints are recoverable once some cwal-managed memory gets freed."),
  FCONTINUE("Capping only applies to memory allocated by/via the scripting\n"
            "engine, not 3rd-party APIs."),

  F6(SW_MEM_FAST1,2,"fast",0,"(F)orce (A)lloc (S)ize (T)racking.",0),
  FCONTINUE("Enables more precise tracking and reuse of recycled memory."),
  FCONTINUE("Several memcap options enable --fast implicitly."),
  
#undef GROUP
#undef F01
#undef F6
#undef FX
#undef FPLUS
#undef FPLUSX
#undef FINFOLINE
    CliAppSwitch_sentinel /* MUST be the last entry in the list */
  };
  return cliAppSwitches;
}
