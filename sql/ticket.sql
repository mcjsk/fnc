-- Template for the TICKET table
CREATE TABLE repo.ticket(
  -- Do not change any column that begins with tkt_
  tkt_id INTEGER PRIMARY KEY,
  tkt_uuid TEXT UNIQUE,
  tkt_mtime DATE,
  tkt_ctime DATE,
  -- Add as many field as required below this line
  type TEXT,
  status TEXT,
  subsystem TEXT,
  priority TEXT,
  severity TEXT,
  foundin TEXT,
  private_contact TEXT,
  resolution TEXT,
  title TEXT,
  comment TEXT
);
CREATE TABLE repo.ticketchng(
  -- Do not change any column that begins with tkt_
  tkt_id INTEGER REFERENCES ticket,
  tkt_rid INTEGER REFERENCES blob,
  tkt_mtime DATE,
  -- Add as many fields as required below this line
  login TEXT,
  username TEXT,
  mimetype TEXT,
  icomment TEXT
);
CREATE INDEX repo.ticketchng_idx1 ON ticketchng(tkt_id, tkt_mtime);
