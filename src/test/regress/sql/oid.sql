-- *************testing built-in type oid ****************
CREATE TABLE OID_TBL(f1 oid);

INSERT INTO OID_TBL(f1) VALUES ('1234');

INSERT INTO OID_TBL(f1) VALUES ('1235');

INSERT INTO OID_TBL(f1) VALUES ('987');

INSERT INTO OID_TBL(f1) VALUES ('-1040');

INSERT INTO OID_TBL(f1) VALUES ('');

-- bad inputs 
INSERT INTO OID_TBL(f1) VALUES ('asdfasd');

SELECT '' AS five, OID_TBL.*;


SELECT '' AS one, o.* FROM OID_TBL o WHERE o.f1 = '1234'::oid;

SELECT '' AS four, o.* FROM OID_TBL o WHERE o.f1 <> '1234';

SELECT '' AS four, o.* FROM OID_TBL o WHERE o.f1 <= '1234';

SELECT '' AS three, o.* FROM OID_TBL o WHERE o.f1 < '1234';

SELECT '' AS two, o.* FROM OID_TBL o WHERE o.f1 >= '1234';

SELECT '' AS one, o.* FROM OID_TBL o WHERE o.f1 > '1234';


