-- This file contains the schema for the database that is kept in the
-- ~/.fossil file and that stores information about the users setup.
--
CREATE TABLE cfg.global_config(
  name TEXT PRIMARY KEY,
  value TEXT
);

-- Identifier for this file type.
-- The integer is the same as 'FSLG'.
PRAGMA cfg.application_id=252006675;
