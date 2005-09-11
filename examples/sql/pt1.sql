--
-- A simple name => value pair, where the value is just the hash of the name;
-- meant for storing values between 1 and 1M or so.
-- 
-- The database used for the pt1.g/pt1d.C test suite.
--
-- Run this command as:
--
--    mysqladmin -u root create pt1
--    mysql -u root pt1 < pt1.sql
--
-- $Id$

create table sha1_tab ( 
	x integer unsigned not null primary key,
	y varchar(100) not null
);
