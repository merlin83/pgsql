/*
 * SQL code

- - -- load the new functions
- - --
load '/home/dz/lib/postgres/array_iterator.so';

- - -- define the array operators *=, **=, *~ and **~ for type _text
- - --
create function array_texteq(_text, text)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_all_texteq(_text, text)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_textregexeq(_text, text)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_all_textregexeq(_text, text)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create operator *= (
  leftarg=_text, 
  rightarg=text, 
  procedure=array_texteq);

create operator **= (
  leftarg=_text,
  rightarg=text,
  procedure=array_all_texteq);

create operator *~ (
  leftarg=_text,
  rightarg=text,
  procedure=array_textregexeq);

create operator **~ (
  leftarg=_text,
  rightarg=text,
  procedure=array_all_textregexeq);

- - -- define the array operators *=, **=, *~ and **~ for type _char16
- - --
create function array_char16eq(_char16, char16)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_all_char16eq(_char16, char16)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_char16regexeq(_char16, text)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_all_char16regexeq(_char16, text)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create operator *= (
  leftarg=_char16,
  rightarg=char16,
  procedure=array_char16eq);

create operator **= (
  leftarg=_char16,
  rightarg=char16,
  procedure=array_all_char16eq);

create operator *~ (
  leftarg=_char16,
  rightarg=text,
  procedure=array_char16regexeq);

create operator **~ (
  leftarg=_char16,
  rightarg=text,
  procedure=array_all_char16regexeq);

- - -- define the array operators *=, **=, *> and **> for type _int4
- - --
create function array_int4eq(_int4, int4)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_all_int4eq(_int4, int4)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_int4gt(_int4, int4)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create function array_all_int4gt(_int4, int4)
  returns bool
  as '/home/dz/lib/postgres/array_iterator.so' 
  language 'c';

create operator *= (
  leftarg=_int4,
  rightarg=int4,
  procedure=array_int4eq);

create operator **= (
  leftarg=_int4,
  rightarg=int4,
  procedure=array_all_int4eq);

create operator *> (
  leftarg=_int4,
  rightarg=int4,
  procedure=array_int4gt);

create operator **> (
  leftarg=_int4,
  rightarg=int4,
  procedure=array_all_int4gt);

*/

/* end of file */

