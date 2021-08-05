-- Posted by Petite Abeille to the sqlite-users mailing list:
-- fetch all compile options used for this version of sqlite3:
with
Option( name, position )
as
(
  select  sqlite_compileoption_get( 1 ) as name,
          0 as position

  union all
  select  sqlite_compileoption_get( position + 1 ) as name,
          position + 1 as position
  from    Option
  where   sqlite_compileoption_get( position + 1 ) is not null
)
select    DISTINCT name
from      Option
order by  name
;
