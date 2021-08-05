INSERT INTO reportfmt(title,mtime,cols,sqlcode) 
VALUES('All Tickets',julianday('1970-01-01'),'#ffffff Key:
#f2dcdc Active
#e8e8e8 Review
#cfe8bd Fixed
#bde5d6 Tested
#cacae5 Deferred
#c8c8c8 Closed','SELECT
  CASE WHEN status IN (''Open'',''Verified'') THEN ''#f2dcdc''
       WHEN status=''Review'' THEN ''#e8e8e8''
       WHEN status=''Fixed'' THEN ''#cfe8bd''
       WHEN status=''Tested'' THEN ''#bde5d6''
       WHEN status=''Deferred'' THEN ''#cacae5''
       ELSE ''#c8c8c8'' END AS ''bgcolor'',
  substr(tkt_uuid,1,10) AS ''#'',
  datetime(tkt_mtime) AS ''mtime'',
  type,
  status,
  subsystem,
  title
FROM ticket');
