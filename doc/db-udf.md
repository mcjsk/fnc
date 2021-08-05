# Fossil DB User-defined Functions

The Fossil DB schemas can be perused, in the form of commented SQL, in [](/dir/sql).

The library reserves the db symbol prefixes "fsl_" and "fx_fsl_"
(case-insensitive) for its own use - clients should not define any
functions or tables with those name prefixes. Fossil(1) reserves *all*
table names which do not start with "fx_" ("fossil extension"). During
a rebuild, fossil(1) will *drop* any repo tables it does not know
about unless their names start with "fx_".

A libfossil-bound DB handle gets several SQL-callable functions
(UDFs - User-defined Functions) for working with repository state, as
listed below in alphabetical order...

-----

## `FSL_CKOUT_DIR()`

`FSL_CKOUT_DIR([INTEGER=1])` returns the path to the current checkout directory, or NULL if no checkout is opened. If passed no argument or passed a value which evaluates to non-0 in an integer context, the trailing slash is included in the returned value (which is Fossil's historical convention with regard to directory names). If passed any value which evaluates to integer 0, the slash is not included.

e.g. to get the path to the ".fossil-settings" (versionable settings) directory for the current checkout:

```
SELECT FSL_CKOUT_DIR() || '.fossil-settings';
```


## `FSL_CI_MTIME()`

`FSL_CI_MTIME(INT,INT)` takes two RIDs as arguments: the manifest (checkin) version RID and the blob.rid value of a file which part of the first RID's checkin.

It behaves like `fsl_mtime_of_manifest_file()`, returning the calculated (and highly synthetic!) mtime as an SQL integer (Unix epoch timestamp). This is primarily
for internal use.


## `FSL_CONTENT()`

`FSL_CONTENT(INTEGER|STRING)` returns the undeltified,
uncompressed content for the blob record with the given RID (if
the argument is an integer) or symbolic name (as per
`fsl_sym_to_rid()`). If the argument does not resolve to an
in-repo blob, a db-level error is triggered. If passed an
integer, no validation is done on its validity, but such
checking can be enforced by instead passing the the RID as
a string in the form "rid:THE_RID".


## `FSL_DIRPART()`

`FSL_DIRPART(STRING[, BOOL=0])` behaves like
`fsl_file_dirpart()`, returning the result as a string
unless it is empty, in which case the result is an
SQL NULL. If passed a truthy second argument then a trailing
slash is added to the result, else the result will have
no trailing slash.

An example of getting all directory names in the repository (across all
file versions, for simplicity):

```
SELECT DISTINCT(fsl_dirpart(name)) n
FROM filename WHERE n IS NOT NULL
ORDER BY n
```

To get all the dirs for a specific version one needs to do more work. We'll leave that as an exercise for... me, and once i figure it out i'll post it. It seems that getting that information requires C-level code for the time being.

## `FSL_IS_ENQUEUED()` and `FSL_IF_ENQUEUED()`

`FSL_IS_ENQUEUED(INT)` determines whether a given file is "enqueued" in a pending checkin operation. This is normally only used internally, but "might" have some uses elsewhere. If no files have explicitly been queued up for checkin (via the `fsl_checkin_file_enqueue()` C function) then *all files* are considered to be selected (though only *modified* files would actually be checked in if a commit were made).

As its argument it expects a `vfile.id` field value (`vfile` is the table where fossil tracks the current checkout's status). It returns a truthy value if that file is selected/enqueued, else a falsy value.

`FSL_IF_ENQUEUED(INT,X,Y)` is a close counterpart of `FSL_IS_ENQUEUED()`. If the `vfile.id` passed as the first parameter is enqueued then it resolves to the `X` value, else to the `Y` value, *unless* `Y` is `NULL`, in which case it always resolves to `X`. Why? Because its only intended usage is to be passed the `(id, pathname, origname)` fields from the `vfile` table.

`FSL_IF_ENQUEUED(I,X,Y)` is basically equivalent to this pseudocode:

```
result = FSL_IS_ENQUEUED(I) ? X : ((Y IS NULL) ? X : Y)
```

## `FSL_J2U()`

`FSL_J2U(JULIAN_DAY)` expects a Julian Day value and returns its equivalent in Unix Epoch timestamp as a 64-bit integer, as per `fsl_julian_to_unix()`. Fossil tends to use Julian Days for recording timestamps, but a small few cases use Unix timestamps.


## `FSL_MATCH_VFILE_OR_DIR(p1, p2)`


A helper for resolving expressions like:

```
 WHERE pathname='X' C OR
       (pathname>'X/' C AND pathname<'X0' C)
```

i.e. is X a match for the LHS or is X a directory prefix of
LHS?

The C part is functionally equivalent to empty or `COLLATE NOCASE`,
depending on the case-sensitivity setting of the `fsl_cx` instance.

Resolves to NULL if either argument is NULL, 0 if the comparison shown
above is false, 1 if the comparison is an exact match, or 2 if p2 is a
directory prefix part of p1.

It requires that both of its arguments be canonicalized paths with no
extraneous slashes (including no trailing slash).

Examples:

```
select fsl_match_vfile_or_dir('a/b/c','a/b/c');
==> 1

select fsl_match_vfile_or_dir('a/b/c','a');
==> 2

select fsl_match_vfile_or_dir('a/b/c','a/');
==> 0 because of trailing slash on 2nd arg!

select fsl_match_vfile_or_dir('a/b/c',NULL)'
==> NULL
```

This function gets its name from being used exclusively (as of this
writing) for figuring out whether a user-provided name (the 2nd
argument) matches, or is a directory prefix of, the `vfile.pathname`
or `vfile.origname` db fields.

## `FSL_SYM2RID()`

`FSL_SYM2RID(STRING)` returns a blob RID for the given symbol, as per `fsl_sym_to_rid()`. Triggers an SQL error if `fsl_sym_to_rid()` fails.

TODO: add an optional boolean second param (default=true) which tells
it to return NULL instead of triggering an error.

## `FSL_USER()`

Returns the current value of `fsl_cx_user_get()`, or `NULL` if that is
not set.

Example:

```
# f-query -e 'select fsl_user()'
fsl_user()
stephan

# f-query -e 'select fsl_user()' --user root
fsl_user()
root
```

## `NOW()`

Returns the current time as an integer, as per `time(2)`.
