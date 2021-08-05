CREATE TABLE repo.forumpost(
  fpid INTEGER PRIMARY KEY,  -- BLOB.rid for the artifact
  froot INT,                 -- fpid of the thread root
  fprev INT,                 -- Previous version of this same post
  firt INT,                  -- This post is in-reply-to
  fmtime REAL                -- When posted.  Julian day
);
CREATE INDEX repo.forumthread ON forumpost(froot,fmtime);
