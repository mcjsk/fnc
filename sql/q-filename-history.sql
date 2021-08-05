-- For a given file name, find all changes in the history of
-- that file, stopping at the point where it was added or
-- renamed.
SELECT
substr(b.uuid,0,12) as manifestUuid,
datetime(p.mtime) as manifestTime,
--       ml.*,
ml.mid AS manifestRid,
-- b.size AS manifestSize,
ml.pid AS parentManifestRid,
ml.fid AS fileContentRid

-- fileContentRid=0 at the point of a rename (under the old name). The
-- fields (manifest*, prevFileContentRid) will match at the rename
-- point across this query and the same query against the renamed
-- file. Only (fileContentRid, filename) always differ across the
-- rename-point records.
-- renamedEntry.fileContentRid=origEntry.prevFileContentRid if no
-- changes were made to the file between renaming and committing.
,
ml.pid AS prevFileContentRid
  -- prevFileContentRid=0 at start of history,
  -- prevFileContentRid=fileContentRid for a file which was just
  -- renamed UNLESS it was modified after the rename, in which case...
  -- ???
,
fn.name AS filename,
prevfn.name AS priorname
FROM
mlink ml, -- map of files/filenames to checkins
filename fn,
blob b, -- checkin manifest
plink p -- checkin heritage
-- This LEFT JOIN adds rename info:
LEFT JOIN filename prevfn ON ml.pfnid=prevfn.fnid
WHERE
(fn.name
    -- IN('f-status.c', 'f-apps/f-status.c')
    -- GLOB '*f-status.c'
    -- = 'test.c'
    = 'f-status.c' -- was renamed/moved to...
    OR fn.name = 'f-apps/f-status.c' -- was target of ^^^^ that rename
    -- = 'src/fsl_io.c' -- short history
    -- = 'Makefile.in' -- long history with merge
)
-- Interesting: with this is will ONLY report the rename point:
--   AND ml.pfnid=prevfn.fnid
AND ml.fnid=fn.fnid
AND ml.mid=b.rid
AND p.cid=ml.mid -- renamed file will have non-0 here
ORDER BY manifestTime DESC
;
