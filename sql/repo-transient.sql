-- This file contains parts of the schema that can change from one
-- version to the next. The data stored in these tables is
-- reconstructed from the information in the main repo schema by the
-- "rebuild" operation.

-- Filenames
--
CREATE TABLE repo.filename(
  fnid INTEGER PRIMARY KEY,    -- Filename ID
  name TEXT UNIQUE             -- Name of file page
);

-- Linkages between check-ins, files created by each check-in, and
-- the names of those files.
--
-- Each entry represents a file that changed content from pid to fid
-- due to the check-in that goes from pmid to mid.  fnid is the name
-- of the file in the mid check-in.  If the file was renamed as part
-- of the mid check-in, then pfnid is the previous filename.
--
-- There can be multiple entries for (mid,fid) if the mid check-in was
-- a merge.  Entries with isaux==0 are from the primary parent.  Merge
-- parents have isaux set to true.
--
-- Field name mnemonics:
--    mid = Manifest ID.  (Each check-in is stored as a "Manifest")
--    fid = File ID.
--    pmid = Parent Manifest ID.
--    pid = Parent file ID.
--    fnid = File Name ID.
--    pfnid = Parent File Name ID.
--    isaux = pmid IS AUXiliary parent, not primary parent
--
-- pid==0    if the file is added by check-in mid.
-- pid==(-1) if the file exists in a merge parents but not in the primary
--           parent.  In other words, if the file file was added by merge.
--           (TODO: confirm if/where this is used in fossil and then make sure
--           libfossil does so, too.)
-- fid==0    if the file is removed by check-in mid.
--
CREATE TABLE repo.mlink(
  mid INTEGER,        -- Check-in that contains fid
  fid INTEGER,        -- New file content RID. 0 if deleted
  pmid INTEGER,       -- Check-in RID that contains pid
  pid INTEGER,        -- Prev file content RID. 0 if new. -1 if from a merge
  fnid INTEGER REFERENCES filename,   -- Name of the file
  pfnid INTEGER,      -- Previous name. 0 if unchanged
  mperm INTEGER,                      -- File permissions.  1==exec
  isaux BOOLEAN DEFAULT 0             -- TRUE if pmid is the primary
);
CREATE INDEX repo.mlink_i1 ON mlink(mid);
CREATE INDEX repo.mlink_i2 ON mlink(fnid);
CREATE INDEX repo.mlink_i3 ON mlink(fid);
CREATE INDEX repo.mlink_i4 ON mlink(pid);

-- Parent/child linkages between checkins
--
CREATE TABLE repo.plink(
  pid INTEGER REFERENCES blob,    -- Parent manifest
  cid INTEGER REFERENCES blob,    -- Child manifest
  isprim BOOLEAN,                 -- pid is the primary parent of cid
  mtime DATETIME,                 -- the date/time stamp on cid.  Julian day.
  baseid INTEGER REFERENCES blob, -- Baseline if cid is a delta manifest.
  UNIQUE(pid, cid)
);
CREATE INDEX repo.plink_i2 ON plink(cid,pid);

-- A "leaf" checkin is a checkin that has no children in the same
-- branch.  The set of all leaves is easily computed with a join,
-- between the plink and tagxref tables, but it is a slower join for
-- very large repositories (repositories with 100,000 or more checkins)
-- and so it makes sense to precompute the set of leaves.  There is
-- one entry in the following table for each leaf.
--
CREATE TABLE repo.leaf(rid INTEGER PRIMARY KEY);

-- Events used to generate a timeline
--
CREATE TABLE repo.event(
  type TEXT,                      -- Type of event: 'ci', 'w', 'e', 't', 'g'
  mtime DATETIME,                 -- Time of occurrence. Julian day.
  objid INTEGER PRIMARY KEY,      -- Associated record ID
  tagid INTEGER,                  -- Associated ticket or wiki name tag
  uid INTEGER REFERENCES user,    -- User who caused the event
  bgcolor TEXT,                   -- Color set by 'bgcolor' property
  euser TEXT,                     -- User set by 'user' property
  user TEXT,                      -- Name of the user
  ecomment TEXT,                  -- Comment set by 'comment' property
  comment TEXT,                   -- Comment describing the event
  brief TEXT,                     -- Short comment when tagid already seen
  omtime DATETIME                 -- Original unchanged date+time, or NULL
);
CREATE INDEX repo.event_i1 ON event(mtime);

-- A record of phantoms.  A phantom is a record for which we know the
-- UUID but we do not (yet) know the file content.
--
CREATE TABLE repo.phantom(
  rid INTEGER PRIMARY KEY         -- Record ID of the phantom
);

-- A record of orphaned delta-manifests.  An orphan is a delta-manifest
-- for which we have content, but its baseline-manifest is a phantom.
-- We have to track all orphan manifests so that when the baseline arrives,
-- we know to process the orphaned deltas.
CREATE TABLE repo.orphan(
  rid INTEGER PRIMARY KEY,        -- Delta manifest with a phantom baseline
  baseline INTEGER                -- Phantom baseline of this orphan
);
CREATE INDEX repo.orphan_baseline ON orphan(baseline);

