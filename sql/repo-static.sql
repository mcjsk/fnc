-- This file contains parts of the schema that are fixed and
-- unchanging across Fossil versions.


-- The BLOB and DELTA tables contain all records held in the repository.
--
-- The BLOB.CONTENT column is always compressed using zlib.  This
-- column might hold the full text of the record or it might hold
-- a delta that is able to reconstruct the record from some other
-- record.  If BLOB.CONTENT holds a delta, then a DELTA table entry
-- will exist for the record and that entry will point to another
-- entry that holds the source of the delta.  Deltas can be chained.
--
-- The blob and delta tables collectively hold the "global state" of
-- a Fossil repository.  
--
CREATE TABLE repo.blob(
  rid INTEGER PRIMARY KEY,        -- Record ID
  rcvid INTEGER,                  -- Origin of this record
  size INTEGER,                   -- Size of content. -1 for a phantom.
  uuid TEXT UNIQUE NOT NULL,      -- SHA1 hash of the content
  content BLOB,                   -- Compressed content of this record
  CHECK( length(uuid)>=40 AND rid>0 )
);
CREATE TABLE repo.delta(
  rid INTEGER PRIMARY KEY,                 -- Record ID
  srcid INTEGER NOT NULL REFERENCES blob   -- Record holding source document
);
CREATE INDEX repo.delta_i1 ON delta(srcid);

-------------------------------------------------------------------------
-- The BLOB and DELTA tables above hold the "global state" of a Fossil
-- project; the stuff that is normally exchanged during "sync".  The
-- "local state" of a repository is contained in the remaining tables of
-- the zRepositorySchema1 string.  
-------------------------------------------------------------------------

-- Whenever new blobs are received into the repository, an entry
-- in this table records the source of the blob.
--
CREATE TABLE repo.rcvfrom(
  rcvid INTEGER PRIMARY KEY,      -- Received-From ID
  uid INTEGER REFERENCES user,    -- User login
  mtime DATETIME,                 -- Time of receipt.  Julian day.
  nonce TEXT UNIQUE,              -- Nonce used for login
  ipaddr TEXT                     -- Remote IP address.  NULL for direct.
);
INSERT INTO repo.rcvfrom(rcvid,uid,mtime,nonce,ipaddr)
VALUES (1, 1, julianday('now'), NULL, NULL);

-- Information about users
--
-- The user.pw field can be either cleartext of the password, or
-- a SHA1 hash of the password.  If the user.pw field is exactly 40
-- characters long we assume it is a SHA1 hash.  Otherwise, it is
-- cleartext.  The sha1_shared_secret() routine computes the password
-- hash based on the project-code, the user login, and the cleartext
-- password.
--
CREATE TABLE repo.user(
  uid INTEGER PRIMARY KEY,        -- User ID
  login TEXT UNIQUE,              -- login name of the user
  pw TEXT,                        -- password
  cap TEXT,                       -- Capabilities of this user
  cookie TEXT,                    -- WWW login cookie
  ipaddr TEXT,                    -- IP address for which cookie is valid
  cexpire DATETIME,               -- Time when cookie expires
  info TEXT,                      -- contact information
  mtime DATE,                     -- last change.  seconds since 1970
  photo BLOB                      -- JPEG image of this user
);

-- The VAR table holds miscellanous information about the repository.
-- in the form of name-value pairs.
--
CREATE TABLE repo.config(
  name TEXT PRIMARY KEY NOT NULL,  -- Primary name of the entry
  value CLOB,                      -- Content of the named parameter
  mtime DATE,                      -- last modified.  seconds since 1970
  CHECK( typeof(name)='text' AND length(name)>=1 )
);

-- Artifacts that should not be processed are identified in the
-- "shun" table.  Artifacts that are control-file forgeries or
-- spam or artifacts whose contents violate administrative policy
-- can be shunned in order to prevent them from contaminating
-- the repository.
--
-- Shunned artifacts do not exist in the blob table.  Hence they
-- have not artifact ID (rid) and we thus must store their full
-- UUID.
--
CREATE TABLE repo.shun(
  uuid UNIQUE,          -- UUID of artifact to be shunned. Canonical form
  mtime DATE,           -- When added.  seconds since 1970
  scom TEXT             -- Optional text explaining why the shun occurred
);

-- Artifacts that should not be pushed are stored in the "private"
-- table.  Private artifacts are omitted from the "unclustered" and
-- "unsent" tables.
--
CREATE TABLE repo.private(rid INTEGER PRIMARY KEY);

-- An entry in this table describes a database query that generates a
-- table of tickets.
--
CREATE TABLE repo.reportfmt(
   rn INTEGER PRIMARY KEY,  -- Report number
   owner TEXT,              -- Owner of this report format (not used)
   title TEXT UNIQUE,       -- Title of this report
   mtime DATE,              -- Last modified.  seconds since 1970
   cols TEXT,               -- A color-key specification
   sqlcode TEXT             -- An SQL SELECT statement for this report
);

-- Some ticket content (such as the originators email address or contact
-- information) needs to be obscured to protect privacy.  This is achieved
-- by storing an SHA1 hash of the content.  For display, the hash is
-- mapped back into the original text using this table.  
--
-- This table contains sensitive information and should not be shared
-- with unauthorized users.
--
CREATE TABLE repo.concealed(
  hash TEXT PRIMARY KEY,    -- The SHA1 hash of content
  mtime DATE,               -- Time created.  Seconds since 1970
  content TEXT              -- Content intended to be concealed
);

-- The application ID helps the unix "file" command to identify the
-- database as a fossil repository.
PRAGMA repo.application_id=252006673;
