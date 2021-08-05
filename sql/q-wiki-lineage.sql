WITH RECURSIVE
 page_name(name) AS(
--   select substr(t.tagname,6) from tag t where t.tagname glob 'wiki-*'
--   SELECT 'home' -- long history
--   UNION ALL
   SELECT 'HackersGuide' -- short history
   UNION ALL
   SELECT 'building' -- moderate history
 ),
 wiki_tagids(name, rid,mtime) AS (
   SELECT page_name.name, x.rid AS rid, x.mtime AS mtime
   FROM tag t, tagxref x, page_name
   WHERE x.tagid=t.tagid
   AND t.tagname='wiki-'||page_name.name
--   ORDER BY mtime DESC
 ),
 wiki_lineage(name, rid,uuid, mtime, size, user) AS(
   SELECT wt.name name, wt.rid rid,
          b.uuid uuid,
          wt.mtime mtime,
          b.size size,
          e.user user
    FROM wiki_tagids wt,
         blob b,
         event e
    WHERE wt.rid=b.rid
    AND e.objid=b.rid
 )
SELECT name, rid,uuid,datetime(mtime,'localtime'),size,user
FROM wiki_lineage
ORDER BY mtime DESC;
