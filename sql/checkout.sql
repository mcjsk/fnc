-- The VVAR table holds miscellanous information about the local database
-- in the form of name-value pairs.  This is similar to the VAR table
-- table in the repository except that this table holds information that
-- is specific to the local checkout.
--
-- Important Variables:
--
--     repository        Full pathname of the repository database
--     user-id           Userid to use
--
CREATE TABLE ckout.vvar(
  name TEXT PRIMARY KEY NOT NULL,  -- Primary name of the entry
  value CLOB,                      -- Content of the named parameter
  CHECK( typeof(name)='text' AND length(name)>=1 )
);

-- Each entry in the vfile table represents a single file in the
-- current checkout.
--
-- The file.rid field is 0 for files or folders that have been
-- added but not yet committed.
--
-- Vfile.chnged is 0 for unmodified files, 1 for files that have
-- been edited or which have been subjected to a 3-way merge.
-- Vfile.chnged is 2 if the file has been replaced from a different
-- version by the merge and 3 if the file has been added by a merge.
-- Vfile.chnged is 4|5 is the same as 2|3, but the operation has been
-- done by an --integrate merge.  The difference between vfile.chnged==2|4
-- and a regular add is that with vfile.chnged==2|4 we know that the
-- current version of the file is already in the repository.
--
CREATE TABLE ckout.vfile(
  id INTEGER PRIMARY KEY,           -- ID of the checked out file
  vid INTEGER REFERENCES blob,      -- The baseline this file is part of.
  chnged INT DEFAULT 0,             -- 0:unchnged 1:edited 2:m-chng 3:m-add 4:i-chng 5:i-add
  deleted BOOLEAN DEFAULT 0,        -- True if deleted
  isexe BOOLEAN,                    -- True if file should be executable
  islink BOOLEAN,                   -- True if file should be symlink
  rid INTEGER,                      -- Originally from this repository record
  mrid INTEGER,                     -- Based on this record due to a merge
  mtime INTEGER,                    -- Mtime of file on disk. sec since 1970
  pathname TEXT,                    -- Full pathname relative to root
  origname TEXT,                    -- Original pathname. NULL if unchanged
  mhash TEXT,                       -- Hash of mrid iff mrid!=rid. Added 2019-01-19.
  UNIQUE(pathname,vid)
);

-- This table holds a record of uncommitted merges in the local
-- file tree.  If a VFILE entry with id has merged with another
-- record, there is an entry in this table with (id,merge) where
-- merge is the RECORD table entry that the file merged against.
-- An id of 0 or <-3 here means the version record itself.  When
-- id==(-1) that is a cherrypick merge, id==(-2) that is a
-- backout merge and id==(-4) is a integrate merge.

CREATE TABLE ckout.vmerge(
  id INTEGER REFERENCES vfile,      -- VFILE entry that has been merged
  merge INTEGER,                    -- Merged with this record
  mhash TEXT                        -- SHA1/SHA3 hash for merge object
);
CREATE UNIQUE INDEX ckout.vmergex1 ON vmerge(id,mhash);

-- The following trigger will prevent older versions of Fossil that
-- do not know about the new vmerge.mhash column from updating the
-- vmerge table.  This must be done with a trigger, since legacy Fossil
-- uses INSERT OR IGNORE to update vmerge, and the OR IGNORE will cause
-- a NOT NULL constraint to be silently ignored.
CREATE TRIGGER ckout.vmerge_ck1 AFTER INSERT ON vmerge
WHEN new.mhash IS NULL BEGIN
  SELECT raise(FAIL,
  'trying to update a newer checkout with an older version of Fossil');
END;

-- Identifier for this file type.
-- The integer is the same as 'FSLC'.
PRAGMA ckout.application_id=252006674;
