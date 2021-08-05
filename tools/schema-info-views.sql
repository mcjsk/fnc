-- Source: https://sqlite.org/forum/forumpost/9c83d6f39970e7cc
--
--  Schema Info Views
--
-- This is a set of views providing Schema information
-- for SQLite DBs in table format.
--
DROP VIEW IF EXISTS SysIndexColumns;
DROP VIEW IF EXISTS SysIndexes;
DROP VIEW IF EXISTS SysColumns;
DROP VIEW IF EXISTS SysObjects;
--
CREATE TEMP VIEW SysObjects AS
SELECT ObjectType COLLATE NOCASE, ObjectName COLLATE NOCASE
   FROM (SELECT type AS ObjectType, name AS ObjectName
           FROM sqlite_master
          WHERE type IN ('table', 'view', 'index')
        )
;
--
CREATE TEMP VIEW SysColumns AS
SELECT ObjectType COLLATE NOCASE, ObjectName COLLATE NOCASE, ColumnID 
COLLATE NOCASE,
        ColumnName COLLATE NOCASE, Type COLLATE NOCASE, Affinity COLLATE NOCASE,
        IsNotNull, DefaultValue, IsPrimaryKey
   FROM (SELECT ObjectType, ObjectName, cid AS ColumnID,
   	name AS ColumnName, type AS Type,
                CASE
                  WHEN trim(type) = '' THEN 'Blob'
                  WHEN instr(UPPER(type),'INT' )>0 THEN 'Integer'
                  WHEN instr(UPPER(type),'CLOB')>0 THEN 'Text'
                  WHEN instr(UPPER(type),'CHAR')>0 THEN 'Text'
                  WHEN instr(UPPER(type),'TEXT')>0 THEN 'Text'
                  WHEN instr(UPPER(type),'BLOB')>0 THEN 'Blob'
                  WHEN instr(UPPER(type),'REAL')>0 THEN 'Real'
                  WHEN instr(UPPER(type),'FLOA')>0 THEN 'Real'
                  WHEN instr(UPPER(type),'DOUB')>0 THEN 'Real'
                  ELSE 'Numeric'
                END AS Affinity,
                "notnull" AS IsNotNull, dflt_value as DefaultValue,
		pk AS IsPrimaryKey
           FROM SysObjects
           JOIN pragma_table_info(ObjectName)
        )
;
--
CREATE TEMP VIEW SysIndexes AS
SELECT ObjectType COLLATE NOCASE, ObjectName COLLATE NOCASE,
       IndexName COLLATE NOCASE, IndexID, IsUnique COLLATE NOCASE,
       IndexOrigin COLLATE NOCASE, isPartialIndex
   FROM (SELECT ObjectType,ObjectName,name AS IndexName, seq AS IndexID,
                "unique" AS isUnique, origin AS IndexOrigin,
		partial AS isPartialIndex
           FROM SysObjects
           JOIN pragma_index_list(ObjectName)
        )
;
--
CREATE TEMP VIEW SysIndexColumns AS
SELECT ObjectType COLLATE NOCASE, ObjectName COLLATE NOCASE,
       IndexName COLLATE NOCASE,
       IndexColumnSequence, ColumnID, ColumnName COLLATE NOCASE,
       isDescendingOrder, Collation, isPartOfKey
   FROM (SELECT ObjectType, ObjectName, IndexName, seqno AS 
IndexColumnSequence, cid AS ColumnID,
                name AS ColumnName, "desc" AS isDescendingOrder,
		coll AS Collation, key AS isPartOfKey
           FROM SysIndexes
           JOIN pragma_index_xinfo(IndexName)
        )
;
