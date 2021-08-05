-- Here's a reference point for a branch/merge test:
-- http://fossil.wanderinghorse.net/repos/libfossil/index.cgi/timeline?n=20&y=ci&b=2014-01-29+18:22:00

-- All ancestors (direct or merged!) of the checkin
-- RID given in the first SELECT...
WITH RECURSIVE
-- Top-level configuration for this query...
  conf(id,primaryOnly,cutoffDays,historyLimit) AS (
    SELECT
-- origin RID or UUID. Change this to the RID/UUID of the
-- origin for ancestry tracking:
--    SELECT 3003 /*2984*/ /*3285*/ as id -- origin RID
--    SELECT '8f89acc0f05df7bae1e7946efe5324f1e6905a9e' as id

-- d508 is one step after a merge:
     'd508a2e7ab04caf0f228e71babb1c9c69894a903'
--    3089
-- d508 poses an interesting problem: the branch point
-- (6b581c8) is listed twice, once as the parent along
-- trunk and aonce along the branch. To make that data
-- useful we have to add a "childRid" field to the result
-- so that its lineage is no longer ambiguous.

-- 1d59 is at the end of that merged branch.
--    SELECT '1d59e4291a04f17bbdbe74154bb4d53c095a284a' as id

      AS id,
      0 AS primaryOnly, -- true to follow only primary (non-branch) parents
      0.0 AS cutoffDays, -- max number of days back from origin version
      10 AS historyLimit -- max number of entries from origin version
  ),
  origin(rid, mtime, cutoffTime, branch) AS(
    -- origin RID
    SELECT b.rid as rid,
           e.mtime as mtime,
--           (e.mtime - 10) as cutoffTime -- Julian days
           (CASE WHEN 0.0=conf.cutoffDays THEN 0.0 ELSE mtime - conf.cutoffDays END) cutoffTime,
           (SELECT group_concat(substr(tagname,5), ', ') FROM tag, tagxref
               WHERE tagname GLOB 'sym-*' AND tag.tagid=tagxref.tagid
               AND tagxref.rid=b.rid AND tagxref.tagtype>0) as branch
    FROM blob b, event e, conf
    WHERE
--      b.rid=conf.id
-- or use the UUID of the origin:
      -- b.uuid=conf.id
      (CASE WHEN 'integer'=typeof(conf.id) THEN b.rid=conf.id ELSE b.uuid=conf.id END)
    AND e.objid=b.rid
  ),
  lineage(rid,childRid,uuid,tm,user,branch,comment) AS (
     SELECT origin.rid, 0, b.uuid, origin.mtime, e.user,
            origin.branch,
            coalesce(e.ecomment,e.comment)
        FROM blob b, event e, origin
        WHERE b.rid=origin.rid and e.objid=b.rid
     UNION ALL
     SELECT p.pid, p.cid, b.uuid, e.mtime, e.user,
           (SELECT group_concat(substr(tagname,5), ', ') FROM tag, tagxref
               WHERE tagname GLOB 'sym-*' AND tag.tagid=tagxref.tagid
               AND tagxref.rid=b.rid AND tagxref.tagtype>0) as branch,
            coalesce(e.ecomment,e.comment)
     FROM plink p, blob b,
          lineage a, event e,
          origin, conf
        WHERE
        p.pid=b.rid
        AND p.cid=a.rid
        AND e.objid=p.pid
-- Whether or not to follow non-merge parents...
        AND (CASE WHEN conf.primaryOnly THEN p.isprim ELSE 1 END)
-- Only trace back this far in time...
        AND e.mtime >= origin.cutoffTime
-- ^^^ if that is removed, also remove origin from the join!

-- Optionally limit it to the first N
-- lineage (including the original checkin):
    LIMIT (SELECT historyLimit FROM conf)
 )
SELECT
       a.rid rid,
       a.childRid,
       substr(a.uuid,0,8) uuid,
       datetime(a.tm,'localtime') time,
       a.branch,
       substr(a.comment,0,20)||'...' comment
from lineage a, lineage b
-- WHERE b.rid=a.childRid
-- OR a.rid=b.rid
-- OR (a.childRid=0 AND b.rid=a.rid)
WHERE a.rid=b.childRid
-- OR (b.childRid=0 AND a.rid=b.rid)
ORDER BY time DESC
;