-- Unclustered records.  An unclustered record is a record (including
-- a cluster records themselves) that is not mentioned by some other
-- cluster.
--
-- Phantoms are usually included in the unclustered table.  A new cluster
-- will never be created that contains a phantom.  But another repository
-- might send us a cluster that contains entries that are phantoms to
-- us.
--
CREATE TABLE repo.unclustered(
  rid INTEGER PRIMARY KEY         -- Record ID of the unclustered file
);

-- Records which have never been pushed to another server.  This is
-- used to reduce push operations to a single HTTP request in the
-- common case when one repository only talks to a single server.
--
CREATE TABLE repo.unsent(
  rid INTEGER PRIMARY KEY         -- Record ID of the phantom
);

-- Each baseline or manifest can have one or more tags.  A tag
-- is defined by a row in the next table.
-- 
-- Wiki pages are tagged with "wiki-NAME" where NAME is the name of
-- the wiki page.  Tickets changes are tagged with "ticket-UUID" where 
-- UUID is the indentifier of the ticket.  Tags used to assign symbolic
-- names to baselines are branches are of the form "sym-NAME" where
-- NAME is the symbolic name.
--
CREATE TABLE repo.tag(
  tagid INTEGER PRIMARY KEY,       -- Numeric tag ID
  tagname TEXT UNIQUE              -- Tag name.
);
INSERT INTO repo.tag VALUES(1, 'bgcolor');         -- FSL_TAGID_BGCOLOR
INSERT INTO repo.tag VALUES(2, 'comment');         -- FSL_TAGID_COMMENT
INSERT INTO repo.tag VALUES(3, 'user');            -- FSL_TAGID_USER
INSERT INTO repo.tag VALUES(4, 'date');            -- FSL_TAGID_DATE
INSERT INTO repo.tag VALUES(5, 'hidden');          -- FSL_TAGID_HIDDEN
INSERT INTO repo.tag VALUES(6, 'private');         -- FSL_TAGID_PRIVATE
INSERT INTO repo.tag VALUES(7, 'cluster');         -- FSL_TAGID_CLUSTER
INSERT INTO repo.tag VALUES(8, 'branch');          -- FSL_TAGID_BRANCH
INSERT INTO repo.tag VALUES(9, 'closed');          -- FSL_TAGID_CLOSED
INSERT INTO repo.tag VALUES(10,'parent');          -- FSL_TAGID_PARENT
INSERT INTO repo.tag VALUES(11,'note');            -- FSL_TAG_NOTE
-- arguable, to force auto-increment to start at 100:
-- INSERT INTO tag VALUES(99,'FSL_TAGID_MAX_INTERNAL');

-- Assignments of tags to baselines.  Note that we allow tags to
-- have values assigned to them.  So we are not really dealing with
-- tags here.  These are really properties.  But we are going to
-- keep calling them tags because in many cases the value is ignored.
--
CREATE TABLE repo.tagxref(
  tagid INTEGER REFERENCES tag,   -- The tag that was added or removed
  tagtype INTEGER,                -- 0:-,cancel  1:+,single  2:*,propagate
  srcid INTEGER REFERENCES blob,  -- Artifact of tag. 0 for propagated tags
  origid INTEGER REFERENCES blob, -- check-in holding propagated tag
  value TEXT,                     -- Value of the tag.  Might be NULL.
  mtime TIMESTAMP,                -- Time of addition or removal. Julian day
  rid INTEGER REFERENCE blob,     -- Artifact tag is applied to
  UNIQUE(rid, tagid)
);
CREATE INDEX repo.tagxref_i1 ON tagxref(tagid, mtime);

-- When a hyperlink occurs from one artifact to another (for example
-- when a check-in comment refers to a ticket) an entry is made in
-- the following table for that hyperlink.  This table is used to
-- facilitate the display of "back links".
--
CREATE TABLE repo.backlink(
  target TEXT,           -- Where the hyperlink points to
  srctype INT,           -- 0: check-in  1: ticket  2: wiki
  srcid INT,             -- rid for checkin or wiki.  tkt_id for ticket.
  mtime TIMESTAMP,       -- time that the hyperlink was added. Julian day.
  UNIQUE(target, srctype, srcid)
);
CREATE INDEX repo.backlink_src ON backlink(srcid, srctype);

-- Each attachment is an entry in the following table.  Only
-- the most recent attachment (identified by the D card) is saved.
--
CREATE TABLE repo.attachment(
  attachid INTEGER PRIMARY KEY,   -- Local id for this attachment
  isLatest BOOLEAN DEFAULT 0,     -- True if this is the one to use
  mtime TIMESTAMP,                -- Last changed.  Julian day.
  src TEXT,                       -- UUID of the attachment.  NULL to delete
  target TEXT,                    -- Object attached to. Wikiname or Tkt UUID
  filename TEXT,                  -- Filename for the attachment
  comment TEXT,                   -- Comment associated with this attachment
  user TEXT                       -- Name of user adding attachment
);
CREATE INDEX repo.attachment_idx1 ON attachment(target, filename, mtime);
CREATE INDEX repo.attachment_idx2 ON attachment(src);

-- For tracking cherrypick merges
CREATE TABLE repo.cherrypick(
  parentid INT,
  childid INT,
  isExclude BOOLEAN DEFAULT false,
  PRIMARY KEY(parentid, childid)
) WITHOUT ROWID;
CREATE INDEX repo.cherrypick_cid ON cherrypick(childid);
