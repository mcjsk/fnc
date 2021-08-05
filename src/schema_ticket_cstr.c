/* Binary form of file ../sql/ticket.sql */
/** @page page_schema_ticket_cstr Schema: ticket.sql
@code
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
 @endcode
 @see schema_ticket()
*/
/* auto-generated code - edit at your own risk! (Good luck with that!) */
static char const fsl_schema_ticket_cstr_a[] = {
45, 45, 32, 84, 101, 109, 112, 108, 97, 116, 101, 32, 102, 111, 114, 32, 116, 104, 101, 32, 
84, 73, 67, 75, 69, 84, 32, 116, 97, 98, 108, 101, 10, 67, 82, 69, 65, 84, 69, 32, 
84, 65, 66, 76, 69, 32, 114, 101, 112, 111, 46, 116, 105, 99, 107, 101, 116, 40, 10, 32, 
32, 45, 45, 32, 68, 111, 32, 110, 111, 116, 32, 99, 104, 97, 110, 103, 101, 32, 97, 110, 
121, 32, 99, 111, 108, 117, 109, 110, 32, 116, 104, 97, 116, 32, 98, 101, 103, 105, 110, 115, 
32, 119, 105, 116, 104, 32, 116, 107, 116, 95, 10, 32, 32, 116, 107, 116, 95, 105, 100, 32, 
73, 78, 84, 69, 71, 69, 82, 32, 80, 82, 73, 77, 65, 82, 89, 32, 75, 69, 89, 44, 
10, 32, 32, 116, 107, 116, 95, 117, 117, 105, 100, 32, 84, 69, 88, 84, 32, 85, 78, 73, 
81, 85, 69, 44, 10, 32, 32, 116, 107, 116, 95, 109, 116, 105, 109, 101, 32, 68, 65, 84, 
69, 44, 10, 32, 32, 116, 107, 116, 95, 99, 116, 105, 109, 101, 32, 68, 65, 84, 69, 44, 
10, 32, 32, 45, 45, 32, 65, 100, 100, 32, 97, 115, 32, 109, 97, 110, 121, 32, 102, 105, 
101, 108, 100, 32, 97, 115, 32, 114, 101, 113, 117, 105, 114, 101, 100, 32, 98, 101, 108, 111, 
119, 32, 116, 104, 105, 115, 32, 108, 105, 110, 101, 10, 32, 32, 116, 121, 112, 101, 32, 84, 
69, 88, 84, 44, 10, 32, 32, 115, 116, 97, 116, 117, 115, 32, 84, 69, 88, 84, 44, 10, 
32, 32, 115, 117, 98, 115, 121, 115, 116, 101, 109, 32, 84, 69, 88, 84, 44, 10, 32, 32, 
112, 114, 105, 111, 114, 105, 116, 121, 32, 84, 69, 88, 84, 44, 10, 32, 32, 115, 101, 118, 
101, 114, 105, 116, 121, 32, 84, 69, 88, 84, 44, 10, 32, 32, 102, 111, 117, 110, 100, 105, 
110, 32, 84, 69, 88, 84, 44, 10, 32, 32, 112, 114, 105, 118, 97, 116, 101, 95, 99, 111, 
110, 116, 97, 99, 116, 32, 84, 69, 88, 84, 44, 10, 32, 32, 114, 101, 115, 111, 108, 117, 
116, 105, 111, 110, 32, 84, 69, 88, 84, 44, 10, 32, 32, 116, 105, 116, 108, 101, 32, 84, 
69, 88, 84, 44, 10, 32, 32, 99, 111, 109, 109, 101, 110, 116, 32, 84, 69, 88, 84, 10, 
41, 59, 10, 67, 82, 69, 65, 84, 69, 32, 84, 65, 66, 76, 69, 32, 114, 101, 112, 111, 
46, 116, 105, 99, 107, 101, 116, 99, 104, 110, 103, 40, 10, 32, 32, 45, 45, 32, 68, 111, 
32, 110, 111, 116, 32, 99, 104, 97, 110, 103, 101, 32, 97, 110, 121, 32, 99, 111, 108, 117, 
109, 110, 32, 116, 104, 97, 116, 32, 98, 101, 103, 105, 110, 115, 32, 119, 105, 116, 104, 32, 
116, 107, 116, 95, 10, 32, 32, 116, 107, 116, 95, 105, 100, 32, 73, 78, 84, 69, 71, 69, 
82, 32, 82, 69, 70, 69, 82, 69, 78, 67, 69, 83, 32, 116, 105, 99, 107, 101, 116, 44, 
10, 32, 32, 116, 107, 116, 95, 114, 105, 100, 32, 73, 78, 84, 69, 71, 69, 82, 32, 82, 
69, 70, 69, 82, 69, 78, 67, 69, 83, 32, 98, 108, 111, 98, 44, 10, 32, 32, 116, 107, 
116, 95, 109, 116, 105, 109, 101, 32, 68, 65, 84, 69, 44, 10, 32, 32, 45, 45, 32, 65, 
100, 100, 32, 97, 115, 32, 109, 97, 110, 121, 32, 102, 105, 101, 108, 100, 115, 32, 97, 115, 
32, 114, 101, 113, 117, 105, 114, 101, 100, 32, 98, 101, 108, 111, 119, 32, 116, 104, 105, 115, 
32, 108, 105, 110, 101, 10, 32, 32, 108, 111, 103, 105, 110, 32, 84, 69, 88, 84, 44, 10, 
32, 32, 117, 115, 101, 114, 110, 97, 109, 101, 32, 84, 69, 88, 84, 44, 10, 32, 32, 109, 
105, 109, 101, 116, 121, 112, 101, 32, 84, 69, 88, 84, 44, 10, 32, 32, 105, 99, 111, 109, 
109, 101, 110, 116, 32, 84, 69, 88, 84, 10, 41, 59, 10, 67, 82, 69, 65, 84, 69, 32, 
73, 78, 68, 69, 88, 32, 114, 101, 112, 111, 46, 116, 105, 99, 107, 101, 116, 99, 104, 110, 
103, 95, 105, 100, 120, 49, 32, 79, 78, 32, 116, 105, 99, 107, 101, 116, 99, 104, 110, 103, 
40, 116, 107, 116, 95, 105, 100, 44, 32, 116, 107, 116, 95, 109, 116, 105, 109, 101, 41, 59, 
10, 
0};
char const * fsl_schema_ticket_cstr = fsl_schema_ticket_cstr_a;
/* end of ../sql/ticket.sql */
