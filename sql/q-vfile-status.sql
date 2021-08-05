-- provides output similar to fossil(1)'s status command, with the caveat that
-- that that command is the one which updates the vtile table referenced here.
--
-- Requires both a checkout and repo db to be attached.
SELECT
        id,vid, mrid, deleted,
        chnged,
        datetime(mtime,'unixepoch','localtime') as local_time,
        size, uuid, origname, pathname
FROM vfile LEFT JOIN blob ON vfile.mrid=blob.rid
WHERE vid=(SELECT value FROM vvar WHERE name='checkout')
AND chnged
ORDER BY pathname;